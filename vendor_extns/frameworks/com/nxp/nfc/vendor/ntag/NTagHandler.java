/*
 *  Copyright 2025-2026 NXP
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

package com.nxp.nfc.vendor.ntag;

import android.annotation.IntDef;
import android.app.Activity;
import android.nfc.NfcAdapter;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcAdapter.NxpNTagStatusCallback;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.NxpNfcUtils;
import com.nxp.nfc.core.NxpNciPacketHandler;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NTagHandler implements INxpNfcNtfHandler {

  private final NxpNciPacketHandler mNxpNciPacketHandler;
  private NfcAdapter mNfcAdapter;
  private NxpNTagStatusCallback mNTagStatusCallback;
  private static final ExecutorService NTAG_CALLBACK_EXECUTOR =
                        Executors.newSingleThreadExecutor();

  public static final int ENABLE_NTAG = 0x01;
  public static final int DISABLE_NTAG = 0x00;

  public static final int STATUS_SUCCESS = 0x00;
  public static final int STATUS_FAILED = 0x01;

  @IntDef(value =
              {
                  ENABLE_NTAG,
                  DISABLE_NTAG,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface NTagMode {}

  @IntDef(value =
              {
                  STATUS_SUCCESS,
                  STATUS_FAILED,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface NTagStatus {}

  public enum NTagStatusCode {
    Success(0x00),
    Failed(0x01);
    public final int value;
    NTagStatusCode(int value) { this.value = (int)value; }
  }

  public enum NTagSubOid {
    Disable(0x00),
    Enable(0x01),
    Append(0x02),
    Detected(0x03),
    NTagEnable(0x04),
    NTagDisable(0x05),
    NTagRemove(0x06),
    NTagEnableStatus(0x07);
    public int value;
    NTagSubOid(int value) { this.value = (int)value; }
  }

  public static final byte QTAG_SUB_GID = 0x30;
  public static Object ntagSync = new Object();
  public static boolean sIsQPollEnabled = false;
  private static boolean sNTagDetected = false;
  private static final String TAG = "NTagHandler";

  public NTagHandler(NfcAdapter nfcAdapter) {
    this.mNfcAdapter = nfcAdapter;
    this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
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

    synchronized (ntagSync) {
      if (subGidOid == (QTAG_SUB_GID | NTagSubOid.Detected.value)) {
        if (payload.length > 1) {
          if (payload[1] == NTagStatusCode.Success.value) {
            sNTagDetected = true;
            NxpNfcLogger.d(TAG, "NTag cback:" + sNTagDetected);
            if (mNTagStatusCallback != null) {
              byte uid[] = new byte[payload.length - 2];
              System.arraycopy(payload, 2, uid, 0, payload.length - 2);
              mNTagStatusCallback.onNTagDiscovered(uid, sNTagDetected);
              NxpNfcLogger.d(TAG, "NTag UID:" + NxpNfcUtils.toHexString(uid));
            }
          } else
            sNTagDetected = false;
        } else {
          sNTagDetected = false;
        }
      } else if ((subGidOid == (QTAG_SUB_GID | NTagSubOid.NTagEnable.value)) &&
                 (notificationType ==
                  NfcAdapter.SEND_VENDOR_NCI_STATUS_REJECTED)) {
        sIsQPollEnabled = false;
        sNTagDetected = false;
        NxpNfcLogger.d(TAG, "sIsQPollEnabled: " + sIsQPollEnabled);
      } else if (subGidOid == (QTAG_SUB_GID | NTagSubOid.NTagRemove.value)) {
        sNTagDetected = false;
        if (mNTagStatusCallback != null) {
          mNTagStatusCallback.onNTagDiscovered(null, sNTagDetected);
          NxpNfcLogger.d(TAG, "NTag cback:" + sNTagDetected);
        }
        NxpNfcLogger.d(TAG, "NTag removed");
      }
      NxpNfcLogger.d(TAG, "sNTagDetected: " + sNTagDetected);
    }
  }

  public boolean isNTagEnabled() throws IOException {

    if (mNfcAdapter.getAdapterState() == NfcAdapter.STATE_OFF) {
      NxpNfcLogger.e(TAG, "NFC is disabled");
      return false;
    }

    if (!sIsQPollEnabled)
      return false;

    try {
      NxpNfcLogger.d(TAG, "Sending VendorNciMessage for isNTagModeEnabled");
      byte[] ntag = {(byte)(QTAG_SUB_GID | NTagSubOid.NTagEnableStatus.value),
                     (byte)NTagSubOid.NTagEnableStatus.value};
      byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          ntag);
      if (vendorRsp != null && vendorRsp.length > 0 &&
          vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
        NxpNfcLogger.d(TAG, "NTagMode enabled!!");
        return true;
      } else {
        NxpNfcLogger.d(TAG, "NTagMode disabled!!");
        sIsQPollEnabled = false;
        return false;
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
  }

  public @NTagStatus int setNTagMode(Activity activity,
                                     NxpNTagStatusCallback mNTagStatusCallback,
                                     @NTagMode int qMode) throws IOException {
    int status = NTagStatusCode.Failed.value;
    boolean mNtagMode = (qMode != 0);
    if (mNTagStatusCallback == null)
      return status;

    if (sIsQPollEnabled == mNtagMode) {
      NxpNfcLogger.d(TAG, "NTag mode is already in the same mode" + qMode);
      return NTagStatusCode.Success.value;
    }

    this.mNTagStatusCallback = mNTagStatusCallback;
    NxpNfcLogger.d(TAG, "setNTagMode Enter mode: " + qMode);

    if (mNfcAdapter.getAdapterState() == NfcAdapter.STATE_OFF) {
      NxpNfcLogger.d(TAG, "NFC is disabled");
      return status;
    }

    byte[] ntag = {(byte)(QTAG_SUB_GID | NTagSubOid.NTagDisable.value),
                   (byte)NTagSubOid.NTagDisable.value};

    if (qMode == NTagSubOid.Enable.value) {
      ntag[0] = (byte)(QTAG_SUB_GID | NTagSubOid.NTagEnable.value);
      ntag[1] = (byte)NTagSubOid.NTagEnable.value;
    }

    synchronized (ntagSync) { sNTagDetected = false; }

    try {
      NxpNfcLogger.d(TAG, "Sending VendorNciMessage");
      byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          ntag);
      if (vendorRsp != null && vendorRsp.length > 0 &&
          vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
        status = NTagStatusCode.Success.value;
        if (qMode == NTagSubOid.Disable.value) {
          sIsQPollEnabled = false;
          mNxpNciPacketHandler.unregisterNtfCallback(this);
        } else {
          sIsQPollEnabled = true;
          mNxpNciPacketHandler.registerNtfCallback(NTAG_CALLBACK_EXECUTOR, this);
        }
      } else {
        NxpNfcLogger.e(TAG, "setNTagMode failed!!");
        status = NTagStatusCode.Failed.value;
        sIsQPollEnabled = false;
        return status;
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }

    return status;
  }
}
