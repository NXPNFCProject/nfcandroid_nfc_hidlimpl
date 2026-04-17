/*
 * Copyright 2025-2026 NXP
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

package com.nxp.nfc.vendor.fw;

import android.nfc.NfcAdapter;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;
import android.os.SystemProperties;

import java.io.IOException;
import java.util.Arrays;

public class NfcFirmwareInfo {
  private NfcAdapter mNfcAdapter;
  private final NxpNciPacketHandler mNxpNciPacketHandler;

  private static final String TAG = "NfcFirmwareInfo";
  public static final int GET_FW_VERSION = 0x6F;

  public NfcFirmwareInfo(NfcAdapter nfcAdapter) {
      this.mNfcAdapter = nfcAdapter;
      this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
  }

  public byte[] fwStringToBytes(String fwVerStr) {
    //String fw = "N02.20.51.00";
    fwVerStr = fwVerStr.substring(1).replace(".", ""); // "02205100"

    byte[] byteValues = new byte[] {
        (byte) Integer.parseInt(fwVerStr.substring(0, 2), 16),
        (byte) Integer.parseInt(fwVerStr.substring(2, 4), 16),
        (byte) Integer.parseInt(fwVerStr.substring(4, 6), 16)
    };
    return byteValues;
  }


  /**
   * Returns the NFC firmware version provided by the vendor NFC HAL.
   *
   * <p>The value is read from a system property initialized during NFC HAL
   * startup and is available irrespective of the current NFC ON/OFF state.</p>
   *
   * @return NFC firmware version as a byte array.
   * @throws IOException if the firmware ver prop is missing or not set.
   */
  public byte[] getFwVersion() throws IOException {
    NxpNfcLogger.d(TAG, "getFwVersion Enter");

    byte[] fwVersion =  new byte[3];
    Arrays.fill(fwVersion, (byte)0);

    String propVal = SystemProperties.get("nfc.fw.ver", "");
    NxpNfcLogger.d(TAG, "FW version:" + propVal);

    if (!propVal.equals("")) {
      fwVersion = fwStringToBytes(propVal);
    } else {
      NxpNfcLogger.d(TAG, "The nfc.fw.ver prop is not set");
      throw new IOException("nfc.fw.ver is not set");
    }
    return fwVersion;

  }
}
