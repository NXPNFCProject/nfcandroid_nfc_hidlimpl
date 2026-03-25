/*
 *
 *  The original Work has been changed by NXP.
 *
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

package com.nxp.wlc.vendor.utils;

import android.content.Context;
import android.nfc.NfcAdapter;

import com.nxp.wlc.core.NfcOperations;
import com.nxp.wlc.core.NxpNciPacketHandler;
import com.nxp.wlc.INxpOEMCallbacks;
import com.nxp.wlc.NxpNfcLogger;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * This class is responsible to control utils api calls
 */
public class UtilsHandler implements INxpOEMCallbacks {

    private static final String TAG = "UtilsHandler";
    private final NfcOperations mNfcOperations;
    private final NxpNciPacketHandler mNxpNciPacketHandler;
    private final Context mContext;

    private final byte STATUS_FAILED = 0x03;
    private final byte STATUS_SUCCESS = 0x00;
    private final byte STATUS_SERVICE_UNAVAILABLE = (byte) 0xFF;
 
    public UtilsHandler(NfcAdapter nfcAdapter, Context context) {
        NxpNfcLogger.d(TAG, "Entry UtilsHandler");
        this.mContext = context;
        this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
        this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
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
        mNfcOperations.registerNxpOemCallback(this);
        if (mNfcOperations.isDiscoveryStarted()) {
            mNfcOperations.disableDiscovery();
        }
        mNfcOperations.unregisterNxpOemCallback();
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
