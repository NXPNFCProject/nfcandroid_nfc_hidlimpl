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

package com.nxp.nfc.vendor.dualAntenna;

import android.annotation.IntDef;
import android.nfc.NfcAdapter;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcAdapter;
import com.nxp.nfc.NxpNfcAdapter.NxpReaderCallback;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Set;

/**
 * This class is responsible to enable/disable the dual antenna
 */

public class DualAntennaHandler implements INxpNfcNtfHandler {

  private static final String TAG = "DualAntenna";
  private final NxpNciPacketHandler mNxpNciPacketHandler;
  private final NfcOperations mNfcOperations;
  private static int APPEND_Q_POLL = 0X02;
  private static int ONLY_Q_POLL = 0X01;
  private static int Q_TAG_MODE_INDEX = 1;

  public static final byte DUAL_ANTENNA_SUB_GID_OID = 0x40;
  public static final byte QTAG_SUB_GID_OID = 0x31;
  public static boolean sIsQPollEnabled = false;
  static int antennaOneConfiguration = 0x00;
  static int antennaTwoConfiguration = 0x00;
  static int antennaPolling = 0x00;

  public static final int STATUS_SUCCESS = 0x00;
  public static final int STATUS_FAILED = 0x01;

  public static final int ENABLE_READERMODE = 0x01;
  public static final int DISABLE_READERMODE = 0x00;

  @IntDef(value =
              {
                  ENABLE_READERMODE,
                  DISABLE_READERMODE,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface ReaderModeStatus {}

  @IntDef(value =
              {
                  STATUS_SUCCESS,
                  STATUS_FAILED,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface DualAntennaStatus {}

  public enum DualAntennaStatusCode {
    Success(0x00),
    Failed(0x01);
    public final int value;
    DualAntennaStatusCode(int value) { this.value = (int)value; }
    public int getValue() { return value; }
  }

  public enum ReaderMode {
    Disable(0x00),
    Enable(0x01);
    public final int value;
    ReaderMode(int value) { this.value = (int)value; }
    public int getValue() { return value; }
  }

  public enum ConDiscParam {
    EnableAnt1(0x41),
    EnableAnt2(0x81),
    EnableBothAnt(0xC1);
    public final int value;
    ConDiscParam(int value) { this.value = (int)value; }
    public int getValue() { return value; }
  }

  public enum DualAntennaSubOid {
    Disable(0x00),
    Enable(0x01),
    isSupported(0x02),
    setDiscoveryTechnology(0x03),
    enableReaderMode(0x04);
    public int value;
    DualAntennaSubOid(int value) { this.value = (int)value; }
    public int getValue() { return value; }
  }

  public enum RfPollingStatus {
    polling_disable(0x00),
    passive_ABF(0x01),
    passive_ABFQK(0x05),
    passive_Q(0x0D);
    public int value;
    RfPollingStatus(int value) { this.value = (int)value; }
    public int getValue() { return value; }
  }

  public enum QTagMode {
    DISABLE_QTAG_MODE(0),
    ENABLE_QTAG_ONLY_MODE(1),
    APPEND_QTAG_MODE(2),
    INVALID_MODE(3);
    private final int mValue;
    QTagMode(int value) { this.mValue = (int)value; }
    public int getValue() { return mValue; }
    public static QTagMode fromValue(int value) {
      for (QTagMode mode : values()) {
        if (mode.getValue() == value)
          return mode;
      }
      return INVALID_MODE;
    }
  }

  public DualAntennaHandler(NfcAdapter nfcAdapter) {
    NxpNfcLogger.d(TAG, "");
    this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
    this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
  }

  @Override
  public void onVendorNciNotification(int gid, int oid, byte[] payload) {
    if (payload == null || payload.length < 2) {
      NxpNfcLogger.d(TAG, "Invalid payload");
      return;
    }
  }

  public boolean isDualAnetannaSupported() throws IOException {

    byte[] payload = {
        (byte)(DUAL_ANTENNA_SUB_GID_OID | DualAntennaSubOid.isSupported.value)};
    try {
      byte[] isSupported = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          payload);

      if (!(isSupported != null && isSupported.length > 0 &&
          isSupported[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS)) {
        return false;
      }
      return true;
    } catch (Exception e) {
      NxpNfcLogger.d(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
  }

  public void enableQTagDualAntenna(int mode) throws IOException {
    QTagMode qMode = QTagMode.fromValue(mode);
    byte[] qtag = {QTAG_SUB_GID_OID,
                   (byte)QTagMode.DISABLE_QTAG_MODE.getValue()};
    if (qMode == QTagMode.DISABLE_QTAG_MODE) {
      qtag[Q_TAG_MODE_INDEX] = (byte)QTagMode.DISABLE_QTAG_MODE.getValue();
    } else if (qMode == QTagMode.ENABLE_QTAG_ONLY_MODE) {
      qtag[Q_TAG_MODE_INDEX] = (byte)QTagMode.ENABLE_QTAG_ONLY_MODE.getValue();
    } else if (qMode == QTagMode.APPEND_QTAG_MODE) {
      qtag[Q_TAG_MODE_INDEX] = (byte)QTagMode.APPEND_QTAG_MODE.getValue();
    } else {
      NxpNfcLogger.e(TAG, "Invalid mode");
    }

    try {
      NxpNfcLogger.d(TAG, "Sending VendorNciMessage");
      byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          qtag);

      boolean isSuccess =
          vendorRsp != null && vendorRsp.length > 1 &&
          vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS;
      if (!isSuccess) {
        NxpNfcLogger.e(TAG, "enableQtag failed!!");
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
  }

  public @DualAntennaStatus
  int setDiscoveryTechnology_DualAntenna(int antennaOneTech, int antennaTwoTech)
      throws IOException {

    int pollTech;
    try {
      NxpNfcLogger.d(TAG, "setDiscoveryTechnology_DualAntenna framework Enter");
      Set<Integer> validValues = Set.of(RfPollingStatus.passive_ABF.value,
                                        RfPollingStatus.passive_ABFQK.value,
                                        RfPollingStatus.passive_Q.value,
                                        RfPollingStatus.polling_disable.value);
      if ((!validValues.contains(antennaOneTech)) ||
          (!validValues.contains(antennaTwoTech))) {
        NxpNfcLogger.e(TAG, "Invalid parameters");
        return DualAntennaStatusCode.Failed.value;
      }
      byte[] payload = {(byte)(DUAL_ANTENNA_SUB_GID_OID |
                               DualAntennaSubOid.setDiscoveryTechnology.value),
                        (byte)antennaOneTech, (byte)antennaTwoTech};

      byte[] setDiscoveryTechnology = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          payload);
      if (setDiscoveryTechnology != null && setDiscoveryTechnology.length > 0 &&
          setDiscoveryTechnology[1] ==
              NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
        antennaOneConfiguration = antennaOneTech;
        antennaTwoConfiguration = antennaTwoTech;
        if ((antennaOneTech == RfPollingStatus.passive_ABFQK.value) ||
            (antennaTwoTech == RfPollingStatus.passive_ABF.value)) {
          pollTech = RfPollingStatus.passive_ABFQK.value;
        } else if ((antennaOneTech == RfPollingStatus.passive_Q.value) &&
                   (antennaTwoTech == RfPollingStatus.passive_Q.value)) {
          pollTech = RfPollingStatus.passive_Q.value;
        } else if ((antennaOneTech == RfPollingStatus.passive_ABF.value) &&
                   (antennaTwoTech == RfPollingStatus.passive_ABF.value)) {
          pollTech = RfPollingStatus.passive_ABF.value;
        } else if ((antennaTwoTech == RfPollingStatus.passive_ABFQK.value) ||
                   (antennaOneTech == RfPollingStatus.passive_ABF.value)) {
          pollTech = RfPollingStatus.passive_ABFQK.value;
        } else if ((antennaOneTech == RfPollingStatus.passive_Q.value) ||
                   (antennaTwoTech == RfPollingStatus.passive_Q.value)) {
          pollTech = RfPollingStatus.passive_Q.value;
        } else {
          return DualAntennaStatusCode.Failed.value;
        }
        NxpNfcLogger.i(TAG, " setDiscoveryTechnology_DualAntenna PollTech  " + pollTech);
        switch (pollTech) {
        case 0x05: {
          enableQTagDualAntenna(APPEND_Q_POLL);

        } break;
        case 0x0D: {
          enableQTagDualAntenna(ONLY_Q_POLL);
        } break;
        case 0x01: {
          mNfcOperations.setDiscoveryTech(
              NfcAdapter.FLAG_LISTEN_NFC_PASSIVE_A |
                  NfcAdapter.FLAG_LISTEN_NFC_PASSIVE_B |
                  NfcAdapter.FLAG_LISTEN_NFC_PASSIVE_F,
              NfcAdapter.FLAG_LISTEN_DISABLE);
        } break;
        default:
          NxpNfcLogger.d(TAG, "Unknown message received");
          break;
        }
        return DualAntennaStatusCode.Success.value;
      } else {
        return DualAntennaStatusCode.Failed.value;
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
  }

  public @DualAntennaStatus int
  setPollingMode_DualAntenna(@ReaderModeStatus int ant1,
                             @ReaderModeStatus int ant2) throws IOException {

    NxpNfcLogger.d(TAG, "setPollingMode_DualAntenna framework Enter new");
    int readerMode = 0x00;
    if (ant1 == ReaderMode.Enable.value && ant2 == ReaderMode.Enable.value) {
      readerMode = ConDiscParam.EnableBothAnt.value;
    } else if (ant1 == ReaderMode.Enable.value &&
               ant2 == ReaderMode.Disable.value) {
      readerMode = ConDiscParam.EnableAnt1.value;
    } else if (ant1 == ReaderMode.Disable.value &&
               ant2 == ReaderMode.Enable.value) {
      readerMode = ConDiscParam.EnableAnt2.value;
    } else {
      NxpNfcLogger.d(TAG, "only card mode enabled");
    }
    byte[] payload = {(byte)(DUAL_ANTENNA_SUB_GID_OID |
                             DualAntennaSubOid.enableReaderMode.value),
                      (byte)readerMode};
    try {
      byte[] enableReaderMode = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          payload);
      if (enableReaderMode != null && enableReaderMode.length > 0 &&
          enableReaderMode[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
        antennaPolling = readerMode;
        return DualAntennaStatusCode.Success.value;
      } else {
        return DualAntennaStatusCode.Failed.value;
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
  }

  public @DualAntennaStatus int[] getDiscoveryTechnology_DualAntenna()
      throws IOException {
    NxpNfcLogger.d(TAG,
                   "getDiscoveryTechnology_DualAntenna framework Enter new");
    int[] antennaConf = new int[2];
    antennaConf[0] = antennaOneConfiguration;
    antennaConf[1] = antennaTwoConfiguration;
    return antennaConf;
  }

  public @DualAntennaStatus int getPollingMode_DualAntenna()
      throws IOException {
    NxpNfcLogger.d(TAG, "getPollingMode_DualAntenna framework Enter new");
    return antennaPolling;
  }
}
