/*
 * Copyright 2024-2026 NXP
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
 * @brief Responsible for Sending VendorNciCmds,
 *        Listening the to the NfcState Changes etc..
 * @hide
 */
public class NxpNciPacketHandler {

    private static final String TAG = "NxpNciPacketHandler";

    private static volatile NxpNciPacketHandler sNxpNciPacketHandler;

    private NfcAdapter mNfcAdapter;
    private final Map<INxpNfcNtfHandler, Executor> mCallbackMap =
        new ConcurrentHashMap<>();
    private byte[] mVendorNciRsp;
    private byte mCurrentCmdSubGidOid;
    private boolean mIsSubGidCheckReq = true;
    private CountDownLatch mResCountDownLatch;
    private boolean isCallbackRegistered = false;
    private static final int NCI_GID_MIN = 0;
    private static final int NCI_GID_MAX = 15;
    private static final int NCI_OID_MIN = 0;

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
        synchronized (NxpNciPacketHandler.class) {
          if (sNxpNciPacketHandler == null) {
            sNxpNciPacketHandler = new NxpNciPacketHandler(nfcAdapter);
          }
        }
      }
      if (sNxpNciPacketHandler == null) {
        NxpNfcLogger.e(TAG, "getInstance: sNxpNciPacketHandler is null");
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
      if (nxpNfcNtfHandler == null) {
        NxpNfcLogger.e(TAG, "unregisterNtfCallback: nxpNfcNtfHandler is null");
        throw new IllegalArgumentException("nxpNfcNtfHandler is null");
      }
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
     * Validates gid and oid per NCI specification.
     * @param gid Group ID - must be 0 to 15 (4 bits)
     * @param oid Opcode ID - must be >= 0
     * @return true if valid, false otherwise
     */
    private boolean isValidNciMessage(int gid, int oid) {
        return ((oid >= NCI_OID_MIN) &&
                (gid >= NCI_GID_MIN && gid <= NCI_GID_MAX));
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
        if (payload == null) {
            NxpNfcLogger.e(TAG, "sendVendorNciMessage: payload is null");
            return new byte[] { (byte) NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED };
        }
        if (!isValidNciMessage(gid, oid)) {
            NxpNfcLogger.e(TAG, "Invalid NCI params gid: " + gid
                            + " (must be 0-15) oid: " + oid
                            + " (must be >= 0)");
            return new byte[]{
                (byte) NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED
            };
        }
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
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            NxpNfcLogger.e(TAG, "Error while calling sendVendorNciMessage()");
            mVendorNciRsp = new byte[] { (byte) NfcAdapter.SEND_VENDOR_NCI_STATUS_FAILED };
        }
        return mVendorNciRsp != null ? mVendorNciRsp.clone() : null;
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
