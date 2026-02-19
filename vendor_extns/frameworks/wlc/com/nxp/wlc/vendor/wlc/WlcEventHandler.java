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

package com.nxp.wlc.vendor.wlcinterface;


import android.content.Context;
import android.nfc.NfcAdapter;
import java.util.concurrent.Executors;
import java.util.Arrays;
import android.util.Log;

import com.nxp.wlc.INxpNfcNtfHandler;
import com.nxp.wlc.INxpOEMCallbacks;
import com.nxp.wlc.NxpNfcLogger;
import com.nxp.wlc.core.NfcOperations;
import com.nxp.wlc.core.NxpNciPacketHandler;

/**
 * This class is handles WLC Events specific notfications
 */
public class WlcEventHandler implements INxpNfcNtfHandler, INxpOEMCallbacks {

    private static final String TAG = "WlcEventHandler";

    private final NfcOperations mNfcOperations;
    private final NxpNciPacketHandler mNxpNciPacketHandler;
    private final Context mContext;
    private IWlcEventCallbacks mWlcCallbacks = null;

    private static final byte NCI_PROP_NTF_VAL = (byte) 0x6F;
    private static final byte NCI_WLC_PROP_OID_VAL = (byte) 0x71;

    private static final byte NCI_WLC_SEND_RAW_CMD_SUB_OID      = 0x00;
    private static final byte NCI_WLC_STATUS_NTF_SUB_OID        = 0x01;
    private static final byte NCI_WLC_OBS_DATA_PKT_CMD_SUB_OID  = 0x02;

    private static final byte WLC_SUB_OID_INDEX   = 0x01;
    private static final byte WLC_DATA_PKT_INDEX  = 0x05;


    public WlcEventHandler(NfcAdapter nfcAdapter, Context context) {
        this.mContext = context;
        this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
        this.mNfcOperations = NfcOperations.getInstance(nfcAdapter);
    }

    /**
     * This API registers the Application callbacks to be called
     * for WLC Event notifications.
     * @param callbacks : callback to be registered
     */
    public void registerWlcCallbacks(IWlcEventCallbacks callbacks) {
        NxpNfcLogger.d(TAG, "Entry registerWlcCallbacks");
        mWlcCallbacks = callbacks;
        mNxpNciPacketHandler.registerNtfCallback(Executors.newCachedThreadPool(),
                        this);
    }

    /**
     * This API unregisters the Application callbacks to be called
     * for WLC Event notifications.
     */
    public void unregisterWlcCallbacks() {
        NxpNfcLogger.d(TAG, "Entry unregisterWlcCallbacks");
        mWlcCallbacks = null;
        mNxpNciPacketHandler.unregisterNtfCallback(this);
    }

    @Override
    public void onVendorNciNotification(int gid, int oid, byte[] payload) {
        if (payload == null || payload.length < 2) {
            NxpNfcLogger.e(TAG, "Invalid payload");
            return;
        }

        NxpNfcLogger.d(TAG, "onVendorNciNotification GID: " +  gid + " OID: " + oid);
        if((gid != NCI_PROP_NTF_VAL) || (oid != NCI_WLC_PROP_OID_VAL)){
            NxpNfcLogger.e(TAG, "Unknown Notification");
            return;
        }

        byte subOid = payload[WLC_SUB_OID_INDEX];
        byte[] wlcData = Arrays.copyOfRange(payload, WLC_DATA_PKT_INDEX, payload.length);

        switch(subOid){
            case NCI_WLC_STATUS_NTF_SUB_OID:
                if(mWlcCallbacks != null)
                    mWlcCallbacks.onWlcStatusNtf(wlcData);
                break;
            case NCI_WLC_OBS_DATA_PKT_CMD_SUB_OID:
                if(mWlcCallbacks != null)
                    mWlcCallbacks.onWlcDataReceived(wlcData);
                break;
            default:
                NxpNfcLogger.d(TAG, "Unknown Notification");
                break;
        }
    }
}
