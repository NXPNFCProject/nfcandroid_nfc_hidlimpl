/*
 *
 *  The original Work has been changed by NXP.
 *
 *  Copyright 2024 NXP
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

package com.nxp.nfc.vendor.mpos;

import android.app.Activity;
import android.nfc.NfcAdapter;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;
import java.io.IOException;

/**
 * This class is responsible to start/stop the mPos reader and
 * handles the mPos action notfications
 */
public class MposHandler implements INxpNfcNtfHandler {

  private static final String TAG = "MposHandler";

  public static final byte MPOS_READER_MODE_NTF_SUB_GID_OID = 0x5F;
  public static final byte MPOS_READER_MODE_SET_DEDICATED_MODE_CMD = 0x51;
  public static final byte DEDICATED_MODE_OFF = 0x00;
  public static final byte DEDICATED_MODE_ON = 0x01;
  /**
   * mpos mode status
   */
  public static final int MPOS_STATUS_SUCCESS = 0x00;
  public static final int MPOS_STATUS_FAILED = 0x03;

  private static boolean isMposEnabled = false;
  private final NxpNciPacketHandler mNxpNciPacketHandler;
  private final NfcOperations mNfcOperations;
  private final Object lock = new Object();
  private boolean isCardActivated = false;

  public MposHandler(NfcAdapter nfcAdapter, Activity activity) {
    this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
    this.mNfcOperations = NfcOperations.getInstance(nfcAdapter, activity);
    mPOSStarted(false);
  }

  public boolean isMposModeEnabled() { return isMposEnabled; }

  public void mPOSStarted(boolean enabled) { isMposEnabled = enabled; }

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

    if (subGidOid == MPOS_READER_MODE_NTF_SUB_GID_OID) {
      handleMposNotification(notificationType);
    }
  }

  private void handleMposNotification(byte notificationType) {
    ScrNtfActionEvent actionEvent =
        ScrNtfActionEvent.fromValue(notificationType);
    if (actionEvent == null) {
      NxpNfcLogger.d(TAG, "Unknown notification type: " + notificationType);
      return;
    }

    switch (actionEvent) {
    case SE_READER_START_RF_DISCOVERY:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_START_RF_DISCOVERY");
      mNfcOperations.setDiscoveryTech(NfcAdapter.FLAG_LISTEN_NFC_PASSIVE_A |
                                          NfcAdapter.FLAG_LISTEN_NFC_PASSIVE_B,
                                      NfcAdapter.FLAG_LISTEN_DISABLE);
      break;
    case SE_READER_START_DEFAULT_RF_DISCOVERY:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_START_DEFAULT_RF_DISCOVERY");
      mNfcOperations.enableDiscovery();
      break;
    case SE_READER_TAG_DISCOVERY_STARTED:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_START_SUCCESS");
      mPOSStarted(true);
      break;
    case SE_READER_TAG_DISCOVERY_START_FAILED:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_START_FAIL");
      break;
    case SE_READER_TAG_DISCOVERY_RESTARTED:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_RESTART");
      break;
    case SE_READER_TAG_ACTIVATED:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_ACTIVATED");
      synchronized (lock) {
        isCardActivated = true;
      }
      break;
    case SE_READER_STOP_RF_DISCOVERY:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_STOP_RF_DISCOVERY");
      if (mNfcOperations.isEnabled()) {
        mNfcOperations.disableDiscovery();
      }
      break;
    case SE_READER_STOPPED:
      mPOSStarted(false);
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_STOP_SUCCESS");
      synchronized (lock) {
        if (isCardActivated) {
          isCardActivated = false;
          lock.notify();
        }
      }
      break;
    case SE_READER_STOP_FAILED:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_STOP_FAIL");
      break;
    case SE_READER_TAG_TIMEOUT:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_TIMEOUT");
      break;
    case SE_READER_TAG_REMOVE_TIMEOUT:
      NxpNfcLogger.d(TAG, "ACTION_NFC_MPOS_READER_MODE_REMOVE_CARD");
      break;
    case SE_READER_MULTIPLE_TAG_DETECTED:
      NxpNfcLogger.d(TAG,
                     "ACTION_NFC_MPOS_READER_MODE_MULTIPLE_TARGET_DETECTED");
      break;
    default:
      NxpNfcLogger.d(TAG, "Unknown message received");
      break;
    }
  }

  public int mPOSSetReaderMode(String pkg, boolean enable) throws IOException {
    NxpNfcLogger.d(TAG, "mPOSSetReaderMode Enter : " + enable);

    if (enable == false) {
      synchronized (lock) {
        /* When card activated. we should not allow to stop mPOS untill card
         *  deactivated notification arrived.
         */
        if (isCardActivated) {
          try {
            NxpNfcLogger.d(TAG, "Transaction is going on. Wait to complete" +
                                    enable);
            lock.wait(); // Wait for SE_READER_STOPPED event
          } catch (InterruptedException e) {
            isCardActivated = false;
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    if (enable != isMposEnabled) {
      if (mNfcOperations.isEnabled()) {
        NxpNfcLogger.d(TAG, "Disabling discovery");
        mNfcOperations.disableDiscovery();
      }
      isMposEnabled = enable;
    }
    mNxpNciPacketHandler.setCurrentNtfHandler(this);
    byte[] mpos = {MPOS_READER_MODE_SET_DEDICATED_MODE_CMD,
                   (byte)(enable ? DEDICATED_MODE_ON : DEDICATED_MODE_OFF)};
    try {
      NxpNfcLogger.d(TAG, "Sending VendorNciMessage to start MPOS");
      byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          mpos);
      if (vendorRsp != null && vendorRsp.length > 0 &&
          vendorRsp[0] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
        return MPOS_STATUS_SUCCESS;
      } else {
        return MPOS_STATUS_FAILED;
      }
    } catch (Exception e) {
      NxpNfcLogger.d(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
  }

  public boolean mPOSGetReaderMode(String pkg) throws IOException {
    try {
      return isMposModeEnabled();
    } catch (Exception e) {
      NxpNfcLogger.d(TAG, "Exception in mPOSGetReaderMode");
      e.printStackTrace();
      throw new IOException("RemoteException in mPOSGetReaderMode (int state)");
    }
  }
}
