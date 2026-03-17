/*
 * Copyright 2024-2026 NXP
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

import android.annotation.IntDef;
import android.app.Activity;
import com.nxp.wlc.NxpNfcAdapter.NxpReaderCallback;
import com.nxp.wlc.vendor.lxdebug.ILxDebugCallbacks;
import com.nxp.wlc.vendor.wlcinterface.IWlcEventCallbacks;
import com.nxp.wlc.vendor.wlcinterface.WlcEventHandler;
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

  /**
   * This api is called to get current FW version.
   * @return byte array of fwVersion
   *         fwVersion byte array of length 3 - Suceess
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
   * This API registers the callback to get WLC_STATUS_NTF & WPT DATA Events.
   * @param callbacks : callback object to be register.
   */
  public void registerWlcCallbacks(IWlcEventCallbacks callbacks);

  /**
   * This API sends command to get the observables data.
   * Command is accepted if at least one set of observables is available,
   * it will be rejected otherwise.
   *
   * @retuns byte array of Response in a TLV format
   */
  public byte[] getObservables();

  /**
   * This API unregisters the Application callbacks to be called
   * for WLC Status & WPT Data Event notifications.
   */
  void unregisterWlcCallbacks();

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
   * POLL configurations and sets the Listen tech paramaters.
   * @param listenTech Flags indicating listen technologies.
   * @return status     :-0x00 :EFDSTATUS_SUCCESS
   *                      0x01 :EFDSTATUS_FAILED
   *                      0x05 :EFDSTATUS_ERROR_NFC_IS_OFF
   *                      0x06 :EFDSTATUS_ERROR_UNKNOWN
   */
  int startCardEmulation(int listenTech);

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
  int startRssiMode(int rssiNtfTimeIntervalInMillisec) throws IOException;

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
   * This api shall be called only if Nfcservice is enabled.
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
  int enableDebugNtf(byte[] fieldValue);
}
