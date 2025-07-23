/*
 * Copyright 2024-2025 NXP
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

package com.nxp.nfc.core;

import android.annotation.CallbackExecutor;
import android.annotation.NonNull;
import android.nfc.NfcAdapter;
import android.nfc.NfcAdapter.NfcVendorNciCallback;

import com.nxp.nfc.INxpNfcNtfHandler;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;
import com.nxp.nfc.NxpNfcUtils;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

/**
 * @class NxpNciPacketHandler
 * @brief Responsible for Sending VendorNciCmd's,
 *        Listening the to the NfcState Changes etc..
 * @hide
 */
public class NxpNciPacketHandler {

    private static final String TAG = "NxpNciPacketHandler";

    private static NxpNciPacketHandler sNxpNciPacketHandler;

    private NfcAdapter mNfcAdapter;
    private final Map<INxpNfcNtfHandler, Executor> mCallbackMap =
        new ConcurrentHashMap<>();
    private byte[] mVendorNciRsp;
    private byte mCurrentCmdSubGidOid;
    private boolean mIsSubGidCheckReq = true;
    private CountDownLatch mResCountDownLatch;
    private boolean isCallbackRegistered = false;

    /**
     * @brief hold NTF callback executor.
     */
    private static final ExecutorService NTF_CALLBACK_EXECUTOR =
                        Executors.newCachedThreadPool();

    private NxpNciPacketHandler(NfcAdapter nfcAdapter) {
      this.mNfcAdapter = nfcAdapter;
    }

    public static NxpNciPacketHandler getInstance(NfcAdapter nfcAdapter) {
      if (sNxpNciPacketHandler == null) {
        sNxpNciPacketHandler = new NxpNciPacketHandler(nfcAdapter);
      }
      return sNxpNciPacketHandler;
    }

    public void registerNtfCallback(@NonNull @CallbackExecutor Executor executor,
                                 @NonNull INxpNfcNtfHandler nxpNfcNtfHandler) {
      if (executor == null ) {
        NxpNfcLogger.e(TAG, "Executor must not be null!");
        throw new IllegalArgumentException();
      }

      synchronized(NxpNciPacketHandler.this) {
        if (mCallbackMap.containsKey(nxpNfcNtfHandler)) {
            NxpNfcLogger.e(TAG, "Callback already registered.");
            return;
        }

        if (mCallbackMap.isEmpty() || !isCallbackRegistered) {
          NxpNfcLogger.d(TAG, "registerNfcVendorNciCallback");
          mNfcAdapter.registerNfcVendorNciCallback(
                    NTF_CALLBACK_EXECUTOR, mNfcVendorNciCallback);
          isCallbackRegistered = true;
        }

        mCallbackMap.put(nxpNfcNtfHandler, executor);
      }
    }

    /**
     * sets the value to verify Sub GID of response with command
     * @param value : true if check needed false if check not needed
     */
    public void shouldCheckResponseSubGid(boolean value) {
        mIsSubGidCheckReq = value;
    }

    public void
    unregisterNtfCallback(@NonNull INxpNfcNtfHandler nxpNfcNtfHandler) {
      synchronized(NxpNciPacketHandler.this) {
        if (mCallbackMap.containsKey(nxpNfcNtfHandler))
          mCallbackMap.remove(nxpNfcNtfHandler);
        else
          NxpNfcLogger.e(TAG, "Callback not registered");

        if (mCallbackMap.isEmpty() && isCallbackRegistered) {
            NxpNfcLogger.d(TAG, "unregisterNfcVendorNciCallback");
            mNfcAdapter.unregisterNfcVendorNciCallback(mNfcVendorNciCallback);
            isCallbackRegistered = false;
        }
      }
    }

    /**
     * send vendor nci message with provided gid, oid & payload and wits until response
     * is received
     * @param gid
     * @param oid
     * @param payload
     * @return response byte array
     */
    public synchronized byte[] sendVendorNciMessage(int gid, int oid, byte[] payload) {
        NxpNfcLogger.d(TAG,
                    "sendVendorNciMessage API  gid: " + gid
                        + ", oid: " + oid + ", " + NxpNfcUtils.toHexString(payload));
        int status = NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED;
        try {
            if (mNfcAdapter != null) {
                synchronized(NxpNciPacketHandler.this) {
                    if (mCallbackMap.isEmpty() || !isCallbackRegistered) {
                        NxpNfcLogger.d(TAG, "registerNfcVendorNciCallback");
                        mNfcAdapter.registerNfcVendorNciCallback(
                                NTF_CALLBACK_EXECUTOR, mNfcVendorNciCallback);
                        isCallbackRegistered = true;
                    }
                }
                if (mIsSubGidCheckReq) {
                    mCurrentCmdSubGidOid = payload[0];
                }
                mResCountDownLatch = new CountDownLatch(1);
                status = mNfcAdapter.sendVendorNciMessage(NfcAdapter.MESSAGE_TYPE_COMMAND,
                            gid, oid, payload);
                if (status != NfcAdapter.SEND_VENDOR_NCI_STATUS_SUCCESS) {
                    mVendorNciRsp = new byte[] { (byte) status };
                    NxpNfcLogger.e(TAG, "sendVendorNciMessage: error " + status);
                } else {
                    if (!mResCountDownLatch.await(NxpNfcConstants.SEND_RAW_WAIT_TIME_OUT_VAL,
                                            TimeUnit.MILLISECONDS)) {
                        NxpNfcLogger.d(TAG, "sendVendorNciMessage: error in wait " + status);
                        mVendorNciRsp = new byte[] { (byte) NxpNfcConstants.TIMEOUT_ERR_CODE };
                    }
                }
                synchronized(NxpNciPacketHandler.this) {
                    if (mCallbackMap.isEmpty() && isCallbackRegistered) {
                        NxpNfcLogger.d(TAG, "unRegisterNfcVendorNciCallback");
                        mNfcAdapter.unregisterNfcVendorNciCallback(mNfcVendorNciCallback);
                        isCallbackRegistered = false;
                    }
                }
            }
        } catch (Exception e) {
            NxpNfcLogger.e(TAG, "Error while calling sendVendorNciMessage()");
            mVendorNciRsp = new byte[] { (byte) NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED };
        }
        return mVendorNciRsp;
    }

    /**
     * Callback for Vendor Nci Response and Vendor Nci Notification
     */
    NfcVendorNciCallback mNfcVendorNciCallback = new NfcVendorNciCallback() {

        @Override
        public void onVendorNciResponse(int gid, int oid, byte[] payload) {
            NxpNfcLogger.d(TAG, "onVendorNciResponse Gid " + gid + " Oid " + oid
                            + ",payload: " + NxpNfcUtils.toHexString(payload));
            if ((!mIsSubGidCheckReq) || (mCurrentCmdSubGidOid == payload[0])) {
                NxpNfcLogger.d(TAG, "Expected Response received!");
                mVendorNciRsp = payload;
                if (mResCountDownLatch != null) {
                    mResCountDownLatch.countDown();
                }
            } else {
                NxpNfcLogger.e(TAG, "UnExpected Response received!");
            }
        }

        @Override
        public void onVendorNciNotification(int gid, int oid, byte[] payload) {
            NxpNfcLogger.d(TAG, "onVendorNciNotification Gid " + gid + " Oid " + oid
                    + ", payload: " + NxpNfcUtils.toHexString(payload));
            if (mCallbackMap.size() >= 1)
              mCallbackMap.forEach(
                  (INxpNfcNtfHandler, Executor)
                      -> INxpNfcNtfHandler.onVendorNciNotification(gid, oid,
                                                                   payload));
        }
    };
}
