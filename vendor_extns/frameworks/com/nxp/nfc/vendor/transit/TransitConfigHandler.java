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
  public static final int MAX_CONFIG_LEN = 249;
  public static final int SUB_GIDOID_INDEX = 0x00;
  public static final int PAYLOAD_LEN_INDEX = 0x01;
  public static final int PBF_INDEX = 0x02;
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
    if ((configBytes.length == 0) || (cmdBytes.length == 0)) {
      NxpNfcLogger.e(TAG, "payload is null");
      return new byte[0];
    }
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

  public boolean sendCmd(byte[] payloadBytes) {
    byte[] vendorRsp = {};
    mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(),
                                          this);
    try {
      NxpNfcLogger.d(TAG, "Sending VendorNciMessage");
      vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
        NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
        payloadBytes);
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
    }
    if (vendorRsp != null && vendorRsp.length > 1 &&
      vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS)
      return true;
    else
      return false;
  }

  public boolean setConfig(String configs) throws IOException {
    if ((!mNfcOperations.isEnabled()) || (mNfcOperations.isCardEmulationActivated()) ||
        (mNfcOperations.isTagConnected())) {
      NxpNfcLogger.e(TAG, "NFC is disabled or busy, Rejecting request..");
      return false;
    }

    int resetStatus = checkResetRequired(configs);
    byte[] configBytes = {DISABLE_TRANSIT};
    Boolean cmdStatus = false;

    if (resetStatus == TRANSIT_CONFIG_REQUIRE_RF_RESET) {
      if (mNfcOperations.isDiscoveryStarted()) {
        mNfcOperations.disableDiscovery();
      }
      byte[] rfRegisterPayload  = generateRfRegisterPayload(configs);
      byte[] cmdBytes = {(byte)(TRANSIT_CONFIG_SUB_GID|RF_REGISTER_SUB_OID),
                        (byte)rfRegisterPayload.length};
      configBytes = rfRegisterPayload;
      byte[] payloadBytes = generateCmd(cmdBytes, configBytes);
      try {
        if (payloadBytes.length == 0)
          cmdStatus = false;
        else
          cmdStatus = sendCmd(payloadBytes);
      } catch (Exception e) {
        NxpNfcLogger.e(TAG, "Exception in sendCmd");
        throw new IOException("Error sending VendorNciMessage", e);
      }
    } else if(resetStatus == TRANSIT_CONFIG_REQUIRE_NFC_RESET) {
      int startIndex = 0;
      int endIndex = 0;
      byte[] cmdBytes = {0x00, 0x00, 0x00};
      cmdBytes[SUB_GIDOID_INDEX] = (byte)(TRANSIT_CONFIG_SUB_GID|TRANSIT_CONFIG_SUB_OID);
      if (configs == null) {
        NxpNfcLogger.d(TAG, "Removing libnfc-nci-update.conf");
        cmdBytes[PAYLOAD_LEN_INDEX] = 0x00;
      } else {
        NxpNfcLogger.d(TAG, "Updating libnfc-nci-update.conf");
        configBytes = configs.getBytes(StandardCharsets.UTF_8);
        cmdBytes[PAYLOAD_LEN_INDEX] = (byte)configBytes.length;
      }
      while (startIndex < configBytes.length) {
        int pbf = 1;
        endIndex = startIndex + MAX_CONFIG_LEN;
        if (endIndex >= configBytes.length) {
          pbf = 0;
          endIndex = configBytes.length;
        }
        byte[] partialPayload = Arrays.copyOfRange(configBytes, startIndex, endIndex);
        startIndex = endIndex;

        cmdBytes[PAYLOAD_LEN_INDEX] = (byte)(partialPayload.length + 1);
        cmdBytes[PBF_INDEX] = (byte)pbf;
        byte[] payloadBytes = generateCmd(cmdBytes, partialPayload);
        try {
          if (payloadBytes.length == 0)
            cmdStatus = false;
          else
            cmdStatus = sendCmd(payloadBytes);
          if (!cmdStatus) {
            NxpNfcLogger.e(TAG, "Error in sendCmd, clearing stored config...");
            byte[] emptyPayload = {(byte)(TRANSIT_CONFIG_SUB_GID|TRANSIT_CONFIG_SUB_OID),
                        DISABLE_TRANSIT};
            sendCmd(emptyPayload);
            break;
          }
        } catch (Exception e) {
          NxpNfcLogger.e(TAG, "Exception in sendCmd");
          throw new IOException("Error sending VendorNciMessage", e);
        }
      }
    }
    if(cmdStatus) {
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
    }
    if (resetStatus == TRANSIT_CONFIG_REQUIRE_RF_RESET) {
      mNfcOperations.enableDiscovery();
    }
    return cmdStatus;
  }
}
