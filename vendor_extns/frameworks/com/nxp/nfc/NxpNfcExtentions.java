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
import com.nxp.nfc.INxpNfcExtentions;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.vendor.ntag.INxpNfcNTag;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class NxpNfcExtentions implements INxpNfcExtentions {
  private static final String TAG = "NxpNfcExtentions";

  /**
   * @brief Reflection variables for loading {@link NxpNfcNTag}
   */
  private Class mNxpNfcNTagClass;
  private Object mNxpNfcNTagObj;

  private NfcAdapter mNfcAdapter;

  /**
   * @brief private constructor to create the instance of {@link
   * NxpNfcExtentions}
   * @param nfcAdapter
   */
  public NxpNfcExtentions(NfcAdapter nfcAdapter) { mNfcAdapter = nfcAdapter; }
  /**
   * @brief Creates the Instance of {@link NxpNfcExtentions}
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
      NxpNfcLogger.e(TAG, "Error in Instantiating NxpNfcExtentions! Msg: " +
                              e.getLocalizedMessage());
    }
    return null;
  }
}
