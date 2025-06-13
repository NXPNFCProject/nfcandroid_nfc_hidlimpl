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

package com.nxp.nfc.vendor.utils;

import android.content.Context;
import android.nfc.NfcAdapter;

import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.core.NxpNciPacketHandler;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

/**
 * This class is responsible to control utils api calls
 */
public class UtilsHandler  {

    private static final String TAG = "UtilsHandler";
    private final NfcOperations mNfcOperations;
    private final NxpNciPacketHandler mNxpNciPacketHandler;
    private final Context mContext;
    private final byte ACTIVATE_SE = 0x03;
    private final byte MODE_SET_ON = 0x01;
    private final byte ESE1_ID = (byte) 0xC0;
    private final String ESE_STR = "eSE";

    private final byte STATUS_FAILED = 0x03;
    private final byte STATUS_SUCCESS = 0x00;
    private final byte STATUS_SERVICE_UNAVAILABLE = (byte) 0xFF;
    private final Map<String, Byte> configuredSEList;

    public UtilsHandler(NfcAdapter nfcAdapter, Context context) {
        NxpNfcLogger.d(TAG, "Entry UtilsHandler");
        this.mContext = context;
        this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
        this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
        configuredSEList =  new HashMap<String, Byte>();
        configuredSEList.put(ESE_STR + 1, ESE1_ID);
    }

    /**
     * This api is called by applications to Activate Secure Element Interface.
     * <p>Requires {@link android.Manifest.permission#NFC} permission.<ul>
     * <li>This api shall be called only Nfcservice is enabled.
     * </ul>
     * @return whether  the update of configuration is
     *          success or not.
     *          0x03 - failure
     *          0x00 - success
     *          0xFF - Service Unavailable
     * @throws  IOException if any exception occurs during setting the NFC
     * configuration.
     */
    public int activateSeInterface() throws IOException {
        NxpNfcLogger.d(TAG, "Entry activateSeInterface");
        return setSeMode(true);
    }

    /**
     * This api is called by applications to Deactivate Secure Element Interface.
     * <p>Requires {@link android.Manifest.permission#NFC} permission.<ul>
     * <li>This api shall be called only Nfcservice is enabled.
     * </ul>
     * @return whether  the update of configuration is
     *          success or not.
     *          0x03 - failure
     *          0x00 - success
     *          0xFF - Service Unavailable
     * @throws  IOException if any exception occurs during setting the NFC
     * configuration.
     */
    public int deactivateSeInterface() throws IOException {
        NxpNfcLogger.d(TAG, "Entry deactivateSeInterface");
        return setSeMode(false);
    }

    /**
     * This API is called by application to stop RF discovery
     * <p>Requires {@link android.Manifest.permission#NFC} permission.
     * <li>This api shall be called only Nfcservice is enabled.
     * </ul>
     * @return None
     * @throws IOException If a failure occurred during stop discovery
    */
    public void stopPoll() throws IOException {
        NxpNfcLogger.d(TAG, "Entry stopPoll");
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
    }

    /**
     * This API is called by application to start RF discovery
     * <p>Requires {@link android.Manifest.permission#NFC} permission.
     * <li>This api shall be called only Nfcservice is enabled.
     * </ul>
     * @return None
     * @throws IOException If a failure occurred during start discovery
    */
    public void startPoll() throws IOException {
        NxpNfcLogger.d(TAG, "Entry startPoll");
        mNfcOperations.enableDiscovery();
    }

    private int setSeMode(boolean active) throws IOException {
        NxpNfcLogger.d(TAG, "Entry setSeMode Active : " + active);
        if (mNfcOperations == null || !mNfcOperations.isEnabled()) {
            return STATUS_SERVICE_UNAVAILABLE;
        }
        byte gid = 0x22;
        byte powerLinkOid = 0x03;
        byte modeSetOid   = 0x01;
        boolean status = false;

        Map<String, Integer> eeList = mNfcOperations.getActiveEEList();
        Map<String, Integer> seList = eeList.entrySet()
            .stream()
            .filter(entry -> entry.getKey().startsWith(ESE_STR))
            .collect(Collectors.toMap(Map.Entry::getKey, Map.Entry::getValue));
        if (0 >= seList.size()) {
            NxpNfcLogger.i(TAG, "No eSE  Found");
            return STATUS_FAILED;
        }
        for (Map.Entry<String, Integer> se : seList.entrySet()) {
            NxpNfcLogger.e(TAG, "SE " + se.getKey());
            Byte seId = configuredSEList.get(se.getKey());
            if (seId == null) {
                NxpNfcLogger.i(TAG, " Ignoring " + se.getKey() + " not supported");
                continue;
            }
            byte[] cmdPayload = {seId, 0x01};
            if (active) {
                cmdPayload[cmdPayload.length - 1] = ACTIVATE_SE;
            }
            status = sendCommand(gid, powerLinkOid, cmdPayload);
            if (active && status) {
                cmdPayload[cmdPayload.length - 1] = MODE_SET_ON;
                status = sendCommand(gid, modeSetOid, cmdPayload);
            }
            if (!status) {
                NxpNfcLogger.e(TAG, "Failed for " + se.getKey());
                return STATUS_FAILED;
            }
        }
        return STATUS_SUCCESS;
    }

    private boolean sendCommand(byte gid, byte oid, byte[] cmd) throws IOException {
        mNxpNciPacketHandler.shouldCheckResponseSubGid(false);
        byte[] vendorRsp = mNxpNciPacketHandler.sendVendorNciMessage(gid,
                oid, cmd);
        mNxpNciPacketHandler.shouldCheckResponseSubGid(true);
        if (vendorRsp != null
                && vendorRsp.length > 0
                && vendorRsp[0] == NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
            NxpNfcLogger.d(TAG, "Send Success");
            return true;
        }
        return false;
    }
}
