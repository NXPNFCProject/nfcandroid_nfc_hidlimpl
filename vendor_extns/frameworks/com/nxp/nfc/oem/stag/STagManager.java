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
package com.nxp.nfc.oem.stag;

import com.nxp.nfc.oem.stag.STagConstants.CmdType;

import java.nio.ByteBuffer;
import java.util.Arrays;

import com.nxp.nfc.core.NxpNciPacketHandler;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.NxpNfcUtils;
/**
 * @hide
 */
public class STagManager implements INxpNfcNtfHandler {

    private final String TAG = "STagManager";
    private NxpNciPacketHandler mNxpNciPacketHandler;
    private NfcOperations mNfcOperations;
    private int mCurrentStatus = STagConstants.STATUS_NOT_STARTED;
    boolean mIsControllerAlwaysOn = false;

    public STagManager(NxpNciPacketHandler NxpNciPacketHandler, NfcOperations nfcOperations) {
        this.mNxpNciPacketHandler = NxpNciPacketHandler;
        this.mNfcOperations = nfcOperations;
    }

    /**
     * @brief sned the startcover auth command to nfcc
     * disable the discovery if it is enabled
     * @throws UnsupportedOperationException if FEATURE_NFC,
     * FEATURE_NFC_HOST_CARD_EMULATION, FEATURE_NFC_HOST_CARD_EMULATION_NFCF,
     * FEATURE_NFC_OFF_HOST_CARD_EMULATION_UICC and FEATURE_NFC_OFF_HOST_CARD_EMULATION_ESE
     * are unavailable
     */
    public byte[] startCoverAuth() {
        byte status = STagConstants.STATUS_FAILED;
        byte[] resp = {status};
        mIsControllerAlwaysOn = false;

        if (mNfcOperations.isRfFieldActivated()) {
            resp[STagConstants.STATUS_INDEX] = STagConstants.STATUS_FIELD_DETECTED;
            NxpNfcLogger.d(TAG, "RfFieldActivated: " + resp[STagConstants.STATUS_INDEX]);
            return resp;
        }

        if (mCurrentStatus == STagConstants.STATUS_STARTED) {
            resp[STagConstants.STATUS_INDEX] = STagConstants.STATUS_REJECTED;
            NxpNfcLogger.d(TAG, "Already Started!");
            return resp;
        }

        if (!mNfcOperations.isEnabled()) {
            mNfcOperations.setControllerAlwaysOn(true);
            if (!mNfcOperations.isControllerAlwaysOn()) {
                NxpNfcLogger.d(TAG, "Controller AlwaysOn Failed!");
                return resp;
            }
            mIsControllerAlwaysOn = true;
        } else {
            if (mNfcOperations.isDiscoveryStarted()) {
                mNfcOperations.disableDiscovery();
            }
        }

        for (int i = 0; i < STagConstants.MAX_STAG_RETRY; i++) {
            resp = sendStartPollCommand();
            status = resp[resp.length >= STagConstants.MAINLINE_RES_MIN_LEN ?
                        STagConstants.MAINLINE_STATUS_INDEX : STagConstants.STATUS_INDEX];

            NxpNfcLogger.d(TAG, "Retry cnt: " + i + ", status: "
                            + NxpNfcUtils.toHexString(resp));
            switch (status) {
                case STagConstants.NFC_STAG_STATUS_BYTE_0:
                case STagConstants.NFC_STAG_STATUS_BYTE_1:
                case STagConstants.NFC_STAG_STATUS_BYTE_2:
                case STagConstants.AUTH_STATUS_EFD_ON:
                    sendStopPollCommand();
                    break;
                default:
                    NxpNfcLogger.d(TAG, "startCoverAuth default status: " + (byte) status);
                    break;
            }
            if (status == STagConstants.STATUS_STARTED) {
                break;
            }
        }
        if (status == STagConstants.STATUS_STARTED) {
            NxpNfcLogger.d(TAG, "startCoverAuth STARTED " + status);
            mCurrentStatus = STagConstants.STATUS_STARTED;
            byte[] subGidOidExclResp;
            if (resp.length >= STagConstants.MAINLINE_RES_MIN_LEN) {
                subGidOidExclResp = Arrays.copyOfRange(resp, STagConstants.MAINLINE_STATUS_INDEX,
                                        resp.length);
                resp = subGidOidExclResp;
            }
        } else {
            NxpNfcLogger.e(TAG, "startCoverAuth Failed " + status);
            resp[STagConstants.STATUS_INDEX] = STagConstants.STATUS_FAILED;
            stagFinalize();
        }
        return resp;
    }

    private byte[] sendStartPollCommand() {
        return sendAuthPropCmd(CmdType.START_POLL.ordinal());
    }

    public byte[] transceiveAuthData(byte[] data) {
        int gid = NxpNfcConstants.NFC_NCI_PROP_GID, oid = NxpNfcConstants.NXP_NFC_PROP_OID;
        byte[] payload = { STagConstants.AUTH_TRANS_SUB_GID_OID };
        ByteBuffer buff = ByteBuffer.allocate(payload.length + data.length);
        buff.put(payload);
        buff.put(data);
        byte[] resp = sendAuthPropCmd(gid, oid, buff.array());
        byte[] subGidOidExclResp;
        if (resp.length >= STagConstants.MAINLINE_RES_MIN_LEN) {
            subGidOidExclResp = Arrays.copyOfRange(resp, STagConstants.MAINLINE_STATUS_INDEX,
                                    resp.length);
        } else {
            subGidOidExclResp = resp;
        }
        return subGidOidExclResp;
    }

    public boolean stopCoverAuth() {
        byte[] resp = sendStopPollCommand();
        if (resp.length >= STagConstants.MAINLINE_RES_MIN_LEN
                && resp[STagConstants.MAINLINE_STATUS_INDEX]
                == STagConstants.STATUS_SUCCESS) {
            mCurrentStatus = STagConstants.STATUS_NOT_STARTED;
            stagFinalize();
            return true;
        } else {
            return false;
        }
    }

    private void stagFinalize() {
        if (mIsControllerAlwaysOn) {
            mNfcOperations.setControllerAlwaysOn(false);
        } else {
            mNfcOperations.enableDiscovery();
        }
    }

    private byte[] sendStopPollCommand() {
        return sendAuthPropCmd(CmdType.STOP_POLL.ordinal());
    }

    private byte[] sendAuthPropCmd(int cmd) {
        int gid = NxpNfcConstants.NFC_NCI_PROP_GID, oid = NxpNfcConstants.NXP_NFC_PROP_OID;
        byte[] payload = {};
        CmdType cmdType = CmdType.values()[cmd];
        switch(cmdType) {
            case START_POLL:
                payload = new byte[] { STagConstants.AUTH_START_SUB_GID_OID };
                break;
            case STOP_POLL:
                payload = new byte[] { STagConstants.AUTH_STOP_SUB_GID_OID };
                break;
            default:
                break;
        }
        return sendAuthPropCmd(gid, oid, payload);
    }

    private byte[] sendAuthPropCmd(int gid, int oid, byte[] payload) {
        return mNxpNciPacketHandler.sendVendorNciMessage(gid, oid, payload);
    }

    @Override
    public void onVendorNciNotification(int gid, int oid, byte[] payload) {
        NxpNfcLogger.d(TAG, "onVendorNciNotification:");
    }
}
