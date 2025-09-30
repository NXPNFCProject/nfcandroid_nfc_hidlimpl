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

import android.app.Activity;
import android.nfc.NfcAdapter;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.vendor.dualAntenna.DualAntennaHandler;
import com.nxp.nfc.vendor.dualAntenna.INxpNfcDualAntenna.DualAntennaStatus;
import com.nxp.nfc.vendor.dualAntenna.INxpNfcDualAntenna.ReaderModeStatus;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * @class NxpNfcDualAntenna
 * @brief Concrete implementation of NFC Extension features
 *
 */

public final class NxpNfcDualAntenna implements INxpNfcDualAntenna {
  private static final String TAG = "NxpNfcDualAntenna";

  /**
   * @brief singleton instance object
   */
  private static NxpNfcDualAntenna sNxpNfcDualAntennaAdapter;

  /**
   * @brief Reflection variables for loading {@link NxpNfcExtras}
   */
  private Class mNxpNfcExtrasClass;
  private Object mNxpNfcExtrasObj;
  private NfcAdapter mNfcAdapter;
  private DualAntennaHandler mDualAntennaHandler;

  /**
   * @brief private constructor to create the instance of {@link
   * NxpNfcDualAntenna}
   * @param nfcAdapter
   */
  private NxpNfcDualAntenna(NfcAdapter nfcAdapter) {
    mNfcAdapter = nfcAdapter;
    mDualAntennaHandler = new DualAntennaHandler(nfcAdapter);
  }

  /**
   * @brief NxpNfcAdapter for application context,
   * or throws UnsupportedOperationException nfcAdapter is null.
   *
   * @param nfcAdapter
   * @return {@link NxpNfcDualAntenna} instance
   */
  public static synchronized NxpNfcDualAntenna
  getNxpNfcDualAntennaAdapter(NfcAdapter nfcAdapter) {
    if (sNxpNfcDualAntennaAdapter == null) {
      if (nfcAdapter == null) {
        NxpNfcLogger.e(TAG, "nfcAdapter is null");
        throw new UnsupportedOperationException();
      }
      sNxpNfcDualAntennaAdapter = new NxpNfcDualAntenna(nfcAdapter);
    }
    return sNxpNfcDualAntennaAdapter;
  }

  /**
   * @brief getter for accessing {@link INxpNfcDualAntenna}
   * make sure to call {@link #getNxpNfcDualAntennaAdapter()} before calling
   * this throws UnsupportedOperationException {@link
   * #sNxpNfcDualAntennaAdapter} is null.
   * @return {@link INxpNfcDualAntenna} instance
   */
  public static INxpNfcDualAntenna getNxpNfcAdapterInterface() {
    NxpNfcLogger.e(TAG, "getNxpNfcAdapterInterface");
    if (sNxpNfcDualAntennaAdapter == null) {
      throw new UnsupportedOperationException(
          "You need a reference from NxpNfcDualAntenna to use the "
          + " NXP NFC APIs");
    }
    return ((INxpNfcDualAntenna)sNxpNfcDualAntennaAdapter);
  }

  /**
   * @brief To be called to check feature is supported or not
   * @return {@link INxpNfcDualAntenna.isDualAnetannaSupported} instance
   */
  @Override
  public boolean isDualAnetannaSupported() throws IOException {
    return mDualAntennaHandler.isDualAnetannaSupported();
  }

  /**
   * @brief To be called to configure the antenna's with different polling
   * @return {@link
   *     INxpNfcDualAntenna.setDiscoveryTechnology_DualAntenna} instance
   */
  @Override
  public @DualAntennaStatus int setDiscoveryTechnology_DualAntenna(int tech1,
                                                                   int tech2)
      throws IOException {
    return mDualAntennaHandler.setDiscoveryTechnology_DualAntenna(tech1, tech2);
  }

  /**
   * @brief To be called to enable reader mode on either antenna's.
   * @return {@link INxpNfcDualAntenna.setPollingMode_DualAntenna}
   *     instance
   */
  @Override
  public @DualAntennaStatus int
  setPollingMode_DualAntenna(@ReaderModeStatus int ant1,
                             @ReaderModeStatus int ant2) throws IOException {
    return mDualAntennaHandler.setPollingMode_DualAntenna(ant1, ant2);
  }

  /**
   * @brief To be called to get the discovery technology on both antennas.
   * @return {@link INxpNfcDualAntenna.getDiscoveryTechnology_DualAntenna}
   *     instance
   */
  @Override
  public @DualAntennaStatus int[] getDiscoveryTechnology_DualAntenna()
      throws IOException {
    return mDualAntennaHandler.getDiscoveryTechnology_DualAntenna();
  }

  /**
   * @brief To be called to get the polling mode of both antennas.
   * @return {@link INxpNfcDualAntenna.getDiscoveryTechnology_DualAntenna}
   *     instance
   */
  @Override
  public @DualAntennaStatus int getPollingMode_DualAntenna()
      throws IOException {
    return mDualAntennaHandler.getPollingMode_DualAntenna();
  }
}
