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

package com.nxp.nfc.vendor.ntag;

import android.app.Activity;
import android.nfc.NfcAdapter;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.vendor.ntag.INxpNfcNTag.NTagMode;
import com.nxp.nfc.vendor.ntag.INxpNfcNTag.NTagStatus;
import com.nxp.nfc.vendor.ntag.NTagHandler;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
/**
 * @class NxpNfcNTag
 * @brief Concrete implementation of NFC Extension features
 *
 */
public class NxpNfcNTag implements INxpNfcNTag {
  private static final String TAG = "NxpNfcNTag";

  private NfcAdapter mNfcAdapter;
  private NTagHandler mNTagHandler;

  /**
   * @brief private constructor to create the instance of {@link
   * NxpNfcNTag}
   * @param nfcAdapter
   */
  public NxpNfcNTag(NfcAdapter nfcAdapter) {
    mNfcAdapter = nfcAdapter;
    mNTagHandler = new NTagHandler(nfcAdapter);
  }

  public interface NxpNTagStatusCallback {
    void onNTagDiscovered(byte[] uid, boolean isNTagDetected);
  }

  /**
   * @brief This is provides the info whether NTag mode is enabled or not
   * @return {@link INxpNfcNTag.isNTagEnabled} instance
   */
  @Override
  public boolean isNTagEnabled() throws IOException {
    return mNTagHandler.isNTagEnabled();
  }

  /**
   * @brief To be called to enable NTag
   * @return {@link INxpNfcNTag.setNTagMode} instance
   */
  @Override
  public @NTagStatus int setNTagMode(Activity activity,
                                     NxpNTagStatusCallback mNTagStatusCallback,
                                     @NTagMode int mode) throws IOException {
    return mNTagHandler.setNTagMode(activity, mNTagStatusCallback, mode);
  }
}
