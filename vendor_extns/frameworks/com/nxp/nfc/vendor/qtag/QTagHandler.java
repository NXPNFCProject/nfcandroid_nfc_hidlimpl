/*
 *
 *  The original Work has been changed by NXP.
 *
 *  Copyright 2024-2025 NXP
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

package com.nxp.nfc.vendor.qtag;

import android.app.Activity;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcAdapter;
import com.nxp.nfc.NxpNfcAdapter.NxpReaderCallback;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;
import java.io.IOException;

public class QTagHandler implements INxpNfcNtfHandler {

  private final NxpNciPacketHandler mNxpNciPacketHandler;
  private NfcAdapter mNfcAdapter;
  private final NfcOperations mNfcOperations;

  public static final int QTAG_STATUS_SUCCESS = 0x00;
  public static final int QTAG_STATUS_FAILED = 0x03;
  public static final int QTAG_STATUS_DETECTED = 0x00;
  public static final byte QTAG_SUB_GID_OID = 0x31;
  public static final byte QTAG_DETECTION_SUB_GID_OID = 0x32;
  public static final int INVALID_POLL_TECH = 0;

  public static Object qtagSync = new Object();
  public static boolean sIsQPollEnabled = false;
  private static boolean sQTagDetected = false;
  private static final String TAG = "QTagHandler";

  public QTagHandler(NfcAdapter nfcAdapter) {
    this.mNfcAdapter = nfcAdapter;
    this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
    this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
  }

  @Override
  public void onVendorNciNotification(int gid, int oid, byte[] payload) {
    if (payload == null || payload.length < 2) {
      NxpNfcLogger.d(TAG, "Invalid payload");
      return;
    }

    byte subGidOid = payload[0];
    byte notificationType = payload[1];
    NxpNfcLogger.d(TAG, "Sub-GidOid: " + subGidOid +
                            ", Notification Type: " + notificationType);

    synchronized (qtagSync) {
      if (subGidOid == QTAG_DETECTION_SUB_GID_OID) {
        if (payload.length > 1) {
          if (payload[1] == QTAG_STATUS_DETECTED)
            sQTagDetected = true;
          else
            sQTagDetected = false;
        } else {
          sQTagDetected = false;
        }
      } else if ((subGidOid == QTAG_SUB_GID_OID)
          && (notificationType == NfcAdapter.SEND_VENDOR_NCI_STATUS_REJECTED)) {
        sIsQPollEnabled = false;
        sQTagDetected = false;
        NxpNfcLogger.d(TAG, "sIsQPollEnabled: " + sIsQPollEnabled);
      }
      NxpNfcLogger.d(TAG, "sQTagDetected: " + sQTagDetected);
    }
  }

  public int enableQTag(Activity activity, int mode,
                        NxpReaderCallback mQTagCallback, int pollTech)
      throws IOException {
    NxpNfcLogger.d(TAG,
                   "enableQTag Enter mode: " + mode + " pollTech:" + pollTech);
    QTagMode qMode = QTagMode.fromValue(mode);
    int status = QTAG_STATUS_FAILED;

    if (mNfcAdapter.getAdapterState() == NfcAdapter.STATE_OFF) {
      NxpNfcLogger.e(TAG, "NFC is disabled");
      return status;
    }

    if ((qMode != QTagMode.DISABLE_QTAG_MODE) &&
        (pollTech == INVALID_POLL_TECH)) {
      NxpNfcLogger.e(TAG, "Invalid poll tech");
      return status;
    }

    byte[] qtag = {QTAG_SUB_GID_OID,
                   (byte)QTagMode.DISABLE_QTAG_MODE.getValue()};
    if (qMode == QTagMode.DISABLE_QTAG_MODE) {
      qtag[1] = (byte)QTagMode.DISABLE_QTAG_MODE.getValue();
    } else if (qMode == QTagMode.ENABLE_QTAG_ONLY_MODE) {
      qtag[1] = (byte)QTagMode.ENABLE_QTAG_ONLY_MODE.getValue();
    } else if (qMode == QTagMode.APPEND_QTAG_MODE) {
      qtag[1] = (byte)QTagMode.APPEND_QTAG_MODE.getValue();
    } else {
      NxpNfcLogger.e(TAG, "Invalid mode");
      return status;
    }

    synchronized (qtagSync) { sQTagDetected = false; }

    mNxpNciPacketHandler.setCurrentNtfHandler(this);
    try {
      NxpNfcLogger.d(TAG, "Sending VendorNciMessage");
      byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          qtag);
      if (vendorRsp != null && vendorRsp.length > 0 &&
          vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
        status = QTAG_STATUS_SUCCESS;
        if (qMode == QTagMode.DISABLE_QTAG_MODE)
          sIsQPollEnabled = false;
        else
          sIsQPollEnabled = true;
      } else {
        NxpNfcLogger.e(TAG, "enableQtag failed!!");
        status = QTAG_STATUS_FAILED;
        sIsQPollEnabled = false;
        return status;
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }

    if (status == QTAG_STATUS_SUCCESS) {
      if ((qMode == QTagMode.ENABLE_QTAG_ONLY_MODE) ||
          (qMode == QTagMode.APPEND_QTAG_MODE)) {
        synchronized (qtagSync) {
          mNfcAdapter.enableReaderMode(
              activity, new NfcAdapter.ReaderCallback() {
                @Override
                public void onTagDiscovered(final Tag tag) {
                  NxpNfcLogger.d(TAG, "***** QTag detected *****\n");
                  if (mQTagCallback != null)
                    mQTagCallback.onNxpTagDiscovered(tag, sQTagDetected);
                }
              }, pollTech, null);
        }
      } else if (qMode == QTagMode.DISABLE_QTAG_MODE) {
        mNfcOperations.enableDiscovery();
      }
    }
    return status;
  }
}
