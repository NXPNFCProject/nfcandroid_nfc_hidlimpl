/*
 * Copyright 2024-2025 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.nxp.nfc;

import android.app.Activity;
import android.nfc.NfcAdapter;
import android.nfc.Tag;

import com.nxp.nfc.vendor.fw.NfcFirmwareInfo;
import com.nxp.nfc.vendor.lxdebug.ILxDebugCallbacks;
import com.nxp.nfc.vendor.lxdebug.LxDebugEventHandler;
import com.nxp.nfc.vendor.srd.ISrdCallbacks;
import com.nxp.nfc.vendor.srd.SrdHandler;
import com.nxp.nfc.vendor.mpos.MposHandler;
import com.nxp.nfc.vendor.qtag.QTagHandler;
import com.nxp.nfc.vendor.transit.TransitConfigHandler;
import com.nxp.nfc.INxpNfcAdapter.SRDStatus.*;
import java.io.IOException;


/**
 * @class NxpNfcAdapter
 * @brief Concrete implementation of NFC Extension features
 *
 */
public final class NxpNfcAdapter implements INxpNfcAdapter {
    private static final String TAG = "NxpNfcAdapter";

    /**
     * @brief singleton instance object
     */
    private static NxpNfcAdapter sNxpNfcAdapter;

    private NfcAdapter mNfcAdapter;
    private MposHandler mMposHandler;
    private QTagHandler mQTagHandler;
    private LxDebugEventHandler mLxDebugEventHandler;
    private TransitConfigHandler mTransitHandler;
    private NfcFirmwareInfo mFwHandler;
    private SrdHandler mSrdHandler;

    /**
     * @brief supported chipsets
     */
    private static final int NXP_EN_SN110U = 1;
    private static final int NXP_EN_SN100U = 1;
    private static final int NXP_EN_SN220U = 1;
    private static final int NXP_EN_PN557 = 1;
    private static final int NXP_EN_PN560 = 1;
    private static final int NXP_EN_SN300U = 1;
    private static final int NXP_EN_SN330U = 1;

    private static final int NFC_NXP_MW_ANDROID_VER = 16; // Android version used by NFC MW
    private static final int NFC_NXP_MW_VERSION_MAJ = 0x06; // MW Major Version
    private static final int NFC_NXP_MW_VERSION_MIN = 0x00; // MW Minor Version
    private static final int NFC_NXP_MW_CUSTOMER_ID = 0x00; // MW Customer ID
    private static final int NFC_NXP_MW_RC_VERSION = 0x00; // MW RC Version
    private static final int NFC_BUSY_IN_MPOS = 0x02;

    private static void printComNxpNfcVersion() {
        int validation = (NXP_EN_SN100U << 13);
        validation |= (NXP_EN_SN110U << 14);
        validation |= (NXP_EN_SN220U << 15);
        validation |= (NXP_EN_PN560 << 16);
        validation |= (NXP_EN_SN300U << 17);
        validation |= (NXP_EN_SN330U << 18);
        validation |= (NXP_EN_PN557 << 11);

        String logMessage = String.format(
            "NxpNfcJar Version: NXP_AR_%02X_%05X_%02d.%02X.%02X",
            NFC_NXP_MW_CUSTOMER_ID, validation, NFC_NXP_MW_ANDROID_VER,
            NFC_NXP_MW_VERSION_MAJ, NFC_NXP_MW_VERSION_MIN);
        NxpNfcLogger.d(TAG, logMessage);
    }

    /**
     * @brief private constructor to create the instance of {@link NxpNfcAdapter}
     * @param nfcAdapter
     */
    private NxpNfcAdapter(NfcAdapter nfcAdapter) {
        printComNxpNfcVersion();
        mNfcAdapter = nfcAdapter;
        mMposHandler = new MposHandler(nfcAdapter);
        mQTagHandler = new QTagHandler(nfcAdapter);
        mLxDebugEventHandler = new LxDebugEventHandler(nfcAdapter);
        mTransitHandler = new TransitConfigHandler(nfcAdapter);
        mFwHandler = new NfcFirmwareInfo(nfcAdapter);
        mSrdHandler = new SrdHandler(nfcAdapter);
    }

    public interface NxpReaderCallback {
      void onNxpTagDiscovered(Tag tag, boolean isNxpTagDetected);
    }

    /**
     * @brief NxpNfcAdapter for application context,
     * or throws UnsupportedOperationException nfcAdapter is null.
     *
     * @param nfcAdapter
     * @return {@link NxpNfcAdapter} instance
     */
    public static synchronized NxpNfcAdapter getNxpNfcAdapter(NfcAdapter nfcAdapter) {
        if (sNxpNfcAdapter == null) {
            if (nfcAdapter == null) {
                NxpNfcLogger.e(TAG, "nfcAdapter is null");
                throw new UnsupportedOperationException();
            }
            sNxpNfcAdapter = new NxpNfcAdapter(nfcAdapter);
        }
        return sNxpNfcAdapter;
    }

    /**
     * @brief getter for accessing {@link INxpNfcAdapter}
     * make sure to call {@link #getNxpNfcAdapter()} before calling this
     * throws UnsupportedOperationException {@link #sNxpNfcAdapter} is null.
     * @return {@link INxpNfcAdapter} instance
     */
    public static INxpNfcAdapter getNxpNfcAdapterInterface() {
        if (sNxpNfcAdapter == null) {
            throw new UnsupportedOperationException(
                "You need a reference from NxpNfcAdapter to use the "
                + " NXP NFC APIs");
        }
        return ((INxpNfcAdapter) sNxpNfcAdapter);
    }

    /**
     * @brief To be called to start or stop the mPOS reader mode
     * @return {@link INxpNfcAdapter.mPOSSetReaderMode} instance
     */
    @Override
    public int mPOSSetReaderMode(String pkg, boolean on) throws IOException {
      return mMposHandler.mPOSSetReaderMode(pkg, on);
    }

    /**
     * @brief This is provides the info whether mPOS mode is activated or not
     * @return {@link INxpNfcAdapter.mPOSGetReaderMode} instance
     */
    @Override
    public boolean mPOSGetReaderMode(String pkg) throws IOException {
      return mMposHandler.mPOSGetReaderMode(pkg);
    }

    /**
     * @brief To be called to enable QTag
     * @return {@link INxpNfcAdapter.enableQTag} instance
     */
    @Override
    public int enableQTag(Activity activity, NxpReaderCallback mQTagCallback,
                          int mode, int pollTech, int delay_value)
        throws IOException {
      return mQTagHandler.enableQTag(activity, mode, mQTagCallback, pollTech,
                                     delay_value);
    }

    /**
     * @brief To be called to set NCI configuration
     * @return {@link INxpNfcAdapter.mTransitHandler} instance
     */
    @Override
    public boolean setConfig(String configs) throws IOException {
      return mTransitHandler.setConfig(configs);
    }

    /**
     * @brief This api is called to get current FW version.
     * @return {@link INxpNfcAdapter.NfcFirmwareInfo} instance
     */
    @Override
    public byte[] getFwVersion() throws IOException {
        return mFwHandler.getFwVersion();
    }

    /**
     * This API registers the callback to get Field Detected Events.
     * @param callbacks : callback object to be register.
     */
    @Override
    public void registerLxDebugCallbacks(ILxDebugCallbacks callbacks) {
        mLxDebugEventHandler.registerLxDebugCallbacks(callbacks);
    }

    /**
     * This API unregisters the Application callbacks to be called
     * for LxDebug notifications.
     */
    @Override
    public void unregisterLxDebugCallbacks() {
        mLxDebugEventHandler.unregisterLxDebugCallbacks();
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
    @Override
    public int startExtendedFieldDetectMode(int detectionTimeout) {
        return mLxDebugEventHandler.startExtendedFieldDetectMode(detectionTimeout);
    }

    /**
     * @return status     :-0x00 :EFDSTATUS_SUCCESS
     *                      0x01 :EFDSTATUS_FAILED
     *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
     *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
     *                      0x07 :EFDSTATUS_ERROR_NOT_STARTED
     */
    @Override
    public int stopExtendedFieldDetectMode() {
        return mLxDebugEventHandler.stopExtendedFieldDetectMode();
    }

    /**
     * This API starts card emulation mode. Starts RF Discovery with Default
     * POLL & Listen configurations
     * @return status     :-0x00 :EFDSTATUS_SUCCESS
     *                      0x01 :EFDSTATUS_FAILED
     *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
     *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
     */
    @Override
    public int startCardEmulation() {
        return mLxDebugEventHandler.startCardEmulation();
    }

    /**
     * This api is called by applications enable or disable field
     * detect feauture.
     * This api shall be called only Nfcservice is enabled.
     * @param  mode to Enable(true) and Disable(false)
     * @return whether  the update of configuration is
     *          success or not with reason.
     *          0x01  - NFC_IS_OFF,
     *          0x02  - NFC_BUSY_IN_MPOS
     *          0x03  - ERROR_UNKNOWN
     *          0x00  - SUCCESS
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    @Override
    public int setFieldDetectMode(boolean mode) throws IOException {
        if (mPOSGetReaderMode(TAG)) {
            NxpNfcLogger.e(TAG, "NFC Busy in MPOS");
            return NFC_BUSY_IN_MPOS;
        }
        return mLxDebugEventHandler.setFieldDetectMode(mode);
    }

    /**
     * detect feature is enabled or not
     * @return whether  the feature is enabled(true) disabled (false)
     *          success or not.
     *          Enabled  - true
     *          Disabled - false
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    @Override
    public boolean isFieldDetectEnabled() {
        return mLxDebugEventHandler.isFieldDetectEnabled();
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
     *          0x02  - NFC_BUSY_IN_MPOS
     *          0x03  - ERROR_UNKNOWN
     *          0x00  - SUCCESS
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    @Override
    public int startRssiMode(int rssiNtfTimeIntervalInMillisec) throws IOException {
        if (mPOSGetReaderMode(TAG)) {
            NxpNfcLogger.e(TAG, "NFC Busy in MPOS");
            return NFC_BUSY_IN_MPOS;
        }
        return mLxDebugEventHandler.startRssiMode(rssiNtfTimeIntervalInMillisec);
    }

    /**
     * This api is called by applications to stop RSSI mode
     * This api shall be called only after Nfcservice is enabled.
     * @return whether  the update of configuration is
     *          success or not with reason.
     *          0x01  - NFC_IS_OFF,
     *          0x02  - NFC_BUSY_IN_MPOS
     *          0x03  - ERROR_UNKNOWN
     *          0x00  - SUCCESS
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    @Override
    public int stopRssiMode() throws IOException {
        if (mPOSGetReaderMode(TAG)) {
            NxpNfcLogger.e(TAG, "NFC Busy in MPOS");
            return NFC_BUSY_IN_MPOS;
        }
        return mLxDebugEventHandler.stopRssiMode();
    }

    /**
     * This api is called by applications to check whether RSSI is enabled or not
     * @return whether  the feature is enabled(true) disabled (false)
     *          success or not.
     *          Enabled  - true
     *          Disabled - false
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    @Override
    public boolean isRssiEnabled() throws IOException {
        return mLxDebugEventHandler.isRssiEnabled();
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
    @Override
    public int enableDebugNtf(byte fieldValue) {
        return mLxDebugEventHandler.enableDebugNtf(fieldValue);
    }
    /**
     * This API registers the callback to SRD Events.
     * @param callbacks : callback object to be register.
     */
    @Override
    public void registerSrdCallbacks(ISrdCallbacks callbacks) {
        mSrdHandler.registerSrdCallbacks(callbacks);
    }

    /**
     * This API unregisters the Application callbacks to be called
     * for SRD notifications.
     */
    @Override
    public void unregisterSrdCallbacks() {
        mSrdHandler.unregisterSrdCallbacks();
    }
    /**
     * @brief To be called to start or stop the srd mode
     * @return {@link INxpNfcAdapter.setSRDMode} instance
     */
    @Override
    public @SRDStatus int setSRDMode(boolean on) throws IOException{
      return mSrdHandler.setSRDMode(on);
    }
}
