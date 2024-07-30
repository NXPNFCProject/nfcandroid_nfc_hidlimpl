/*
 * Copyright 2024 NXP
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
package com.nxp.nfc.oem;

import java.util.HashMap;

import android.app.Activity;
import android.nfc.NfcAdapter;

import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.INxpNfcExtras;
import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.core.NxpNciPacketHandler;
import com.nxp.nfc.core.NfcOperations;
import com.nxp.nfc.oem.stag.STagConstants;
import com.nxp.nfc.oem.stag.STagManager;

/**
 * @class NxpNfcExtras
 * @brief Concrete implementation of OEM Extension features
 *
 * @hide
 */
public class NxpNfcExtras implements INxpNfcExtras, INxpNfcNtfHandler {

    public static final String TAG = "NxpNfcExtras";

    private NfcAdapter mNfcAdapter;

    private NxpNciPacketHandler mNxpNciPacketHandler;
    private STagManager mSTagManager;
    private NfcOperations mNfcOperations;

    private HashMap<Integer, INxpNfcNtfHandler> mFeatMap = new HashMap<>();

    public NxpNfcExtras(NfcAdapter nfcAdapter, Activity activity) {
        this.mNfcAdapter = nfcAdapter;
        mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(mNfcAdapter, this);
        mNfcOperations = NfcOperations.getInstance(mNfcAdapter, activity);
        mSTagManager = new STagManager(mNxpNciPacketHandler, mNfcOperations);

        mFeatMap.put(STagConstants.AUTH_ERR_NTF_SUB_GID_OID, mSTagManager);
    }

    @Override
    public void coverAttached(boolean attached, int coverType) {
        NxpNfcLogger.d(TAG, "coverAttached:");
    }

    @Override
    public byte[] startCoverAuth() {
        NxpNfcLogger.d(TAG, "startCoverAuth:");
        return mSTagManager.startCoverAuth();
    }

    @Override
    public boolean stopCoverAuth() {
        NxpNfcLogger.d(TAG, "stopCoverAuth:");
        return mSTagManager.stopCoverAuth();
    }

    @Override
    public byte[] transceiveAuthData(byte[] data) {
        NxpNfcLogger.d(TAG, "transceiveAuthData:");
        return mSTagManager.transceiveAuthData(data);
    }

    @Override
    public void onVendorNciNotification(int gid, int oid, byte[] payload) {
        NxpNfcLogger.d(TAG, "onVendorNciNotification:");
        if (mFeatMap.containsKey(payload[0])) {
            mFeatMap.get(oid).onVendorNciNotification(gid, oid, payload);
        } else {
            NxpNfcLogger.d(TAG, "No OID Found, VendorNciNotification Ignored!");
        }
    }
}
