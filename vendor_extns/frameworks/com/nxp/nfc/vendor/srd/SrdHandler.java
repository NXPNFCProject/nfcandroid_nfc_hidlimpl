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
package com.nxp.nfc.vendor.srd;

import android.content.Context;
import android.content.Intent;
import android.nfc.NfcAdapter;
import com.nxp.nfc.INxpNfcAdapter;
import com.nxp.nfc.INxpNfcAdapter.SRDStatus;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;
import com.nxp.nfc.INxpOEMCallbacks;
import java.util.concurrent.Executors;
import android.os.UserHandle;
import android.util.Log;
import android.net.Uri;
import java.io.IOException;

/**
* This class is responsible to start/stop the Srd reader and
* handles the srd action notfications
*/
public class SrdHandler implements INxpNfcNtfHandler, INxpOEMCallbacks  {

    public static final byte SRD_MODE_NTF_SUB_GID_OID = (byte) 0x23;
    /**
     * srd mode status
     */

    public static final int SRD_MODE_NTF_START_SRD_DISCOVERY = 0x00;
    public static final int SRD_MODE_NTF_SRD_TIMED_OUT = 0x01;
    public static final int SRD_MODE_NTF_SRD_FEATURE_NOT_SUPPORTED = 0x02;
    public static final int SRD_MODE_NTF_SRD_TRANSACTION_STOP = 0x03;
    public static final int SRD_INIT_MODE = 0x20;
    public static final int ACTIVE_SE = 0x21;
    public static final int DEACTIVE_SE = 0x22;

    private static final byte AID_TAG = (byte) 0x81;
    private static final byte DATA_TAG = (byte) 0x82;
    private static final byte SUB_GID_OID_LEN = 0x02;
    private static final byte NCI_HEADER_LEN = 0x02;
    private static final byte HCI_HEADER_LEN = 0x02;

    public static final int STATUS_SUCCESS = 0x00;
    public static final int STATUS_FAILED = 0x03;
    private static final String TAG = "SrdHandler";
    private static final String ESE1_STR = "eSE1";
    private static boolean isSrdEnabled = false;
    private final NxpNciPacketHandler mNxpNciPacketHandler;
    private final NfcOperations mNfcOperations;
    private final Context mContext;
    private ISrdCallbacks mSrdCallbacks = null;
       /*SRD EVT Timeout*/
    private static final String ACTION_SRD_EVT_TIMEOUT =
            "com.nxp.nfc_extras.ACTION_SRD_EVT_TIMEOUT";
    /*SRD Feature not supported */
    private static final String ACTION_SRD_EVT_FEATURE_NOT_SUPPORT =
            "com.nxp.nfc_extras.ACTION_SRD_EVT_FEATURE_NOT_SUPPORT";

    public SrdHandler(NfcAdapter nfcAdapter, Context context) {
        this.mContext = context;
        this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
        this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
        mSRDStarted(false);
    }

    public boolean isSrdModeEnabled() {
        return isSrdEnabled;
    }

    public void mSRDStarted(boolean enabled) {
        isSrdEnabled = enabled;
    }

    @Override
    public void onVendorNciNotification(int gid, int oid, byte[] payload) {
        if (payload == null || payload.length < 2) {
            NxpNfcLogger.d(TAG, "Invalid payload");
            return;
        }

        byte subGidOid = payload[0];
        byte notificationType = payload[1];
        NxpNfcLogger.d(TAG, "Sub-GidOid: " + subGidOid + ", Notification Type: " + notificationType);

        if (subGidOid == SRD_MODE_NTF_SUB_GID_OID) {
            handleSrdNotification(notificationType, payload);
        }
    }

    private void handleSrdNotification(byte notificationType, byte[] payload) {
        int ntfType = Byte.toUnsignedInt(notificationType);
        NxpNfcLogger.d(TAG, "ntfType:" + ntfType);
        switch (ntfType) {
            case SRD_MODE_NTF_START_SRD_DISCOVERY:
                NxpNfcLogger.d(TAG, "ACTION_NFC_SRD_START_RF_DISCOVERY");
                break;
            case SRD_MODE_NTF_SRD_TIMED_OUT:
                NxpNfcLogger.d(TAG, "SRD_EVT_TIMEOUT");
                mNfcOperations.disableDiscovery();
                isSrdEnabled = false;
                if (mSrdCallbacks != null) {
                    NxpNfcLogger.d(TAG, "Sending SRD Callabck to Application");
                    mSrdCallbacks.onSrdTimedout();
                } else {
                    NxpNfcLogger.i(TAG, "No callback registered for SRD");
                }
                if (mContext != null) {
                    NxpNfcLogger.d(TAG, "Broadcasting " + ACTION_SRD_EVT_TIMEOUT);
                    Intent srdTimeoutIntent = new Intent(ACTION_SRD_EVT_TIMEOUT);
                    mContext.sendBroadcast(srdTimeoutIntent);
                }
                mNfcOperations.unregisterNxpOemCallback();
                break;
            case SRD_MODE_NTF_SRD_FEATURE_NOT_SUPPORTED:
                if (mSrdCallbacks != null) {
                    NxpNfcLogger.d(TAG, "Sending SRD_FEATURE_NOT_SUPPORTED Callabck to Application");
                    mSrdCallbacks.onSrdFeatureSupport(false);
                } else {
                    NxpNfcLogger.e(TAG, "No callback registered for SRD");
                }
                if (mContext != null) {
                    NxpNfcLogger.d(TAG, "Broadcasting " + ACTION_SRD_EVT_FEATURE_NOT_SUPPORT);
                    Intent srdFeatureNotSupportedIntent = new Intent(ACTION_SRD_EVT_FEATURE_NOT_SUPPORT);
                    mContext.sendBroadcast(srdFeatureNotSupportedIntent);
                }
                break;
            case SRD_MODE_NTF_SRD_TRANSACTION_STOP:
                handleSrdStopNotification(payload);
                break;
            default:
                NxpNfcLogger.d(TAG, "Unknown message received");
                break;
        }
    }

    private void handleSrdStopNotification(byte[] ntf) {
        try {
            if (mNfcOperations == null || mContext == null || ntf == null) {
                NxpNfcLogger.e(TAG, " Invalid params");
            }
            mNfcOperations.disableDiscovery();
            byte[] reader = ESE1_STR.getBytes(); // consider eSE1 for SRD
            byte[] aid = parseTagValue(ntf, AID_TAG);
            byte[] data = parseTagValue(ntf, DATA_TAG);
            if (aid == null || data == null) {
                NxpNfcLogger.e(TAG, "Invalid AID or Data");
                return;
            }
            StringBuilder aidString = new StringBuilder(aid.length);
            for (byte b : aid) {
                aidString.append(String.format("%02X", b));
            }
            Intent intent = new Intent(NfcAdapter.ACTION_TRANSACTION_DETECTED);
            intent.putExtra(NfcAdapter.EXTRA_AID, aid);
            intent.putExtra(NfcAdapter.EXTRA_DATA, data);
            intent.putExtra(NfcAdapter.EXTRA_SECURE_ELEMENT_NAME, reader);
            String url =
                new String("nfc://secure:0/" + reader + "/" + aidString.toString());
            intent.setData(Uri.parse(url));
            NxpNfcLogger.d(TAG, "Broadcasting " + NfcAdapter.ACTION_TRANSACTION_DETECTED);
            mContext.sendBroadcastAsUser(intent, UserHandle.CURRENT);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Error " + e);
        }
    }

    private byte[] parseTagValue(byte[] ntf, byte inputTag) {
        if (ntf == null || ntf.length < 6) {
            return null;
        }

        int offset = 0;
        offset += SUB_GID_OID_LEN + NCI_HEADER_LEN;
        if (ntf[offset++] > (ntf.length - offset)) {
            return null;
        }
        offset += HCI_HEADER_LEN;
        while ((offset + 2) <= ntf.length) {
            byte tag = ntf[offset++];
            byte tagLen = ntf[offset++];
            if (tag == inputTag && ((offset + tagLen) <= ntf.length)) {
                byte[] data = new byte[tagLen];
                System.arraycopy(ntf, offset, data, 0, tagLen);
                return data;
            }
            offset += tagLen;
        }
        return null;
   }


    /**
     * This API registers the Application callbacks to be called
     * for Srd notifications.
     *
     * @param callbacks : callback to be registered
     */
    public void registerSrdCallbacks(ISrdCallbacks callbacks) {
        NxpNfcLogger.d(TAG, "Entry registerSrdCallbacks");
        mSrdCallbacks = callbacks;
    }

    /**
     * This API unregisters the Application callbacks to be called
     * for Srd notifications.
     */
    public void unregisterSrdCallbacks() {
        NxpNfcLogger.d(TAG, "Entry unregisterSrdCallbacks");
        mSrdCallbacks = null;
    }

    private @SRDStatus int sendSrdVendorNciMessage(byte[] srdCmd) throws IOException {
        try {
            NxpNfcLogger.d(TAG, "Sending SRD cmd through VendorNciMessage");
            byte[] vendorInitRsp = mNxpNciPacketHandler.sendVendorNciMessage(NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID, srdCmd);
            if (vendorInitRsp == null || vendorInitRsp.length < 2) {
                NxpNfcLogger.e(TAG, "Vendor Init is null or Rsp length is less than 2 bytes");
                return INxpNfcAdapter.SRD_STATUS_FAILED;
            }
            if (vendorInitRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                NxpNfcLogger.d(TAG, "SRD CMD success");
            } else {
                NxpNfcLogger.d(TAG, "Wrong VendorNciMessage Response");
                return INxpNfcAdapter.SRD_STATUS_FAILED;
            }
        } catch (Exception e) {
            NxpNfcLogger.d(TAG, "Exception in sendVendorNciMessage");
            return 0xFF;
        }
        return INxpNfcAdapter.SRD_STATUS_SUCCESS;
    }

    public int activateSeInterface() throws IOException {
        NxpNfcLogger.d(TAG, "registerCallback VendorNciMessage on activateSeInterface");
        mNfcOperations.registerNxpOemCallback(this);
        mNxpNciPacketHandler.registerCallback(Executors.newSingleThreadExecutor(), this);

        byte[] activeSECmd = new byte[2];
        activeSECmd[0] = (byte) ACTIVE_SE;
        activeSECmd[1] = 0x01;
        mNxpNciPacketHandler.shouldCheckResponseSubGid(false);
        NxpNfcLogger.d(TAG, "Sending VendorNciMessage to ACTIVE_SE");
        int srdSeStatus = sendSrdVendorNciMessage(activeSECmd);
        mNxpNciPacketHandler.shouldCheckResponseSubGid(true);
        NxpNfcLogger.d(TAG, "srdSeStatus:" + srdSeStatus);
        if (srdSeStatus != INxpNfcAdapter.SRD_STATUS_SUCCESS) {
            return STATUS_FAILED;
        }
        return STATUS_SUCCESS;

    }


    public int deactivateSeInterface() throws IOException {
        NxpNfcLogger.d(TAG, "deactivateSeInterface");
        byte[] deactiveSECmd = new byte[2];
        deactiveSECmd[0] = (byte) DEACTIVE_SE;
        deactiveSECmd[1] = 0x01;
        mNxpNciPacketHandler.shouldCheckResponseSubGid(false);
        NxpNfcLogger.d(TAG, "Sending VendorNciMessage to deactivate SE");
        int seDeactiveStatus = sendSrdVendorNciMessage(deactiveSECmd);
        mNxpNciPacketHandler.shouldCheckResponseSubGid(true);
        NxpNfcLogger.d(TAG, "seDeactiveStatus:" + seDeactiveStatus);
        if (seDeactiveStatus != INxpNfcAdapter.SRD_STATUS_SUCCESS) {
            return STATUS_FAILED;
        }
        return STATUS_SUCCESS;
    }

    public int srdinit() throws IOException {
        NxpNfcLogger.d(TAG, "srdinit");
        byte[] prop_init_cmd = new byte[2];
        prop_init_cmd[0] = (byte) SRD_INIT_MODE;
        prop_init_cmd[1] = 0x01;
        NxpNfcLogger.d(TAG, "Sending VendorNciMessage to init SRD");
        int srdInitStatus = sendSrdVendorNciMessage(prop_init_cmd);
        NxpNfcLogger.d(TAG, "srdInitStatus:" + srdInitStatus);
        if (srdInitStatus != INxpNfcAdapter.SRD_STATUS_SUCCESS) {
            return STATUS_FAILED;
        }
        return STATUS_SUCCESS;
    }
    public void stopPoll() throws IOException {
       NxpNfcLogger.d(TAG, "stopPoll");
       srdinit();
       mNfcOperations.disableDiscovery();
    }

    public void startPoll() throws IOException {
       NxpNfcLogger.d(TAG, "startPoll");
       mNfcOperations.enableDiscovery();
    }

    private void resetSRD() {
        mSRDStarted(false);
        mNfcOperations.unregisterNxpOemCallback();
    }

    @Override
    public boolean onDisableRequested() {
        NxpNfcLogger.d(TAG, "onDisableRequested: ");
        resetSRD();
        return true;
    }

    @Override
    public void onEnableFinished(int status) {
        NxpNfcLogger.d(TAG, "onEnableFinished: ");
        resetSRD();
    }

    @Override
    public void onBootFinished(int status) {
        NxpNfcLogger.d(TAG, "onBootFinished: ");
        resetSRD();
    }
}
