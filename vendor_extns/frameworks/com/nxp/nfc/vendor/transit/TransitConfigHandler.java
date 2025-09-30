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
import com.nxp.nfc.INxpOEMCallbacks;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class TransitConfigHandler implements INxpOEMCallbacks {
    private NfcAdapter mNfcAdapter;
    private final NfcOperations mNfcOperations;
    private final NxpNciPacketHandler mNxpNciPacketHandler;

    private static final int DISABLE_TRANSIT = 0x00;
    private static final int TRANSIT_CONFIG_REQUIRE_NFC_RESET = 0x01;
    private static final int TRANSIT_CONFIG_REQUIRE_RF_RESET = 0x02;
    private static final int TRANSIT_SETCONFIG_STAT_FAILED  = 0xFF;
    private static final int TRANSIT_CONFIG_SUB_GID = 0x70;
    private static final int TRANSIT_CONFIG_SUB_OID = 0x01;
    private static final int RF_REGISTER_SUB_OID = 0x02;
    private static final int MAX_CONFIG_LEN = 249;
    private static final int SUB_GIDOID_INDEX = 0x00;
    private static final int PAYLOAD_LEN_INDEX = 0x01;
    private static final int PBF_INDEX = 0x02;
    private static final String TAG = "TransitConfigHandler";

    public TransitConfigHandler(NfcAdapter nfcAdapter) {
        this.mNfcAdapter = nfcAdapter;
        this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
        this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
    }

    private int checkResetRequired(String configs) {
        NxpNfcLogger.d(TAG, "checkResetRequired configs: " + configs);
        if (configs == null) {
            return TRANSIT_CONFIG_REQUIRE_NFC_RESET;
        }
        if (configs.startsWith("UPDATE_")) {
            String[] lines = configs.split("\n");
            for (String token : lines) {
                if (!token.startsWith("UPDATE_")) {
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
                if (token.startsWith("UPDATE_")) {
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

    private boolean sendCmd(byte[] payloadBytes) throws IOException {
        byte[] vendorRsp = {};
        try {
            NxpNfcLogger.d(TAG, "Sending VendorNciMessage");
            vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(
                NxpNfcConstants.NFC_NCI_PROP_GID, NxpNfcConstants.NXP_NFC_PROP_OID,
                payloadBytes);
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Exception in sendVendorNciMessage");
            throw new IOException("Error in sendVendorNciMessage", e);
        }
        if ((vendorRsp != null) && (vendorRsp.length > 1)
                && (vendorRsp[1] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS)) {
            return true;
        } else {
            return false;
        }
    }

    private boolean clearConfig(byte subGidOid) throws IOException {
        Boolean clearConfigStatus = false;
        try {
            byte[] emptyPayload = {subGidOid, DISABLE_TRANSIT};
            clearConfigStatus = sendCmd(emptyPayload);
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Exception in clearConfig");
            throw new IOException("Error in clearConfig", e);
        }
        return clearConfigStatus;
    }

    private boolean handleSegmentMsg(byte[] configBytes, byte subGidOid) throws IOException {
        boolean nciCmdStatus = false;
        int startIndex = 0;
        int endIndex = 0;
        byte[] cmdBytes = {subGidOid, 0x00, 0x00};
        while (startIndex < configBytes.length) {
            int pbf = 1;
            endIndex = startIndex + MAX_CONFIG_LEN;
            if (endIndex >= configBytes.length) {
                pbf = 0;
                endIndex = configBytes.length;
            }
            byte[] partialPayload = Arrays.copyOfRange(configBytes, startIndex, endIndex);
            startIndex = endIndex;

            cmdBytes[PAYLOAD_LEN_INDEX] = (byte) (partialPayload.length + 1);
            cmdBytes[PBF_INDEX] = (byte) pbf;
            byte[] payloadBytes = generateCmd(cmdBytes, partialPayload);
            try {
                if (payloadBytes.length == 0) {
                    nciCmdStatus = false;
                } else {
                    nciCmdStatus = sendCmd(payloadBytes);
                }
                if (!nciCmdStatus)
                    return nciCmdStatus;
            } catch (Exception e) {
                NxpNfcLogger.e(TAG, "Exception in handleSegmentMsg");
                throw new IOException("Error in handleSegmentMsg", e);
            }
        }
        return nciCmdStatus;
    }

    private boolean handlRfRegisterConfig(String configs) throws IOException {
        Boolean cmdStatus = false;

        if (configs == null) {
            NxpNfcLogger.e(TAG, "Invalid RF Register config");
            return false;
        }
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
        NxpNfcLogger.d(TAG, "Updating RF Register config");
        byte[] configBytes = configs.getBytes(StandardCharsets.UTF_8);
        byte subGidOid = (byte) (TRANSIT_CONFIG_SUB_GID | RF_REGISTER_SUB_OID);
        byte[] cmdBytes = {subGidOid, 0x00, 0x00};

        try {
            if (configBytes.length > MAX_CONFIG_LEN) {
                cmdStatus = handleSegmentMsg(configBytes, subGidOid);
            } else {
                cmdBytes[PAYLOAD_LEN_INDEX] = (byte) configBytes.length;
                byte[] payloadBytes = generateCmd(cmdBytes, configBytes);
                if (payloadBytes.length == 0) {
                    mNfcOperations.enableDiscovery();
                    return false;
                }
                cmdStatus = sendCmd(payloadBytes);
            }
            if(!cmdStatus) {
                NxpNfcLogger.e(TAG, "Error in handlRfRegisterConfig, clearing stored config...");
                clearConfig(subGidOid);
            }
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Exception in handlRfRegisterConfig");
            clearConfig(subGidOid);
            throw new IOException("Error in handlRfRegisterConfig", e);
        } finally {
            mNfcOperations.enableDiscovery();
        }
        return cmdStatus;
    }

    private boolean handleLibNfcConfig(String configs) throws IOException {
        boolean cmdStatus = false;
        byte subGidOid = (byte) (TRANSIT_CONFIG_SUB_GID | TRANSIT_CONFIG_SUB_OID);

        try {
            if (configs == null) {
                NxpNfcLogger.d(TAG, "Removing libnfc-nci-update.conf");
                cmdStatus = clearConfig(subGidOid);
            } else {
                NxpNfcLogger.d(TAG, "Updating libnfc-nci-update.conf");
                byte[] configBytes = configs.getBytes(StandardCharsets.UTF_8);
                byte[] cmdBytes = {subGidOid, 0x00, 0x00};
                if (configBytes.length > MAX_CONFIG_LEN) {
                    cmdStatus = handleSegmentMsg(configBytes, subGidOid);
                } else {
                    cmdBytes[PAYLOAD_LEN_INDEX] = (byte) configBytes.length;
                    byte[] payloadBytes = generateCmd(cmdBytes, configBytes);
                    cmdStatus = sendCmd(payloadBytes);
                }
            }

            if (!cmdStatus) {
                NxpNfcLogger.e(TAG, "Error in handleLibNfcConfig, clearing stored config...");
                clearConfig(subGidOid);
                return cmdStatus;
            }
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Exception in handleLibNfcConfig");
            clearConfig(subGidOid);
            throw new IOException("Error handleLibNfcConfig", e);
        } finally {
            try {
                mNfcAdapter.disable();
                mNfcAdapter.enable();
            } catch (Exception e) {
                NxpNfcLogger.e(TAG, "Exception in NFC_RESET");
                throw new IOException("Error in NFC_RESET", e);
            }
        }
        return cmdStatus;
    }

    public boolean setConfig(String configs) throws IOException {
        mNfcOperations.registerNxpOemCallback(this);
        if ((!mNfcOperations.isEnabled()) || (mNfcOperations.isCardEmulationActivated())
                || (mNfcOperations.isTagConnected()) || mNfcOperations.isEeListenActivated()) {
            NxpNfcLogger.e(TAG, "NFC is disabled or busy, Rejecting request..");
            mNfcOperations.unregisterNxpOemCallback();
            return false;
        }

        boolean updateStatus = false;
        int resetStatus = checkResetRequired(configs);
        try {
            switch (resetStatus) {
                case TRANSIT_CONFIG_REQUIRE_NFC_RESET:
                    updateStatus = handleLibNfcConfig(configs);
                    break;

                case TRANSIT_CONFIG_REQUIRE_RF_RESET:
                    updateStatus = handlRfRegisterConfig(configs);
                    break;

                default:
                    NxpNfcLogger.e(TAG, "Invalid config");
                    break;
            }
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Exception in setConfig");
            throw new IOException("Error in setConfig", e);
        }

        if (!updateStatus) {
            NxpNfcLogger.e(TAG, "setConfig() failed...");
        }
        mNfcOperations.unregisterNxpOemCallback();
        return updateStatus;
    }
}
