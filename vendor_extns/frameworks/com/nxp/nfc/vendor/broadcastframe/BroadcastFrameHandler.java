/*
 *
 *  The original Work has been changed by NXP.
 *
 *  Copyright 2026 NXP
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

package com.nxp.nfc.vendor.broadcastframe;

import android.content.Context;
import android.nfc.NfcAdapter;

import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.core.NxpNciPacketHandler;

import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BroadcastFrameHandler implements INxpNfcNtfHandler {

    private static final String TAG = "BroadcastFrameHandler";

    private final Context mContext;
    private final NxpNciPacketHandler mNxpNciPacketHandler;
    private volatile IBroadcastFrameNotificationCallbacks mBroadcastFrameNtfCallback = null;
    private static final ExecutorService BROADCAST_FRAME_CALLBACK_EXECUTOR =
                        Executors.newSingleThreadExecutor();

    private static final byte BROADCAST_FRAME_SUB_GID = 0x01;
    private static final byte BROADCAST_FRAME_SUB_OID = 0x03;

    private static final byte MIN_PAYLOAD_LENGTH = 0x03;

    public BroadcastFrameHandler(NfcAdapter nfcAdapter, Context context) {
        this.mContext = context;
        this.mNxpNciPacketHandler = NxpNciPacketHandler.getInstance(nfcAdapter);
    }


    /**
     * This API   the Application callbacks to be called
     * for broadcast frame notifications.
     * @param callbacks : callback to be registered
     */
    public void registerBroadcastFrameNotificationCallbacks (
            IBroadcastFrameNotificationCallbacks callbacks) {
        NxpNfcLogger.d(TAG, "registerBroadcastFrameNotificationCallbacks");
        if (callbacks == null) {
            throw new IllegalArgumentException("Callback cannot be null");
        }
        mBroadcastFrameNtfCallback = callbacks;
        mNxpNciPacketHandler.registerNtfCallback(BROADCAST_FRAME_CALLBACK_EXECUTOR, this);
    }

    /**
     * This API unregisters the Application callbacks to be called
     * for broadcast frame notifications.
     */
    public void unregisterBroadcastFrameNotificationCallbacks () {
        NxpNfcLogger.d(TAG, "unregisterBroadcastFrameNotificationCallbacks");
        mBroadcastFrameNtfCallback = null;
        mNxpNciPacketHandler.unregisterNtfCallback(this);
    }

    @Override
    public void onVendorNciNotification(int gid, int oid, byte[] payload) {
        if (payload == null || payload.length < MIN_PAYLOAD_LENGTH) {
            NxpNfcLogger.e(TAG, "Invalid payload");
            return;
        }
        NxpNfcLogger.d(TAG, "onVendorNciNotification GID: " + gid + " OID: " + oid);
        if (gid == NxpNfcConstants.NFC_NCI_NTF_PROP_GID && oid == NxpNfcConstants.NXP_NFC_PROP_OID
                && payload[0] == BROADCAST_FRAME_SUB_GID && payload[1] == BROADCAST_FRAME_SUB_OID) {
            NxpNfcLogger.d(TAG, "Broadcast Proprietary Action notification");
            if (mBroadcastFrameNtfCallback != null) {
                byte[] broadcastFramePayload = Arrays.copyOfRange(payload, MIN_PAYLOAD_LENGTH - 1,
                        payload.length);
                mBroadcastFrameNtfCallback.onBroadcastFrameNotificationReceived(broadcastFramePayload);
            }
        } else {
            NxpNfcLogger.d(TAG, "Unknown Notification");
        }
    }
}
