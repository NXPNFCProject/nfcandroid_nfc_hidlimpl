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

package com.nxp.wlc;

import android.app.Activity;
import android.content.Context;
import android.nfc.NfcAdapter;
import android.nfc.Tag;

import com.nxp.wlc.vendor.fw.NfcFirmwareInfo;
import com.nxp.wlc.vendor.lxdebug.ILxDebugCallbacks;
import com.nxp.wlc.vendor.lxdebug.LxDebugEventHandler;
import com.nxp.wlc.vendor.wlcinterface.IWlcEventCallbacks;
import com.nxp.wlc.vendor.wlcinterface.WlcEventHandler;
import com.nxp.wlc.vendor.utils.UtilsHandler;
import android.os.RemoteException;
import android.util.Log;
import java.io.IOException;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
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

    /**
     * @brief Reflection variables for loading {@link NxpNfcExtentions}
     */
    private Class mNxpNfcExtentionsClass;
    private Object mNxpNfcExtentionsObj;

    private NfcAdapter mNfcAdapter;
    private LxDebugEventHandler mLxDebugEventHandler;
    private WlcEventHandler mWlcEventHandler;
    private NfcFirmwareInfo mFwHandler;
    private UtilsHandler mUtilsHandler;

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
    private static final int NFC_NXP_MW_VERSION_MAJ = 0x50; // MW Major Version
    private static final int NFC_NXP_MW_VERSION_MIN = 0x00; // MW Minor Version
    private static final int NFC_NXP_MW_CUSTOMER_ID = 0x00; // MW Customer ID
    private static final int NFC_NXP_MW_RC_VERSION = 0x00; // MW RC Version

    private static void printComNxpNfcVersion() {
        int validation = (NXP_EN_SN100U << 13);
        validation |= (NXP_EN_SN110U << 14);
        validation |= (NXP_EN_SN220U << 15);
        validation |= (NXP_EN_PN560 << 16);
        validation |= (NXP_EN_SN300U << 17);
        validation |= (NXP_EN_SN330U << 18);
        validation |= (NXP_EN_PN557 << 11);

        String logMessage = String.format(
            "NxpWlcJar Version: NXP_AR_%02X_%05X_%02d.%02X.%02X",
            NFC_NXP_MW_CUSTOMER_ID, validation, NFC_NXP_MW_ANDROID_VER,
            NFC_NXP_MW_VERSION_MAJ, NFC_NXP_MW_VERSION_MIN);
        NxpNfcLogger.d(TAG, logMessage);
    }

    /**
     * @brief private constructor to create the instance of {@link NxpNfcAdapter}
     * @param nfcAdapter
     * @param context
     */
    private NxpNfcAdapter(NfcAdapter nfcAdapter, Context context) {
        printComNxpNfcVersion();
        mNfcAdapter = nfcAdapter;
        mLxDebugEventHandler = new LxDebugEventHandler(nfcAdapter, context);
        mWlcEventHandler =  new WlcEventHandler(nfcAdapter, context);
        mFwHandler = new NfcFirmwareInfo(nfcAdapter);
        mUtilsHandler = new UtilsHandler(nfcAdapter, context);
    }

    /**
     * @brief method to print {@link INxpNfcAdapter} declared methods
     * @return None
     */
    private void logExtentionsInterface() {
      Method[] methods = mNxpNfcExtentionsClass.getDeclaredMethods();
      NxpNfcLogger.d(TAG, "Total methods:" + methods.length);
      for (Method method : methods) {
        NxpNfcLogger.d(TAG, "Method: " + method.getName());
      }
    }

    public interface NxpReaderCallback {
      void onNxpTagDiscovered(Tag tag, boolean isNxpTagDetected);
    }

    /**
     * @brief NxpNfcAdapter for application context,
     * or throws UnsupportedOperationException nfcAdapter is null.
     *
     * @param nfcAdapter
     * @param context
     * @return {@link NxpNfcAdapter} instance
     */
    public static synchronized NxpNfcAdapter getNxpNfcAdapter(NfcAdapter nfcAdapter, Context context) {
        if (sNxpNfcAdapter == null) {
            if (nfcAdapter == null) {
                NxpNfcLogger.e(TAG, "nfcAdapter is null");
                throw new UnsupportedOperationException();
            }
            sNxpNfcAdapter = new NxpNfcAdapter(nfcAdapter, context);
        }
        return sNxpNfcAdapter;
    }

    /**
     * @brief NxpNfcAdapter for application context,
     * or throws UnsupportedOperationException nfcAdapter is null.
     *
     * @param nfcAdapter
     * @return {@link NxpNfcAdapter} instance
     */
    public static synchronized NxpNfcAdapter getNxpNfcAdapter(NfcAdapter nfcAdapter) {
        return getNxpNfcAdapter(nfcAdapter, null);
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
     * This API registers the callback to get WLC_STATUS_NTF & WPT DATA Events.
     * @param callbacks : callback object to be register.
     */
    @Override
    public void registerWlcCallbacks(IWlcEventCallbacks callbacks) {
        mWlcEventHandler.registerWlcCallbacks(callbacks);
    }

    /**
     * This API unregisters the Application callbacks to be called
     * for WLC Status & Data notifications.
     */
    @Override
    public void unregisterWlcCallbacks() {
        mWlcEventHandler.unregisterWlcCallbacks();
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
      return mLxDebugEventHandler.startCardEmulation(0x00);
    }

    /**
     * This API starts card emulation mode. Starts RF Discovery with Default
     * POLL configurations and sets the Listen tech paramaters.
     * @param listenTech Flags indicating listen technologies.
     * @return status     :-0x00 :EFDSTATUS_SUCCESS
     *                      0x01 :EFDSTATUS_FAILED
     *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
     *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
     */
    @Override
    public int startCardEmulation(int listenTech) {
      return mLxDebugEventHandler.startCardEmulation(listenTech);
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
    @Override
    public int setFieldDetectMode(boolean mode) throws IOException {
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
     *          0x03  - ERROR_UNKNOWN
     *          0x00  - SUCCESS
     * @throws  IOException if any exception occurs during setting the NFC configuration.
     */
    @Override
    public int startRssiMode(int rssiNtfTimeIntervalInMillisec) throws IOException {
        return mLxDebugEventHandler.startRssiMode(rssiNtfTimeIntervalInMillisec);
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
    @Override
    public int stopRssiMode() throws IOException {
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
     * @deprecated This api is called by application to enable various debug notigications
     * of NFCC.
     * This api shall be called only if Nfcservice is enabled.
     * @return whether  the update of configuration is
     *          success or not.
     *          0x00 - success
     *          0x01 - NFC is not initialized
     *          0x03 - NFCC command failed
     *          0xFF - Service Unavialable
     */
    @Deprecated
    @Override
    public int enableDebugNtf(byte fieldValue) {
        return mLxDebugEventHandler.enableDebugNtf(fieldValue);
    }
    /**
     * This api is called by application to enable various debug notigications
     * of NFCC.
     * This api shall be called only if Nfcservice is enabled.
     * @param fieldValue : bytes to be set for lxdebug config.
     * @return whether  the update of configuration is
     *          success or not.
     *          0x00 - success
     *          0x01 - NFC is not initialized
     *          0x03 - NFCC command failed
     *          0x04 - Invalid argument In case of fieldValue
     *                 is not 2 bytes.
     *          0xFF - Service Unavialable
     */
    @Override
    public int enableDebugNtf(byte[] fieldValue) {
        return mLxDebugEventHandler.enableDebugNtf(fieldValue);
    }

    /**
     * This API is called by application to stop RF discovery
     * <p>Requires {@link android.Manifest.permission#NFC} permission.
     * <li>This api shall be called only Nfcservice is enabled.
     * </ul>
     * @param  mode
     *         LOW_POWER
     *         ULTRA_LOW_POWER
     * @return None
     * @throws IOException If a failure occurred during stop discovery
    */
    public void stopPoll(int mode) throws IOException {
        mUtilsHandler.stopPoll();
    }

    /**
     * This API is called by application to start RF discovery
     * <p>Requires {@link android.Manifest.permission#NFC} permission.
     * <li>This api shall be called only Nfcservice is enabled.
     * </ul>
     * @return None
     * @throws IOException If a failure occurred during start discovery
    */
    public void startPoll() throws IOException {
        mUtilsHandler.startPoll();
    }
}
