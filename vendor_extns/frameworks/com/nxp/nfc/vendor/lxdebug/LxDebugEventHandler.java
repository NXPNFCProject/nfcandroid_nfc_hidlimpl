/*
 *
 *  The original Work has been changed by NXP.
 *
 *  Copyright 2025 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

package com.nxp.nfc.vendor.lxdebug;

import android.annotation.IntDef;
import android.content.Context;
import android.content.Intent;
import android.nfc.NfcAdapter;

import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.INxpOEMCallbacks;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;
import java.util.concurrent.Executors;

import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;


/**
 * This class is responsible to control lxdebug features
 * handles LxDebug Events specific notfications as well
 */
public class LxDebugEventHandler implements INxpNfcNtfHandler, INxpOEMCallbacks {

    private static final String TAG = "LxDebugEventHandler";

    public static final byte FIELD_DETECT_MODE_SET_SUB_GID_OID = (byte) 0x7F;
    public static final byte IS_FIELD_DETECT_ENABLED_SUB_GID_OID = (byte) 0x7E;
    public static final byte IS_FIELD_DETECT_MODE_STARTED_SUB_GID_OID = (byte) 0x7D;

    public static final byte MODE_ENABLE = 0x01;
    public static final byte MODE_DISABLE = 0x00;
    public static final byte MODE_DISABLED_CONFIG = 0x02;

    private boolean mIsEFDMStarted = false;
    private boolean mIsFirstRFFieldOn = false;
    /*Need to remove when stand by issues fixed*/
    private boolean mIsNFCCStandByConfig = true;
    private int mDetectionTimeout = 0;
    private Timer mEFDStopTimer;

    private final NfcOperations mNfcOperations;
    private final NxpNciPacketHandler mNxpNciPacketHandler;
    private final Context mContext;
    private ILxDebugCallbacks mLxDebugCallbacks = null;

    private static final int EFDSTATUS_ERROR_ALREADY_STARTED = 0x02;
    private static final int EFDSTATUS_ERROR_NOT_STARTED = 0x07;
    private static final int EFDSTATUS_ERROR_FEATURE_DISABLED_IN_CONFIG = 0x04;
    private static final int EFDSTATUS_ERROR_FEATURE_NOT_SUPPORTED = 0x03;
    private static final int EFDSTATUS_ERROR_NFC_IS_OFF = 0x05;
    private static final int EFDSTATUS_ERROR_UNKNOWN = 0x06;
    private static final String ACTION_EXTENDED_FIELD_TIMEOUT =
        "com.android.nfc.action.ACTION_EXTENDED_FIELD_TIMEOUT";
    private static final String ACTION_LX_DATA_RECVD =
            "com.android.nfc.action.LX_DATA";

    private static final int FDSTATUS_ERROR_NFC_IS_OFF = 0x01;

    private static final int STATUS_SUCCESS = 0x00;
    private static final int STATUS_FAILED = 0x01;
    private static final int ERROR_UNKNOWN = 0x03;
    private static final int SERVICE_UNAVIABLE = 0xFF;

    private static final byte NCI_OID_SYSTEM_DEBUG_STATE_L1_MESSAGE = 0x35;
    private static final byte NCI_OID_SYSTEM_DEBUG_STATE_L2_MESSAGE = 0x36;
    private static final byte NCI_OID_SYSTEM_DEBUG_STATE_L3_MESSAGE = 0x37;
    private static final byte L2_DEBUG_BYTE0_MASK  = 0x31;


    private static final byte CONF_GID = 0x20;
    private static final byte SET_CONF_OID = 0x02;
    private static final byte GET_CONF_OID = 0x03;


    @IntDef(value = {
            STATUS_SUCCESS,
            STATUS_FAILED,
            EFDSTATUS_ERROR_ALREADY_STARTED,
            EFDSTATUS_ERROR_FEATURE_NOT_SUPPORTED,
            EFDSTATUS_ERROR_FEATURE_DISABLED_IN_CONFIG,
            EFDSTATUS_ERROR_NFC_IS_OFF,
            EFDSTATUS_ERROR_UNKNOWN,
            EFDSTATUS_ERROR_NOT_STARTED,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface EfdmErrorCode {}

    @IntDef(value = {
            STATUS_SUCCESS,
            FDSTATUS_ERROR_NFC_IS_OFF,
            ERROR_UNKNOWN,
            SERVICE_UNAVIABLE,

    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface FdErrorCode {}

    public LxDebugEventHandler(NfcAdapter nfcAdapter, Context context) {
        this.mContext = context;
        this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
        this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
    }

    @Override
    public void onRfFieldDetected(boolean isActive) {
        NxpNfcLogger.d(TAG, "onRfFieldDetected: " + isActive);
        if (mIsFirstRFFieldOn && isActive) {
            startEFDMTimer();
            mIsFirstRFFieldOn = false;
        }
    }

    @Override
    public boolean onDisableRequested() {
        NxpNfcLogger.d(TAG, "onDisableRequested: ");
        if (mIsEFDMStarted || isFieldDetectStarted()) {
            stopEFDMTimer();
            if (setFieldDetectFlag(false) != STATUS_SUCCESS) {
                NxpNfcLogger.e(TAG, "setFieldDetectFlag Failed");
            }
            mIsEFDMStarted = false;
        }
        return true;
    }

    @Override
    public void onEnableFinished(int status){
        NxpNfcLogger.d(TAG, "onEnableFinished: ");
        if (!mIsNFCCStandByConfig) {
            /*Need to remove when stand by issues fixed*/
            enableNFCCStandByConfig(true);
        }
    }

    /**
     * This API registers the Application callbacks to be called
     * for LxDebug notifications.
     * @param callbacks : callback to be registered
     */
    public void registerLxDebugCallbacks(ILxDebugCallbacks callbacks) {
        NxpNfcLogger.d(TAG, "Entry registerLxDebugCallbacks");
        mLxDebugCallbacks = callbacks;
    }

    /**
     * This API unregisters the Application callbacks to be called
     * for LxDebug notifications.
     */
    public void unregisterLxDebugCallbacks() {
        NxpNfcLogger.d(TAG, "Entry unregisterLxDebugCallbacks");
        mLxDebugCallbacks = null;
    }

    /*Need to remove when stand by issues fixed*/
    private void enableNFCCStandByConfig(boolean value) {
        mIsNFCCStandByConfig = value;
        byte[] cmdPayload = {(byte) ((value == true) ? 0x01 : 0x00)};
        try {
            mNxpNciPacketHandler.shouldCheckResponseSubGid(false);
            byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(0x2F,
                    0x00, cmdPayload);
            mNxpNciPacketHandler.shouldCheckResponseSubGid(true);
            if (vendorRsp != null && vendorRsp.length > 0
                    && vendorRsp[0] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                NxpNfcLogger.d(TAG, "NFCC Standby Mode command success");
            }
        } catch (Exception e) {
            NxpNfcLogger.d(TAG, "Exception in sendVendorNciMessage ");
            e.printStackTrace();
        }
    }

    private void startEFDMTimer() {
        NxpNfcLogger.d(TAG, "Entry startEFDMTimer");
        if (!mIsEFDMStarted) {
            NxpNfcLogger.i(TAG, "Not in EFDM Mode");
            return;
        }
        mEFDStopTimer =  new Timer();
        EFDMTimerTask efdmTimerTask =  new EFDMTimerTask();
        mEFDStopTimer.schedule(efdmTimerTask, mDetectionTimeout);
        NxpNfcLogger.e(TAG, "EFDM Timer started");
    }

    class EFDMTimerTask extends TimerTask {
        @Override
        public void run() {
            NxpNfcLogger.i(TAG, "EFDM Timer Expired");
            if (mIsEFDMStarted) {
                if (mNfcOperations.isDiscoveryStarted()) {
                    mNfcOperations.disableDiscovery();
                    /*Need to remove when stand by issues fixed*/
                    enableNFCCStandByConfig(false);
                }
                if (mLxDebugCallbacks != null) {
                    mLxDebugCallbacks.onEFDMTimedout();
                }
                if (mContext != null) {
                    Intent efdmTimeoutIntent = new Intent(ACTION_EXTENDED_FIELD_TIMEOUT);
                    NxpNfcLogger.d(TAG, "BroadCasting " + ACTION_EXTENDED_FIELD_TIMEOUT);
                    mContext.sendBroadcast(efdmTimeoutIntent);
                }
            }
        }
    }

    private void stopEFDMTimer() {
        NxpNfcLogger.d(TAG, "Entry stopEFDMTimer");
        mIsFirstRFFieldOn = false;
        if (mEFDStopTimer != null) {
            mEFDStopTimer.cancel();
        }
    }

    private int setFieldDetectFlag(boolean mode) {
        NxpNfcLogger.d(TAG, "Entry setFieldDetectFlag");
        if (mNxpNciPacketHandler == null) {
            return STATUS_FAILED;
        }

        mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(), this);
        byte[] cmd = {FIELD_DETECT_MODE_SET_SUB_GID_OID,
            (byte) (mode ? MODE_ENABLE : MODE_DISABLE)};
        try {
            byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
                    NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
                    cmd);
            if (vendorRsp != null && vendorRsp.length > 1
                    && vendorRsp[0] == FIELD_DETECT_MODE_SET_SUB_GID_OID
                    && vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                return STATUS_SUCCESS;
            } else {
                NxpNfcLogger.e(TAG, "send vendor nci failed");
            }
        } catch (Exception e) {
            NxpNfcLogger.d(TAG, "Exception in sendVendorNciMessage ");
            e.printStackTrace();
            return EFDSTATUS_ERROR_UNKNOWN; /*ERROR_UNKNOWN*/
        }
        return STATUS_FAILED;
    }

    private boolean isFieldDetectStarted() {
        NxpNfcLogger.d(TAG, "Entry isFieldDetectStarted");
        if (mNxpNciPacketHandler == null) {
            NxpNfcLogger.e(TAG, "NxpNciPacketHandler is null");
            return false;
        }
        mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(), this);
        byte[] cmd = {IS_FIELD_DETECT_MODE_STARTED_SUB_GID_OID};
        try {
            byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
                    NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
                    cmd);
            if (vendorRsp != null && vendorRsp.length > 2
                    && vendorRsp[0] == IS_FIELD_DETECT_MODE_STARTED_SUB_GID_OID
                    && vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                return (vendorRsp[2] == MODE_ENABLE);
            } else {
                NxpNfcLogger.e(TAG, "send vendor nci failed");
            }
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage ");
            e.printStackTrace();
        }
        return false;
    }

    private int checkFieldDetectEnabledFromConfig() {
        NxpNfcLogger.d(TAG, "Entry checkFieldDetectEnabledFromConfig");
        if (mNxpNciPacketHandler == null) {
            NxpNfcLogger.e(TAG, " NXP Nci Pkt Handler is null");
            return STATUS_FAILED;
        }
        mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(), this);
        byte[] efdm_status_cmd = {IS_FIELD_DETECT_ENABLED_SUB_GID_OID};
        try {
            byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
                    NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
                    efdm_status_cmd);
            if (vendorRsp != null && vendorRsp.length > 1
                    && vendorRsp[0] == IS_FIELD_DETECT_ENABLED_SUB_GID_OID
                    && vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                if (vendorRsp.length >= 3) {
                    switch (vendorRsp[2]) {
                        case MODE_DISABLE:
                            return EFDSTATUS_ERROR_FEATURE_NOT_SUPPORTED;
                        case MODE_ENABLE:
                            return STATUS_SUCCESS;
                        case MODE_DISABLED_CONFIG:
                            return EFDSTATUS_ERROR_FEATURE_DISABLED_IN_CONFIG;
                        default:
                            NxpNfcLogger.e(TAG, "unkonwn mode");
                            break;
                    }
                }
            } else {
                NxpNfcLogger.e(TAG, "send vendor nci failed");
            }
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage ");
            e.printStackTrace();
        }
        return EFDSTATUS_ERROR_UNKNOWN;
    }

    /**
     * This API starts extended field detect mode.
     * @param detectionTimeout : The time after 1st RF ON to
     *                            exit extended field detect mode(msec).
     * @return status     :-0x00 :EFDSTATUS_SUCCESS
     *                      0x01 :EFDSTATUS_FAILED
     *                      0x02 :EFDSTATUS_ERROR_ALREADY_STARTED
     *                      0x03 :EFDSTATUS_ERROR_FEATURE_NOT_SUPPORTED
     *                      0x04 :EFDSTATUS_ERROR_FEATURE_DISABLED_IN_CONFIG
     *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
     *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
     */
    public @EfdmErrorCode int startExtendedFieldDetectMode(int detectionTimeout) {
        NxpNfcLogger.d(TAG, "Entry startExtendedFieldDetectMode");
        if (mNfcOperations == null) {
            NxpNfcLogger.e(TAG, "NFC Operations is null");
            return EFDSTATUS_ERROR_UNKNOWN;
        }
        if (!mNfcOperations.isEnabled()) {
            return EFDSTATUS_ERROR_NFC_IS_OFF;
        }
        if (mIsEFDMStarted) {
            return EFDSTATUS_ERROR_ALREADY_STARTED;
        }
        int status = checkFieldDetectEnabledFromConfig();
        if (status != STATUS_SUCCESS) {
            return status;
        }
        mNfcOperations.registerNxpOemCallback(this);
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
        /* check if discovery stopped */
        if (mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Failed to stop discovery");
            return STATUS_FAILED;
        }
        mDetectionTimeout = detectionTimeout;
        status = setFieldDetectFlag(true);
        if (status == STATUS_SUCCESS) {
            mIsEFDMStarted = true;
            mIsFirstRFFieldOn = true;
            mNfcOperations.setDiscoveryTech(NfcAdapter.FLAG_READER_KEEP,
                    NfcAdapter.FLAG_LISTEN_DISABLE);
        } else {
            NxpNfcLogger.e(TAG, "Failed to set Field Detect");
            mNfcOperations.enableDiscovery();
        }
        /* check if discovery started */
        if (!mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Failed to start discovery");
            status = STATUS_FAILED;
        }
        /*Need to remove when stand by issues fixed*/
        if (!mIsNFCCStandByConfig) {
            enableNFCCStandByConfig(true);
        }
        return status;
    }

    /**
     * @return status     :-0x00 :EFDSTATUS_SUCCESS
     *                      0x01 :EFDSTATUS_FAILED
     *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
     *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
     *                      0x07 :EFDSTATUS_ERROR_NOT_STARTED
     */
    public @EfdmErrorCode int stopExtendedFieldDetectMode() {
        NxpNfcLogger.d(TAG, "Entry stopExtendedFieldDetectMode");
        stopEFDMTimer();
        if (mNfcOperations == null) {
            NxpNfcLogger.e(TAG, "NFC Operations is null");
            return EFDSTATUS_ERROR_UNKNOWN;
        }
        if (!mNfcOperations.isEnabled()) {
            return EFDSTATUS_ERROR_NFC_IS_OFF;
        }
        if (!mIsEFDMStarted) {
            return EFDSTATUS_ERROR_NOT_STARTED;
        }
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
            /*Need to remove when stand by issues fixed*/
            enableNFCCStandByConfig(false);
        }
        /* check if discovery stopped */
        if (mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Failed to stop discovery");
            return STATUS_FAILED;
        }
        int status = setFieldDetectFlag(false);
        if (status != STATUS_SUCCESS) {
            NxpNfcLogger.e(TAG, "Failed to reset Field detect flag");
        }
        mIsEFDMStarted = false;
        return status;
    }

    /**
     * This API starts card emulation mode. Starts RF Discovery with Default
     * POLL & Listen configurations
     * @return status     :-0x00 :EFDSTATUS_SUCCESS
     *                      0x01 :EFDSTATUS_FAILED
     *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
     *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
     */
    public @EfdmErrorCode int startCardEmulation() {
        NxpNfcLogger.d(TAG, "Entry startCardEmulation");
        int status = STATUS_SUCCESS;
        if (mNfcOperations == null) {
            NxpNfcLogger.e(TAG, "NFC Operations is null");
            return EFDSTATUS_ERROR_UNKNOWN;
        }
        if (!mNfcOperations.isEnabled()) {
            return EFDSTATUS_ERROR_NFC_IS_OFF;
        }
        stopEFDMTimer();
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
        /* check if discovery stopped */
        if (mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Failed to stop discovery");
            return STATUS_FAILED;
        }
        if (isFieldDetectStarted()) {
            if (setFieldDetectFlag(false) != STATUS_SUCCESS) {
                status = STATUS_FAILED;
            }
        }
        mIsEFDMStarted = false;
        mNfcOperations.enableDiscovery();
        if (!mIsNFCCStandByConfig) {
            enableNFCCStandByConfig(true);
        }
        /* check if discovery started */
        if (!mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Failed to start discovery");
            status = STATUS_FAILED;
        }
        return status;
    }

    /**
     * This api is called by applications enable or disable field
     * detect feauture.
     * This api shall be called only Nfcservice is enabled.
     * @param  mode to Enable(true) and Disable(false)
     * @return whether  the update of configuration is
     *          success or not with reason.
     *          0x01  - NFC_IS_OFF,
     *          0x03  - ERROR_UNKNOWN
     *          0x00  - SUCCESS
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    public @FdErrorCode int setFieldDetectMode(boolean mode) throws IOException {
        int status = ERROR_UNKNOWN;
        if (mNfcOperations == null) {
            NxpNfcLogger.e(TAG, "NFC Operations is null");
            return status;
        }
        if (!mNfcOperations.isEnabled()) {
            NxpNfcLogger.e(TAG, "NFC in Off State");
            return FDSTATUS_ERROR_NFC_IS_OFF;
        }
        if (isFieldDetectStarted() == mode) {
            return STATUS_SUCCESS;
        }
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
        /* check if discovery stopped */
        if (mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Not able to stop discovery");
            return status;
        }
        if (setFieldDetectFlag(mode) == STATUS_SUCCESS) {
            status = STATUS_SUCCESS;
        }
        if (mode && (status == STATUS_SUCCESS)) {
            mNfcOperations.setDiscoveryTech(NfcAdapter.FLAG_READER_KEEP,
                    NfcAdapter.FLAG_LISTEN_DISABLE);
        } else {
            mNfcOperations.enableDiscovery();
        }
        /* check if discovery started */
        if (!mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Not able to start discovery");
            status = ERROR_UNKNOWN;
        }
        return status;
    }

    /**
     * detect feature is enabled or not
     * @return whether  the feature is enabled(true) disabled (false)
     *          success or not.
     *          Enabled  - true
     *          Disabled - false
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    public boolean isFieldDetectEnabled() {
        return isFieldDetectStarted();
    }

    private int sendRSSICmd(boolean mode, int rssiNtfTimeIntervalMs) throws IOException {
        if (mNxpNciPacketHandler == null) {
            NxpNfcLogger.e(TAG, " NXP Nci Pkt Handler is null");
            return ERROR_UNKNOWN;
        }
        byte rssiNtfTimeInterval = (byte) ((rssiNtfTimeIntervalMs / 10)
                + ((rssiNtfTimeIntervalMs % 10 != 0x00) ? 0x01 : 0x00));
        if (mode && (rssiNtfTimeInterval < 0x01 || rssiNtfTimeInterval > 0xFF)) {
            NxpNfcLogger.e(TAG, "Rssi NTF timeout interval should "
                    + "be in between 10 to 2550 Millisec");
            return ERROR_UNKNOWN;
        }
        byte[] cmdPayload = {0x01, (byte) 0xA1, 0x55, 0x02,
            (mode) ? MODE_ENABLE : MODE_DISABLE, rssiNtfTimeInterval};
        try {
            mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(), this);
            mNxpNciPacketHandler.shouldCheckResponseSubGid(false);
            byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(CONF_GID,
                    SET_CONF_OID, cmdPayload);
            mNxpNciPacketHandler.shouldCheckResponseSubGid(true);
            if (vendorRsp != null && vendorRsp.length > 0
                    && vendorRsp[0] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                return STATUS_SUCCESS;
            } else {
               NxpNfcLogger.e(TAG, "Send Vendor Failed");
            }
        } catch (Exception e) {
            NxpNfcLogger.d(TAG, "Exception in sendVendorNciMessage ");
            throw new IOException("Exception while sendVendorNciMessage");
        }
        return ERROR_UNKNOWN;
    }

    /**
     * This api is called by applications to start RSSI mode.
     * Once RSSI is enabled, RSSI data notifications are broadcasted to registered
     * application when the device is in the reader field. Application can then
     * analyze this data and find best position for transaction.
     * This api shall be called only after Nfcservice is enabled.
     * @param  rssiNtfTimeIntervalInMillisec to set time interval between RSSI
     * notification in milliseconds. It is recommended that this value is
     * greater than 10 millisecs and multiple of 10.
     * @return whether  the update of configuration is
     *          success or not with reason.
     *          0x01  - NFC_IS_OFF,
     *          0x03  - ERROR_UNKNOWN
     *          0x00  - SUCCESS
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    public @FdErrorCode int startRssiMode(int rssiNtfTimeIntervalInMillisec) throws IOException {
        NxpNfcLogger.d(TAG, "Entry startRssiMode");
        int status = ERROR_UNKNOWN;
        if (mNfcOperations == null) {
            NxpNfcLogger.e(TAG, "NFC Operations is null");
            return status;
        }
        if (!mNfcOperations.isEnabled()) {
            return FDSTATUS_ERROR_NFC_IS_OFF;
        }
        if (isRssiEnabled()) {
            return STATUS_SUCCESS;
        }
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
        /* check if discovery stopped */
        if (mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Not able to stop discovery");
            return status;
        }
        if ((setFieldDetectFlag(true) == STATUS_SUCCESS)
                && (sendRSSICmd(true, rssiNtfTimeIntervalInMillisec) ==  STATUS_SUCCESS)) {
            mNfcOperations.setDiscoveryTech(NfcAdapter.FLAG_READER_KEEP,
                    NfcAdapter.FLAG_LISTEN_DISABLE);
            status = STATUS_SUCCESS;
        } else {
            mNfcOperations.enableDiscovery();
        }
        /* check if discovery started */
        if (!mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Not able to start discovery");
            status = ERROR_UNKNOWN;
        }
        return status;
    }

    /**
     * This api is called by applications to stop RSSI mode
     * This api shall be called only after Nfcservice is enabled.
     * @return whether  the update of configuration is
     *          success or not with reason.
     *          0x01  - NFC_IS_OFF,
     *          0x03  - ERROR_UNKNOWN
     *          0x00  - SUCCESS
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    public @FdErrorCode int stopRssiMode() throws IOException {
        int status = ERROR_UNKNOWN;
        NxpNfcLogger.d(TAG, "Entry stopRssiMode");
        if (mNfcOperations == null) {
            NxpNfcLogger.e(TAG, "NFC Operations is null");
            return status;
        }
        if (!mNfcOperations.isEnabled()) {
            return FDSTATUS_ERROR_NFC_IS_OFF;
        }
        if (!isRssiEnabled()) {
            return STATUS_SUCCESS;
        }
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
        /* check if discovery stopped */
        if (mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Not able to stop discovery");
            return status;
        }
        if ((setFieldDetectFlag(false) == STATUS_SUCCESS)
                && (sendRSSICmd(false, 0) ==  STATUS_SUCCESS)) {
            status = STATUS_SUCCESS;
        }
        mNfcOperations.enableDiscovery();
        /* check if discovery started */
        if (!mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Not able to start discovery");
            status = ERROR_UNKNOWN;
        }
        return status;
    }

   /**
     * This api is called by applications to check whether RSSI is enabled or not
     * This api shall be called only after Nfcservice is enabled.
     * @return whether  the feature is enabled(true) disabled (false)
     *          success or not.
     *          Enabled  - true
     *          Disabled - false
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    public boolean isRssiEnabled() throws IOException {
        NxpNfcLogger.d(TAG, "Entry isRssiEnabled");
        if (!isFieldDetectStarted()) {
            return false;
        }
        try {
            byte[] cmdPayload = {0x01, (byte) 0xA1, 0x55};
            mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(), this);
            mNxpNciPacketHandler.shouldCheckResponseSubGid(false);
            byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(CONF_GID,
                    GET_CONF_OID, cmdPayload);
            mNxpNciPacketHandler.shouldCheckResponseSubGid(true);
            if (vendorRsp != null && vendorRsp.length > 0
                    && vendorRsp[0] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                 /* response should be status + len + 2 byte Tag + LV*/
                 if ((vendorRsp.length) >= (cmdPayload.length + 4)) {
                     if (vendorRsp[2] == (byte)0xA1 && vendorRsp[3] == 0x55
                             && vendorRsp[5] == 0x01) {
                         NxpNfcLogger.d(TAG, "RSSI Enabled");
                         return true;
                     }
                 }
            } else {
               NxpNfcLogger.e(TAG, "Send Vendor Failed");
            }
        } catch (Exception e) {
            NxpNfcLogger.d(TAG, "Exception in sendVendorNciMessage ");
            throw new IOException("Exception while sendVendorNciMessage");
        }
        return false;
    }

    /**
     * This api is called by application to enable various debug notigications
     * of NFCC.
     * This api shall be called only if Nfcservice is enabled.
     * @return whether  the update of configuration is
     *          success or not.
     *          0x00 - success
     *          0x01 - NFC is not initialized
     *          0x03 - NFCC command failed
     *          0xFF - Service Unavialable
     */
    public @FdErrorCode int enableDebugNtf(byte fieldValue) {
        NxpNfcLogger.d(TAG, "Entry enableDebugNtf");
        int status = SERVICE_UNAVIABLE;
        if (mNfcOperations == null) {
            NxpNfcLogger.e(TAG, "NFC Operations is null");
            return status;
        }
        if (!mNfcOperations.isEnabled()) {
            return FDSTATUS_ERROR_NFC_IS_OFF;
        }
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
        /* check if discovery started */
        if (mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Not able to stop discovery");
            return status;
        }
        /* As of now, bit0, bit4 and bit5 is allowed by this API */
        byte lxFieldValue = (byte) (fieldValue & L2_DEBUG_BYTE0_MASK);
        byte[] cmdPayload = {0x01, (byte) 0xA0, 0x1D, 0x02, lxFieldValue, 0x00};
        try {
            mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(), this);
            mNxpNciPacketHandler.shouldCheckResponseSubGid(false);
            byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(CONF_GID,
                    SET_CONF_OID, cmdPayload);
            mNxpNciPacketHandler.shouldCheckResponseSubGid(true);
            if (vendorRsp != null && vendorRsp.length > 0
                    && vendorRsp[0] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                status = STATUS_SUCCESS;
            } else {
                NxpNfcLogger.e(TAG, "Send Vendor Failed");
                status = ERROR_UNKNOWN;
            }
        } catch (Exception e) {
            NxpNfcLogger.d(TAG, "Exception in sendVendorNciMessage ");
            status = ERROR_UNKNOWN;
            e.printStackTrace();
        }
        mNfcOperations.enableDiscovery();
        /* check if discovery started */
        if (!mNfcOperations.isDiscoveryStarted()) {
            NxpNfcLogger.e(TAG, "Not able to start discovery");
            status = ERROR_UNKNOWN;
        }
        return status;
    }

    @Override
    public void onVendorNciNotification(int gid, int oid, byte[] payload) {
        if (payload == null || payload.length < 2) {
            NxpNfcLogger.e(TAG, "Invalid payload");
            return;
        }
        NxpNfcLogger.d(TAG, "onVendorNciNotification GID: " +  gid + " OID: " + oid);
        switch(oid) {
            case NCI_OID_SYSTEM_DEBUG_STATE_L1_MESSAGE:
                NxpNfcLogger.d(TAG, "NCI_OID_SYSTEM_DEBUG_STATE_L1_MESSAGE: ");
                break;
            case NCI_OID_SYSTEM_DEBUG_STATE_L2_MESSAGE:
                NxpNfcLogger.d(TAG, "NCI_OID_SYSTEM_DEBUG_STATE_L2_MESSAGE: ");
                if (mLxDebugCallbacks != null) {
                    NxpNfcLogger.d(TAG, "Sending Lx Debug Callback to Application");
                    mLxDebugCallbacks.onLxDebugDataReceived(payload);
                } else {
                    NxpNfcLogger.i(TAG, "No callback registered for Lx Debug");
                }
                if (mContext != null) {
                    NxpNfcLogger.d(TAG, "BroadCasting " + ACTION_LX_DATA_RECVD);
                    Intent lxDataRecvdIntent = new Intent();
                    lxDataRecvdIntent.putExtra("LxDebugCfgs", payload);
                    lxDataRecvdIntent.putExtra("lxDbgDataLen", payload.length);
                    lxDataRecvdIntent.setAction(ACTION_LX_DATA_RECVD);
                    mContext.sendBroadcast(lxDataRecvdIntent);
                }
                break;
            case NCI_OID_SYSTEM_DEBUG_STATE_L3_MESSAGE:
                NxpNfcLogger.d(TAG, "NCI_OID_SYSTEM_DEBUG_STATE_L3_MESSAGE: ");
                break;
            default:
                NxpNfcLogger.d(TAG, "Unknown Notification");
                break;
        }
    }
}
