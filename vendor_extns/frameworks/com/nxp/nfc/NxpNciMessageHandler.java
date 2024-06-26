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

package com.nxp.nfc;

import android.nfc.NfcAdapter;
import android.nfc.NfcAdapter.NfcVendorNciCallback;
import android.nfc.NfcOemExtension;
import android.nfc.Tag;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

/**
 * @class NxpNciMessageHandler
 * @brief Responsible for Sending VendorNciCmd's,
 *        Listening the to the NfcState Changes etc..
 *
 */
public class NxpNciMessageHandler {

    private static final String TAG = "NxpNciMessageHandler";

    private NfcAdapter mNfcAdapter;
    private NfcOemExtension mNfcOemExtension;

    private byte[] mVendorNcirsp;
    private CountDownLatch mResCountDownLatch;

    protected NxpNciMessageHandler(NfcAdapter nfcAdapter) {
        this.mNfcAdapter = nfcAdapter;
        mNfcOemExtension = mNfcAdapter.getNfcOemExtension();
        mNfcAdapter.registerNfcVendorNciCallback(Executors.newSingleThreadExecutor(),
                            mNfcVendorNciCallback);
        mNfcOemExtension.registerCallback(Executors.newSingleThreadExecutor(),
                            mOemExtensionCallback);
    }

    public synchronized byte[] sendVendorNciMessage(int gid, int oid, byte[] payload) {
        NxpNfcLogger.d(TAG,
                    "sendVendorNciMessage API  gid: " + gid
                        + ", oid: "+ oid + ", " + NxpNfcUtils.toHexString(payload));
        int status = NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED;
        try {
            if (mNfcAdapter != null) {
                status = mNfcAdapter.sendVendorNciMessage(NfcAdapter.MESSAGE_TYPE_COMMAND, 
                            gid, oid, payload);
                if (status != NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                    mVendorNcirsp = new byte[] { (byte) status };
                    NxpNfcLogger.e(TAG, "sendVendorNciMessage: error " + status);
                } else {
                    mResCountDownLatch = new CountDownLatch(1);
                    if (!mResCountDownLatch.await(NxpNfcConstants.SEND_RAW_WAIT_TIME_OUT_VAL,
                                            TimeUnit.MILLISECONDS)) {
                        NxpNfcLogger.d(TAG, "sendVendorNciMessage: error in wait " + status);
                        mVendorNcirsp = new byte[] { (byte) NxpNfcConstants.TIMEOUT_ERR_CODE };
                    }
                }
            }
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Error while calling sendVendorNciMessage()");
            mVendorNcirsp = new byte[] { (byte) NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED };
        }
        return mVendorNcirsp;
    }

    private NfcOemExtension.Callback mOemExtensionCallback = new NfcOemExtension.Callback() {

        @Override
        public void onTagConnected(boolean connected, Tag tag) {
            NxpNfcLogger.d(TAG, "onTagConnected: " + connected);
        }

        @Override
        public void onRfDiscoveryStarted(boolean isDiscoveryStarted) {
            NxpNfcLogger.d(TAG, "onRfDiscoveryStarted: " + isDiscoveryStarted);
        }

        @Override
        public void onCardEmulationActivated(boolean isActivated) {
            NxpNfcLogger.d(TAG, "onCardEmulationActivated: " + isActivated);
        }

        @Override
        public void onRfFieldActivated(boolean isActivated) {
            NxpNfcLogger.d(TAG, "onRfFieldActivated: " + isActivated);
        }

    };

    NfcVendorNciCallback mNfcVendorNciCallback = new NfcVendorNciCallback() {

        @Override
        public void onVendorNciResponse(int gid, int oid, byte[] payload) {
            NxpNfcLogger.d(TAG, "onVendorNciResponse Gid " + gid + " Oid " + oid
                            + ",payload: " + NxpNfcUtils.toHexString(payload));
            mVendorNcirsp = payload;
            mResCountDownLatch.countDown();
        }

        @Override
        public void onVendorNciNotification(int gid, int oid, byte[] payload) {
            NxpNfcLogger.d(TAG, "onVendorNciNotification Gid " + gid + " Oid " + oid
                            + ", payload: " + NxpNfcUtils.toHexString(payload));
        }

    };
}