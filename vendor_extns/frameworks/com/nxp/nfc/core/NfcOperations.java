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

package com.nxp.nfc.core;

import android.app.Activity;
import android.nfc.NfcAdapter;
import android.nfc.NfcAdapter.ControllerAlwaysOnListener;
import android.nfc.NfcOemExtension;
import android.nfc.Tag;

import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
 /**
 * @class NfcOperations
 * @brief A wrapper class for Nfc functionality.
 *
 * This interface provides methods for enable, disbale discovery etc...
 * @hide
 */
public class NfcOperations {
    private static final String TAG = "NfcOperations";

    private static NfcOperations sNfcOperations = null;

    private Activity mActivity;
    private NfcAdapter mNfcAdapter;
    private NfcOemExtension mNfcOemExtension;

    private CountDownLatch mDisCountDownLatch;
    private CountDownLatch mControllerAlwaysOnLatch;

    private boolean mIsDiscoveryStarted = false;
    private boolean mIsRfFieldActivated = false;
    private boolean mIsCardEmulationActivated = false;

    private NfcOperations(NfcAdapter nfcAdapter, Activity activity) {
        this.mNfcAdapter = nfcAdapter;
        this.mActivity = activity;
        mNfcOemExtension = mNfcAdapter.getNfcOemExtension();
        mNfcAdapter.registerControllerAlwaysOnListener(Executors.newSingleThreadExecutor(),
                            mControllerAlwaysOnListener);
        mNfcOemExtension.registerCallback(Executors.newSingleThreadExecutor(),
                            mOemExtensionCallback);
    }

    public static NfcOperations getInstance(NfcAdapter nfcAdapter, Activity activity) {
        if (sNfcOperations == null) {
            sNfcOperations = new NfcOperations(nfcAdapter, activity);
        }
        return sNfcOperations;
    }

    public void setControllerAlwaysOn(boolean value) {
        NxpNfcLogger.d(TAG, "setControllerAlwaysOn");
        if (value) {
            mControllerAlwaysOnLatch = new CountDownLatch(1);
            mNfcOemExtension.setControllerAlwaysOn(NfcOemExtension.ENABLE_TRANSPARENT);
            try {
                mControllerAlwaysOnLatch.await(NxpNfcConstants.SEND_RAW_WAIT_TIME_OUT_VAL,
                                            TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                NxpNfcLogger.e(TAG, "Error in setControllerAlwaysOn");
            }
        } else {
            mNfcOemExtension.setControllerAlwaysOn(NfcOemExtension.DISABLE);
        }
    }

    public void disableDiscovery() {
        NxpNfcLogger.d(TAG, "disableDiscovery");
        mDisCountDownLatch = new CountDownLatch(1);
        mNfcAdapter.setDiscoveryTechnology(mActivity,
            NfcAdapter.FLAG_READER_DISABLE, NfcAdapter.FLAG_LISTEN_DISABLE);
        try {
            mDisCountDownLatch.await(NxpNfcConstants.SEND_RAW_WAIT_TIME_OUT_VAL,
                            TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            NxpNfcLogger.e(TAG, "Error disabling discovery");
        }
    }

    public void enableDiscovery() {
        NxpNfcLogger.d(TAG, "enableDiscovery");
        mDisCountDownLatch = new CountDownLatch(1);
        mNfcAdapter.resetDiscoveryTechnology(mActivity);
        try {
            mDisCountDownLatch.await(NxpNfcConstants.SEND_RAW_WAIT_TIME_OUT_VAL,
                            TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            NxpNfcLogger.e(TAG, "Error disabling discovery");
        }
    }

    private NfcOemExtension.Callback mOemExtensionCallback = new NfcOemExtension.Callback() {

        @Override
        public void onTagConnected(boolean connected, Tag tag) {
            NxpNfcLogger.d(TAG, "onTagConnected: " + connected);
        }

        @Override
        public void onRfDiscoveryStarted(boolean isDiscoveryStarted) {
            NxpNfcLogger.d(TAG, "onRfDiscoveryStarted: " + isDiscoveryStarted);
            NfcOperations.this.mIsDiscoveryStarted = isDiscoveryStarted;
            if (mDisCountDownLatch != null) mDisCountDownLatch.countDown();
        }

        @Override
        public void onCardEmulationActivated(boolean isActivated) {
            NfcOperations.this.mIsCardEmulationActivated = isActivated;
            NxpNfcLogger.d(TAG, "onCardEmulationActivated: " + isActivated);
        }

        @Override
        public void onRfFieldActivated(boolean isActivated) {
            NfcOperations.this.mIsRfFieldActivated = isActivated;
            NxpNfcLogger.d(TAG, "onRfFieldActivated: " + isActivated);
        }

    };

    ControllerAlwaysOnListener mControllerAlwaysOnListener = new ControllerAlwaysOnListener() {

        @Override
        public void onControllerAlwaysOnChanged(boolean isEnabled) {
            NxpNfcLogger.d(TAG, "onControllerAlwaysOnChanged: " + isEnabled);
            if (mControllerAlwaysOnLatch != null) mControllerAlwaysOnLatch.countDown();

        }
    };

    public boolean isRfFieldActivated() {
        return this.mIsRfFieldActivated;
    }

    public boolean isCardEmulationActivated() {
        return this.mIsCardEmulationActivated;
    }

    public boolean isDiscoveryStarted() {
        return this.mIsDiscoveryStarted;
    }

    public boolean isEnabled() {
        return mNfcAdapter.isEnabled();
    }

    public boolean isControllerAlwaysOn() {
        return mNfcAdapter.isControllerAlwaysOn();
    }
}
