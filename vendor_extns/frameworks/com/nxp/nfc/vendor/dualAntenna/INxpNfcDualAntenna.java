/*
 * Copyright 2025 NXP
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

package com.nxp.nfc.vendor.dualAntenna;

import android.annotation.IntDef;
import android.app.Activity;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * @class INxpNfcDualAntenna
 * @brief Interface to perform the NFC DualAntenna functionality.
 *
 * @hide
 */

public interface INxpNfcDualAntenna {

  public static final int STATUS_SUCCESS = 0x00;
  public static final int STATUS_FAILED = 0x01;

  public static final int ENABLE_READERMODE = 0x01;
  public static final int DISABLE_READERMODE = 0x00;

  @IntDef(value =
              {
                  ENABLE_READERMODE,
                  DISABLE_READERMODE,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface ReaderModeStatus {}

  @IntDef(value =
              {
                  STATUS_SUCCESS,
                  STATUS_FAILED,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface DualAntennaStatus {}

  /**
   * This is the API to be called to check Dual Antenna feature is supported or
   * not. <li>This api shall be called only when NfcService is enabled. <li>This
   * api shall be called only when there are no NFC transactions ongoing.
   * </ul>
   * @throws IOException If a failure occurred
   */
  public boolean isDualAnetannaSupported() throws IOException;
  /**
   * This is the API to be called to configure the antenna's.
   * <li>This api shall be called only when NfcService is enabled.
   * <li>This api shall be called only when there are no NFC transactions
   * ongoing.
   * </ul>
   * @param  tech1 , To configure antenna 1 with given polling configuration
   * @param  tech2 , To configure antenna 2 with given polling configuration
   * @return whether the update of state is
   *          ENABLE_DISABLE_STATUS_SUCCESS,
   *          ENABLE_DISABLE_STATUS_FAILED,
   * @throws IOException If a failure occurred during enable/disable the feature
   */
  public @DualAntennaStatus int setDiscoveryTechnology_DualAntenna(int tech1,
                                                                   int tech2)
      throws IOException;

  /**
   * This is the API to be called to enable reader mode on either antenna's.
   * <li>This api shall be called only when NfcService is enabled.
   * <li>This api shall be called only when there are no NFC transactions
   * ongoing.
   * </ul>
   * @param  ant1 , To configure readerMode in antenna1
   * @param  ant2 , To configure readerMode in antenna2
   * @return whether the update of state is
   *          ENABLE_DISABLE_STATUS_SUCCESS,
   *          ENABLE_DISABLE_STATUS_FAILED,
   * @throws IOException If a failure occurred during enable reader mode
   */
  public @DualAntennaStatus
  int setPollingMode_DualAntenna(@ReaderModeStatus int ant1,
                                 @ReaderModeStatus int ant2) throws IOException;

  /**
   * This is the API to be called to get the discovery technology on both
   * antennas <li>This api shall be called only when NfcService is enabled.
   * <li>This api shall be called only when there are no NFC transactions
   * ongoing.
   * </ul>
   * @throws IOException If a failure occurred
   */
  public @DualAntennaStatus int[] getDiscoveryTechnology_DualAntenna()
      throws IOException;

  /**
   * This is the API to be called to get the Polling Mode on both antennas
   * <li>This api shall be called only when NfcService is enabled. <li>This
   * api shall be called only when there are no NFC transactions ongoing.
   * </ul>
   * @throws IOException If a failure occurred
   */
  public @DualAntennaStatus int getPollingMode_DualAntenna() throws IOException;
}
