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

package com.nxp.nfc;

import android.nfc.NfcAdapter;
import android.os.RemoteException;
import android.util.Log;
import com.nxp.nfc.INxpNfcExtensions;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.vendor.dualAntenna.INxpNfcDualAntenna;
import com.nxp.nfc.vendor.ntag.INxpNfcNTag;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class NxpNfcExtensions implements INxpNfcExtensions {
  private static final String TAG = "NxpNfcExtensions";

  /**
   * @brief Reflection variables for loading {@link NxpNfcNTag}
   */
  private Class mNxpNfcNTagClass;
  private Object mNxpNfcNTagObj;
  private Class mNxpNfcDualAntennaClass;
  private Object mNxpNfcDualAntennaObj;

  private NfcAdapter mNfcAdapter;

  /**
   * @brief private constructor to create the instance of {@link
   * NxpNfcExtensions}
   * @param nfcAdapter
   */
  public NxpNfcExtensions(NfcAdapter nfcAdapter) { mNfcAdapter = nfcAdapter; }
  /**
   * @brief Creates the Instance of {@link NxpNfcExtensions}
   * @param None
   * @return None
   */
  public INxpNfcNTag getNxpNfcNTagInterface() {
    try {
      mNxpNfcNTagClass = Class.forName("com.nxp.nfc.vendor.ntag.NxpNfcNTag");
      Method[] methods = mNxpNfcNTagClass.getDeclaredMethods();
      NxpNfcLogger.d(TAG, "Total methods:" + methods.length);
      for (Method method : methods) {
        NxpNfcLogger.d(TAG, "Method: " + method.getName());
      }
      Constructor<?> nxpNfcNTagCon =
          mNxpNfcNTagClass.getDeclaredConstructor(NfcAdapter.class);
      mNxpNfcNTagObj = nxpNfcNTagCon.newInstance(mNfcAdapter);

      return ((INxpNfcNTag) mNxpNfcNTagObj);
    } catch (ClassNotFoundException | InstantiationException |
             IllegalAccessException | IllegalArgumentException |
             InvocationTargetException | NoSuchMethodException e) {
      NxpNfcLogger.e(TAG, "Error in Instantiating NxpNfcExtensions! Msg: " +
                              e.getLocalizedMessage());
    }
    return null;
  }
  /**
   * @brief Creates the Instance of {@link NxpNfcExtensions}
   * @param None
   * @return None
   */
  public INxpNfcDualAntenna getNxpNfcDualAntennaInterface() {
    try {
      mNxpNfcDualAntennaClass =
          Class.forName("com.nxp.nfc.vendor.dualAntenna.NxpNfcDualAntenna");
      Method[] methods = mNxpNfcDualAntennaClass.getDeclaredMethods();
      NxpNfcLogger.d(TAG, "Total methods:" + methods.length);
      for (Method method : methods) {
        NxpNfcLogger.d(TAG, "Method: " + method.getName());
      }
      Constructor<?> nxpNfcDualAntennaCon =
          mNxpNfcDualAntennaClass.getDeclaredConstructor(NfcAdapter.class);
      mNxpNfcDualAntennaObj = nxpNfcDualAntennaCon.newInstance(mNfcAdapter);

      return ((INxpNfcDualAntenna)mNxpNfcDualAntennaObj);
    } catch (ClassNotFoundException | InstantiationException |
             IllegalAccessException | IllegalArgumentException |
             InvocationTargetException | NoSuchMethodException e) {
      NxpNfcLogger.e(TAG, "Error in Instantiating NxpNfcExtensions! Msg: " +
                              e.getLocalizedMessage());
    }
    return null;
  }
}
