package com.nxp.nfc.vendor.fw;

import android.nfc.NfcAdapter;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;

import java.io.IOException;
import java.util.Arrays;

public class NfcFirmwareInfo {
  private NfcAdapter mNfcAdapter;
  private final NxpNciPacketHandler mNxpNciPacketHandler;

  private static final String TAG = "NfcFirmwareInfo";
  public static final int GET_FW_VERSION = 0x9F;

  public NfcFirmwareInfo(NfcAdapter nfcAdapter) {
      this.mNfcAdapter = nfcAdapter;
      this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
  }

  public byte[] getFwVersion() throws IOException {
    NxpNfcLogger.d(TAG, "getFwVersion Enter:");

    byte[] fwVersion =  new byte[3];
    Arrays.fill(fwVersion, (byte)0);
    byte[] getFwVerPayload = {(byte)GET_FW_VERSION, 0x00};

    try {
      NxpNfcLogger.d(TAG, "Sending VendorNciMessage");
      byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
          NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
          getFwVerPayload);
      if (vendorRsp != null && vendorRsp.length > 0 &&
          vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
          if (vendorRsp.length >= 5) {
            fwVersion = new byte[3];
            fwVersion =  Arrays.copyOfRange(vendorRsp, 2, 5);
            NxpNfcLogger.d(TAG, "Current FW version is : " + String.format("%02X", vendorRsp[2]) +
                            "." + String.format("%02X", vendorRsp[3]) + "." +
                            String.format("%02X", vendorRsp[4]));
          } else {
            NxpNfcLogger.e(TAG, "Invalid response!!");
          }
      } else {
          NxpNfcLogger.e(TAG, "getFwVersion failed!!");
      }
    } catch (Exception e) {
      NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
      throw new IOException("Error sending VendorNciMessage", e);
    }
    return fwVersion;
  }
}
