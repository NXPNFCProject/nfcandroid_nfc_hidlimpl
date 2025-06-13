/*
 *
 *  The original Work has been changed by NXP.
 *
 *  Copyright 2025 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

package com.nxp.nfc.vendor.transit;

import android.nfc.NfcAdapter;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.INxpNfcNtfHandler;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Vector;
import java.util.concurrent.Executors;

public class TransitConfigHandler implements INxpNfcNtfHandler {
  private NfcAdapter mNfcAdapter;
  private final NfcOperations mNfcOperations;
  private final NxpNciPacketHandler mNxpNciPacketHandler;

  public static final int DISABLE_TRANSIT = 0x00;
  public static final int TRANSIT_CONFIG_REQUIRE_NFC_RESET = 0x01;
  public static final int TRANSIT_CONFIG_REQUIRE_RF_RESET = 0x02;
  public static final int TRANSIT_SETCONFIG_STAT_FAILED  = 0xFF;
  public static final int TRANSIT_CONFIG_SUB_GID = 0x70;
  public static final int TRANSIT_CONFIG_SUB_OID = 0x01;
  public static final int RF_REGISTER_SUB_OID = 0x02;
  private static final String TAG = "TransitConfigHandler";

  public TransitConfigHandler(NfcAdapter nfcAdapter) {
    this.mNfcAdapter = nfcAdapter;
    this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
    this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
  }

  @Override
  public void onVendorNciNotification(int gid, int oid, byte[] payload) {
    NxpNfcLogger.d(TAG, "payload: " + payload.length);
  }

  private int checkResetRequired(String configs) {
    NxpNfcLogger.d(TAG,"checkResetRequired configs: "+ configs);
    if(configs == null ) {
      return TRANSIT_CONFIG_REQUIRE_NFC_RESET;
    }
    if(configs.startsWith("UPDATE_") == true) {
      String[] lines = configs.split("\n");
      for (String token : lines) {
          if(token.startsWith("UPDATE_") == false) {
              NxpNfcLogger.d(TAG,
                      "Unexpected config parameters combination" + token);
              return TRANSIT_SETCONFIG_STAT_FAILED;
          }
      }
      return TRANSIT_CONFIG_REQUIRE_RF_RESET;
    } else {
        /*check if format of configs is fine*/
        String[] lines = configs.split("\n");
        for (String token : lines) {
            if(token.startsWith("UPDATE_") == true) {
              NxpNfcLogger.e(TAG,
                    "Unexpected config parameters combination" + token);

                return TRANSIT_SETCONFIG_STAT_FAILED;
            }
        }
        return TRANSIT_CONFIG_REQUIRE_NFC_RESET;
    }
  }

  private byte[] generateCmd(byte[] cmdBytes, byte[] configBytes) {
    byte[] payloadBytes = new byte[cmdBytes.length + configBytes.length];
    System.arraycopy(cmdBytes, 0, payloadBytes, 0, cmdBytes.length);
    System.arraycopy(configBytes, 0, payloadBytes, cmdBytes.length, configBytes.length);
    return payloadBytes;
  }

  private byte[] hexStringToByteArray(String config[]) {
    String configKey = config[0];
    String configVal = config[1].substring(2);

    int length = configVal.length();
    byte[] configPayload = new byte[2];
    configPayload[0] = (byte)RfRegisterConstants.RfConfigMap.get(configKey);
    configPayload[1] = (byte)(length/2);
    byte[] payload = new byte[length/2];
    for (int i = 0; i < length; i+=2) {
      payload[i/2] = (byte)(Integer.parseInt(configVal.substring(i, i+2), 16));
    }
    return generateCmd(configPayload, payload);
  }

  private byte[] generateRfRegisterPayload(String configs) {
    String[] configList = configs.lines().toArray(String[]::new);
    Vector<Byte> rfVector = new Vector<>();
    for (int configIndex = 0;configIndex < configList.length; configIndex++) {
      String[] rfConfig = configList[configIndex].split("=");
      byte[] rfPayload = hexStringToByteArray(rfConfig);
      for (int i = 0; i < rfPayload.length; i++) {
        rfVector.add(rfPayload[i]);
      }
    }
    byte[] rfRegisterBytes = new byte[rfVector.size()];
    for (int i = 0; i < rfVector.size(); i++) {
      rfRegisterBytes[i] = rfVector.get(i);
    }
    return rfRegisterBytes;
  }

  public boolean setConfig(String configs) throws IOException {
    if ((!mNfcOperations.isEnabled()) || (mNfcOperations.isCardEmulationActivated()) ||
        (mNfcOperations.isTagConnected())) {
      NxpNfcLogger.e(TAG, "NFC is disabled or busy, Rejecting request..");
      return false;
    }

    int resetStatus = checkResetRequired(configs);
    byte[] cmdBytes = {0x00, 0x00};
    byte[] configBytes = {DISABLE_TRANSIT};
    byte[] vendorRsp = {};

    if (resetStatus == TRANSIT_CONFIG_REQUIRE_RF_RESET) {
      if (mNfcOperations.isDiscoveryStarted()) {
        mNfcOperations.disableDiscovery();
      }
      byte[] rfRegisterPayload  = generateRfRegisterPayload(configs);
      cmdBytes[0] = (byte)(TRANSIT_CONFIG_SUB_GID|RF_REGISTER_SUB_OID);
      cmdBytes[1] = (byte)rfRegisterPayload.length;
      configBytes = rfRegisterPayload;
    } else if(resetStatus == TRANSIT_CONFIG_REQUIRE_NFC_RESET) {
      cmdBytes[0] = (byte)(TRANSIT_CONFIG_SUB_GID|TRANSIT_CONFIG_SUB_OID);
      if (configs == null) {
        NxpNfcLogger.d(TAG, "Removing libnfc-nci-update.conf");
        cmdBytes[1] = 0x00;
      } else {
        NxpNfcLogger.d(TAG, "Updating libnfc-nci-update.conf");
        configBytes = configs.getBytes(StandardCharsets.UTF_8);
        cmdBytes[1] = (byte)configBytes.length;
      }
    }

    if ((configBytes.length == 0) || (cmdBytes.length == 0)) {
      NxpNfcLogger.e(TAG, "payload is null");
      return false;
    }

    byte[] payloadBytes = generateCmd(cmdBytes, configBytes);
    Boolean status = true;
    mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(),
                                          this);
    try {
      NxpNfcLogger.d(TAG, "Sending VendorNciMessage");
      vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
        NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
        payloadBytes);
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    } finally {
      if (vendorRsp != null && vendorRsp.length > 1 &&
          vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
        if(resetStatus == TRANSIT_CONFIG_REQUIRE_NFC_RESET) {
          try {
            mNfcAdapter.disable();
            mNfcAdapter.enable();
          } catch (Exception e) {
            NxpNfcLogger.e(TAG, e.getMessage());
          }
        }
      } else {
        NxpNfcLogger.e(TAG, "setConfig() failed...");
        status = false;
      }
      if (resetStatus == TRANSIT_CONFIG_REQUIRE_RF_RESET) {
        mNfcOperations.enableDiscovery();
      }
    }
    return status;
  }
}
