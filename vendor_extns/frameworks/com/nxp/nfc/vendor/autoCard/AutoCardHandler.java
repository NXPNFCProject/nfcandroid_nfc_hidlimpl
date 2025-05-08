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

package com.nxp.nfc.vendor.autoCard;

import android.app.Activity;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.os.Bundle;
import com.nxp.nfc.INxpNfcAdapter;
import com.nxp.nfc.INxpNfcAdapter.AutoCardStatus;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;
import java.io.IOException;

public class AutoCardHandler {
  private final NxpNciPacketHandler mNxpNciPacketHandler;
  private NfcAdapter mNfcAdapter;
  private final NfcOperations mNfcOperations;

  private static int NFC_NCI_PROP_GID = 0x2F;
  private static int NXP_NFC_AUTO_CARD_SUB_OID = 0x50;
  private static int NXP_NFC_AUTO_CARD_SUB_OID_INDEX = 0;
  private static int AUTO_CARD_SET_AID = 0x01;
  private static int AUTO_CARD_GET_AID = 0x02;
  private static int AUTO_CARD_AID_PAYLOAD_OFFSET = 0x03;
  private static int AUTO_CARD_STATUS_RSP_INDEX = 0x03;
  private static int AUTO_CARD_CONFIG_INDEX = 0x01;
  private static int CMA_READY_COUNT_INDEX = 0x02;

  public static Object autoCardSync = new Object();
  private static final String TAG = "AutoCardHandler";

  public AutoCardHandler(NfcAdapter nfcAdapter) {
    this.mNfcAdapter = nfcAdapter;
    this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
    this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
  }

  public byte[] getAutoCardAID() throws IOException {

    if (mNfcAdapter.getAdapterState() == NfcAdapter.STATE_OFF) {
      NxpNfcLogger.e(TAG, "NFC is disabled");
      byte[] rspAid = new byte[1];
      rspAid[0] = (byte)INxpNfcAdapter.EACSTATUS_ERROR_NFC_IS_OFF;
      return rspAid;
    }

    byte[] payload = new byte[2];
    payload[NXP_NFC_AUTO_CARD_SUB_OID_INDEX] = (byte)NXP_NFC_AUTO_CARD_SUB_OID;
    payload[AUTO_CARD_CONFIG_INDEX] = (byte)AUTO_CARD_GET_AID;

    try {
      byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          payload);
      if (vendorRsp == null) {
        NxpNfcLogger.e(TAG, "sendVendorNcicmd  response: Null");
        byte[] rspAid = new byte[1];
        rspAid[0] = (byte)NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED;
        return rspAid;
      }

      if (vendorRsp.length > AUTO_CARD_STATUS_RSP_INDEX) {
        if (vendorRsp[AUTO_CARD_STATUS_RSP_INDEX] !=
            INxpNfcAdapter.STATUS_SUCCESS) {
          byte[] rspAid = new byte[1];
          rspAid[0] = vendorRsp[AUTO_CARD_STATUS_RSP_INDEX];
          NxpNfcLogger.e(TAG, "Get Autocard failed Rsp!!" + rspAid);
          return rspAid;
        } else {
          byte[] rspAid =
              new byte[vendorRsp.length - AUTO_CARD_AID_PAYLOAD_OFFSET];
          NxpNfcLogger.e(TAG, "Get Autocard Success!!");
          System.arraycopy(vendorRsp, AUTO_CARD_AID_PAYLOAD_OFFSET, rspAid, 0,
                           vendorRsp.length - AUTO_CARD_AID_PAYLOAD_OFFSET);
          return rspAid;
        }
      } else {
        byte[] rspAid = new byte[1];
        rspAid[0] = (byte)NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED;
        NxpNfcLogger.e(TAG, "Get Autocard Faild!!");
        NxpNfcLogger.e(TAG, "sendVendorNcicmd  response:" + vendorRsp.length);
        return rspAid;
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
  }

  public @AutoCardStatus int setAutoCardAID(byte[] aids, int cmaReadyCount)
      throws IOException {
    if (aids == null)
      return INxpNfcAdapter.EACSTATUS_ERROR_INVALID_PARAM;

    if (mNfcAdapter.getAdapterState() == NfcAdapter.STATE_OFF) {
      NxpNfcLogger.e(TAG, "NFC is disabled");
      return INxpNfcAdapter.EACSTATUS_ERROR_NFC_IS_OFF;
    }
    byte[] payload = new byte[aids.length + AUTO_CARD_AID_PAYLOAD_OFFSET];

    payload[CMA_READY_COUNT_INDEX] = (byte)cmaReadyCount;

    payload[NXP_NFC_AUTO_CARD_SUB_OID_INDEX] = (byte)NXP_NFC_AUTO_CARD_SUB_OID;
    payload[AUTO_CARD_CONFIG_INDEX] = (byte)AUTO_CARD_SET_AID;

    try {
      System.arraycopy(aids, 0, payload, AUTO_CARD_AID_PAYLOAD_OFFSET,
                       aids.length);

      byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          payload);
      if (vendorRsp == null) {
        NxpNfcLogger.e(TAG, "setAutoCardAID response: Null");
        return NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED;
      }
      NxpNfcLogger.e(TAG, "sendVendorNcicmd  Set response length:" +
                              vendorRsp.length);
      if (vendorRsp.length > AUTO_CARD_STATUS_RSP_INDEX) {
        NxpNfcLogger.e(TAG, "sendVendorNcicmd response" +
                                vendorRsp[AUTO_CARD_STATUS_RSP_INDEX]);
        return vendorRsp[AUTO_CARD_STATUS_RSP_INDEX];
      } else {
        NxpNfcLogger.e(TAG, "sendVendorNcicmd failed");
        return NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED;
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
  }
}
