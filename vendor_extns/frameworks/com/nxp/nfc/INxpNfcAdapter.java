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

import android.annotation.IntDef;
import android.app.Activity;
import com.nxp.nfc.NxpNfcAdapter.AutoCardStatusCallback;
import com.nxp.nfc.NxpNfcAdapter.NxpReaderCallback;
import com.nxp.nfc.vendor.lxdebug.ILxDebugCallbacks;
import com.nxp.nfc.vendor.srd.ISrdCallbacks;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * @class INxpNfcAdapter
 * @brief Interface to perform the NFC Extension functionality.
 *
 * @hide
 */
public interface INxpNfcAdapter {


  /**
   * @brief gets the NxpNfc Extensions interface
   *
   * @return NxpNfc Extensions interface
   */
  INxpNfcExtensions getNxpNfcExtensionsInterface();
  /**
   * This is the first API to be called to start or stop the mPOS mode
   * <ul>
   * <li>This api shall be called only NfcService is enabled.
   * <li>This api shall be called only when there are no NFC transactions
   * ongoing
   * </ul>
   * @param  pkg package name of the caller
   * @param  on Sets/Resets the mPOS state.
   * @return whether the update of state is
   *          MPOS_STATUS_SUCCESS,
   *          MPOS_STATUS_FAILED,
   * @throws IOException If a failure occurred during reader mode set or reset
   */
  public int mPOSSetReaderMode(String pkg, boolean on) throws IOException;

  /**
   * This is provides the info whether mPOS mode is activated or not
   * <ul>
   * <li>This api shall be called only NfcService is enabled.
   * <li>This api shall be called only when there are no NFC transactions
   * ongoing
   * </ul>
   * @param  pkg package name of the caller
   * @return TRUE if reader mode is started
   *         FALSE if reader mode is not started
   * @throws IOException If a failure occurred during reader mode set or reset
   */
  public boolean mPOSGetReaderMode(String pkg) throws IOException;

  /**
   * Possible status from {@link #setAutocard}.
   *
   */
  public static final int EACSTATUS_ERROR_REJECTED = 0x01;
  public static final int EACSTATUS_SYNTAX_ERROR = 0x05;
  public static final int EACSTATUS_SEMANTIC_ERROR = 0x06;
  public static final int EACSTATUS_ERROR_NFC_IS_OFF = 0x07;
  public static final int EACSTATUS_ERROR_FEATURE_NOT_SUPPORTED = 0x0D;
  public static final int EACSTATUS_ERROR_FEATURE_NOT_CONFIGURED = 0x0C;
  public static final int EACSTATUS_ERROR_FEATURE_DISABLED_IN_CONFIG = 0x0B;
  public static final int EACSTATUS_ERROR_INVALID_PARAM = 0x0E;

  public static final int STATUS_SUCCESS = 0x00;
  public static final int STATUS_FAILED = 0x03;

  @IntDef(value =
              {
                  STATUS_SUCCESS,
                  STATUS_FAILED,
                  EACSTATUS_SYNTAX_ERROR,
                  EACSTATUS_SEMANTIC_ERROR,
                  EACSTATUS_ERROR_REJECTED,
                  EACSTATUS_ERROR_FEATURE_NOT_SUPPORTED,
                  EACSTATUS_ERROR_FEATURE_NOT_CONFIGURED,
                  EACSTATUS_ERROR_FEATURE_DISABLED_IN_CONFIG,
                  EACSTATUS_ERROR_NFC_IS_OFF,
                  EACSTATUS_ERROR_INVALID_PARAM,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface AutoCardStatus {}

  /**
   * This API get Autocard AID's  to NFCC using the vendor NCI message
   * <li>This api shall be called only NfcService is enabled.
   * @return status     :-0x00 :SUCCESS
   *                      0x01 - 0x06: NCI Status Codes
   *                           : Refer NCI spec v2.3 Table 140
   *                      0x07 : NFC off
   *                      0x0B : Disabled
   *                      0x0C : Config not defined
   *                      0x0D : Feature not supported by platform
   *                      0x0E : EACSTATUS_ERROR_MESSAGE_CORRUPTED
   * byte[0] indicates status, byte[1] onwards AID in TLV format.
   * Number of Aid's     : 1 Byte
   * Protocol type.      : 1 Byte
   * Length of AID       : 1 Byte
   * Aid Entry           : AID (x Bytes supported 5 <= x <= 16)
   * <p>Requires {@link   android.Manifest.permission#NFC} permission.
   */
  public byte[] getAutoCardAID() throws IOException;

  /**
   * This API sends Autocard AID's  to NFCC using the vendor NCI message
   * <ul>
   * <li>This api shall be called only when NfcService is enabled and also
   * <li>autocard feature should be enabled in NFCC.
   * </ul>
   * @param aids  No of AID's to configure.
   * @return status     :-0x00 :SUCCESS
   *                      0x01 - 0x06: NCI Status Codes
   *                           : Refer NCI spec v2.3 Table 140
   *                      0x07 : NFC off
   *                      0x0B : Disabled
   *                      0x0C : Config not defined
   *                      0x0D : Feature not supported by platform
   *                      0x0E : EACSTATUS_ERROR_MESSAGE_CORRUPTED
   * <p>Requires {@link   android.Manifest.permission#NFC} permission.
   */
  public @AutoCardStatus int setAutoCardAID(byte[] aids) throws IOException;

  /**
   * This API get Autocard AID RF parameters from NFCC using the vendor NCI cmd
   * <li>This api shall be called only NfcService is enabled.
   * @return status     :-0x00 :SUCCESS
   *                      0x01 - 0x06: NCI Status Codes
   *                           : Refer NCI spec v2.3 Table 140
   *                      0x07 : NFC off
   *                      0x0B : Disabled
   *                      0x0C : Config not defined
   *                      0x0D : Feature not supported by platform
   *                      0x0E : EACSTATUS_ERROR_MESSAGE_CORRUPTED
   * byte[0] array  : RF params <aid-0 index> <19 bytes data> ...
   *                : <aid-n index> <19 bytes data>
   * <p>Requires {@link   android.Manifest.permission#NFC} permission.
   */
  public byte[] getAutoCardRfParams() throws IOException;

  /**
   * This API sends Autocard AID RF parameters to NFCC using the vendor NCI cmd
   * <ul>
   * <li>This api shall be called only when NfcService is enabled and also
   * <li>autocard feature should be enabled in NFCC.
   * </ul>
   * @param aids  No of AID's to configure.
   * @return status     :-0x00 :SUCCESS
   *                      0x01 - 0x06: NCI Status Codes
   *                           : Refer NCI spec v2.3 Table 140
   *                      0x07 : NFC off
   *                      0x0B : Disabled
   *                      0x0C : Config not defined
   *                      0x0D : Feature not supported by platform
   *                      0x0E : EACSTATUS_ERROR_MESSAGE_CORRUPTED
   * <p>Requires {@link   android.Manifest.permission#NFC} permission.
   */
  public @AutoCardStatus int setAutoCardRfParams(byte[] aids)
      throws IOException;

  /**
   * This API to enable Autocard feature in NFCC using the vendor NCI message
   * <ul>
   * <li>This api shall be called only NfcService is enabled.
   * @return status     :-0x00 :SUCCESS
   *                      0x01 - 0x06: NCI Status Codes
   *                           : Refer NCI spec v2.3 Table 140
   *                      0x07 : NFC off
   *                      0x0B : Disabled
   *                      0x0C : Config not defined
   *                      0x0D : Feature not supported by platform
   *                      0x0E : EACSTATUS_ERROR_MESSAGE_CORRUPTED
   * <p>Requires {@link   android.Manifest.permission#NFC} permission.
   */
  public @AutoCardStatus
  int enableAutoCard(AutoCardStatusCallback mAutoCardStatusCallback)
      throws IOException;

  /**
   * This API to disable Autocard feature in NFCC using the vendor NCI message.
   * <ul>
   * <li>This api shall be called only NfcService is enabled.
   * @return status     :-0x00 :SUCCESS
   *                      0x01 - 0x06: NCI Status Codes
   *                           : Refer NCI spec v2.3 Table 140
   *                      0x07 : NFC off
   *                      0x0B : Disabled
   *                      0x0C : Config not defined
   *                      0x0D : Feature not supported by platform
   *                      0x0E : EACSTATUS_ERROR_MESSAGE_CORRUPTED
   * <p>Requires {@link   android.Manifest.permission#NFC} permission.
   */
  public @AutoCardStatus int disableAutoCard() throws IOException;

  /**
   * This API sets Autocard AID's status to NFCC using the vendor NCI message
   * <ul>
   * <li>This api shall be called only when NfcService is enabled and also
   * <li>autocard feature should be enabled in NFCC.
   * </ul>
   * @param appletStatus  Update status of configured AID's.
   * @return status     :-0x00 :SUCCESS
   *                      0x01 - 0x06: NCI Status Codes
   *                           : Refer NCI spec v2.3 Table 140
   *                      0x07 : NFC off
   *                      0x0B : Disabled
   *                      0x0C : Config not defined
   *                      0x0D : Feature not supported by platform
   *                      0x0E : EACSTATUS_ERROR_MESSAGE_CORRUPTED
   * <p>Requires {@link   android.Manifest.permission#NFC} permission.
   */
  public @AutoCardStatus int setAutoCardAppletStatus(byte[] appletStatus)
      throws IOException;

  /**
   * This API sends suspend/resume cmd to NFCC using the vendor NCI message
   * <ul>
   * <li>This api shall be called only when NfcService is enabled and also
   * <li>autocard feature should be enabled in NFCC.
   * </ul>
   * @param flag  true/false.
   * @return status     :-0x00 :SUCCESS
   *                      0x01 - 0x06: NCI Status Codes
   *                           : Refer NCI spec v2.3 Table 140
   *                      0x07 : NFC off
   *                      0x0B : Disabled
   *                      0x0C : Config not defined
   *                      0x0D : Feature not supported by platform
   * <p>Requires {@link   android.Manifest.permission#NFC} permission.
   */
  public @AutoCardStatus int suspendAutoCard(boolean flag) throws IOException;

  /**
   * This is the API to be called to enable or disable QTag RF mode.
   * <ul>
   * <li>This api shall be called only when NfcService is enabled.
   * <li>This api shall be called only when there are no NFC transactions
   * ongoing.
   * <li>Limit the NFC controller to reader mode while this Activity is in the
   * foreground.
   * </ul>
   * @param  activity activity the Activity that requests the adapter to be in
   *     reader mode.
   * @param mQTagCallback the callback to be called when a tag is discovered
   * @param  mode to ENABLE_QTAG_ONLY_MODE.
   *                 APPEND_QTAG_MODE with input pollTech.
   *                 DISABLE_QTAG_MODE & reset to default discovery.
   * @param pollTech to append QPoll in reader mode.
   * @param delay_value PRESENCE_CHECK_DELAY
   * @return whether the update of state is
   *          QTag_STATUS_SUCCESS,
   *          QTag_STATUS_FAILED,
   * @throws IOException If a failure occurred during QTag RF mode set or reset
   */
  public int enableQTag(Activity activity, NxpReaderCallback mQTagCallback,
                        int mode, int pollTech, int delay_value)
      throws IOException;

  /**
     * This api is called by applications to update the NFC configurations which
     * are already part of libnfc-nci.conf <p>Requires
     * {@link android.Manifest.permission#NFC} permission.
     * <ul>
     * <li>This api shall be called only NfcService is enabled.
     * <li>This api shall be called only when there are no NFC transactions ongoing
     * </ul>
     * @param  configs NFC Configuration to be updated.
     * @apiNote String should have KEY and assigned with Value in Hexadecimal.
     * Refer below sample.
     * POLLING_TECH_MASK=0x4F
     * @return whether  the update of configuration is
     *          success or not.
     *          0xFF - failure
     *          0x00 - success
     * @throws  IOException if any exception occurs during setting the NFC
     * configuration.
     */
  public boolean setConfig(String configs) throws IOException;

  /**
   * This api is called to get current FW version.
   * @return byte array of fwVersion
   *         fwVersion byte array of length 3 - Success
   *            byte[0] - Major version
   *            byte[1] - Minor version
   *            byte[2] - Rom version
   *         fwVersion byte array with all bytes 0x00 - Failure
   */
  public byte[] getFwVersion() throws IOException;

  /**
   * This API registers the callback to get Field Detected Events.
   * @param callbacks : callback object to be register.
   */
  void registerLxDebugCallbacks(ILxDebugCallbacks callbacks);

  /**
   * This API unregisters the Application callbacks to be called
   * for LxDebug notifications.
   */
  void unregisterLxDebugCallbacks();

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
  int startExtendedFieldDetectMode(int detectionTimeout);

  /**
   * @return status     :-0x00 :EFDSTATUS_SUCCESS
   *                      0x01 :EFDSTATUS_FAILED
   *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
   *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
   *                      0x07 :EFDSTATUS_ERROR_NOT_STARTED
   */
  int stopExtendedFieldDetectMode();

  /**
   * This API starts card emulation mode. Starts RF Discovery with Default
   * POLL & Listen configurations
   * @return status     :-0x00 :EFDSTATUS_SUCCESS
   *                      0x01 :EFDSTATUS_FAILED
   *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
   *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
   */
  int startCardEmulation();

  /**
   * This API starts card emulation mode. Starts RF Discovery with Default
   * POLL configurations and sets the Listen tech parameters.
   * @param listenTech Flags indicating listen technologies.
   * @return status     :-0x00 :EFDSTATUS_SUCCESS
   *                      0x01 :EFDSTATUS_FAILED
   *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
   *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
   */
  int startCardEmulation(int listenTech);

  /**
   * This api is called by applications enable or disable field
   * detect feature.
   * This api shall be called only NfcService is enabled.
   * @param  mode to Enable(true) and Disable(false)
   * @return whether  the update of configuration is
   *          success or not with reason.
   *          0x01  - NFC_IS_OFF,
   *          0x02  - NFC_BUSY_IN_MPOS
   *          0x03  - ERROR_UNKNOWN
   *          0x00  - SUCCESS
   * @throws  IOException if any exception occurs during setting the NFC configuration.
   */
  int setFieldDetectMode(boolean mode) throws IOException;

  /**
   * detect feature is enabled or not
   * @return whether  the feature is enabled(true) disabled (false)
   *          success or not.
   *          Enabled  - true
   *          Disabled - false
   * @throws  IOException if any exception occurs during setting the NFC configuration.
   */
  boolean isFieldDetectEnabled();

  /**
   * This api is called by applications to start RSSI mode.
   * Once RSSI is enabled, RSSI data notifications are broadcasted to registered
   * application when the device is in the reader field. Application can then
   * analyze this data and find best position for transaction.
   * This api shall be called only after NfcService is enabled.
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
  int startRssiMode(int rssiNtfTimeIntervalInMillisec) throws IOException;

  /**
   * This api is called by applications to stop RSSI mode
   * This api shall be called only after NfcService is enabled.
   * @return whether  the update of configuration is
   *          success or not with reason.
   *          0x01  - NFC_IS_OFF,
   *          0x02  - NFC_BUSY_IN_MPOS
   *          0x03  - ERROR_UNKNOWN
   *          0x00  - SUCCESS
   * @throws  IOException if any exception occurs during setting the NFC configuration.
   */
  int stopRssiMode() throws IOException;

  /**
   * This api is called by applications to check whether RSSI is enabled or not
   * @return whether  the feature is enabled(true) disabled (false)
   *          success or not.
   *          Enabled  - true
   *          Disabled - false
   * @throws  IOException if any exception occurs during setting the NFC configuration.
   */
  boolean isRssiEnabled() throws IOException;

  /**
   * @deprecated This api is called by application to enable various debug notigications
   * of NFCC.
   * This api shall be called only if NfcService is enabled.
   *
   * @return whether  the update of configuration is
   *          success or not.
   *          0x00 - success
   *          0x01 - NFC is not initialized
   *          0x03 - NFCC command failed
   *          0xFF - Service Unavialable
   */
  @Deprecated
  int enableDebugNtf(byte fieldValue);
  /**
   * This api is called by application to enable various debug notigications
   * of NFCC.
   * This api shall be called only if NfcService is enabled.
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
  int enableDebugNtf(byte[] fieldValue);
  /**
   * This API registers the callback to get SRD Timeout Events.
   * @param callbacks : callback object to be register.
  /**
   * This API registers the callback to get SRD Timeout Events.
   * @param callbacks : callback object to be register.
   */
  void registerSrdCallbacks(ISrdCallbacks callbacks);

  /**
   * This API unregisters the Application callbacks to be called
   * for SRD Timeout notifications.
   */
  void unregisterSrdCallbacks();
  public static final int SRD_STATUS_SUCCESS = 0;
  public static final int SRD_INPROGESS = 1;
  public static final int SRD_STATUS_FAILED = 2;
    /**
     * Possible status from {@link #setSRDMode}.
     *
     */
    @IntDef(prefix = { "STATUS_" }, value = {
            SRD_STATUS_SUCCESS,
            SRD_INPROGESS,
            SRD_STATUS_FAILED,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface SRDStatus{}

}
