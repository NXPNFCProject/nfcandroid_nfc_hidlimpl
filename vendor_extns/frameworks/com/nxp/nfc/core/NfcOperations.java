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

import android.content.ComponentName;
import android.content.Intent;
import android.nfc.NdefMessage;
import android.nfc.NfcAdapter;
import android.nfc.NfcAdapter.ControllerAlwaysOnListener;
import android.nfc.NfcOemExtension;
import android.nfc.OemLogItems;
import android.nfc.RfDiscoverConfig;
import android.nfc.Tag;
import android.nfc.cardemulation.ApduServiceInfo;
import android.os.AsyncTask;

import com.nxp.nfc.INxpOEMCallbacks;
import com.nxp.nfc.NxpNfcConstants;
import com.nxp.nfc.NxpNfcLogger;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;
import java.lang.IllegalStateException;
import java.util.List;
import java.util.Map;

/**
 * @class NfcOperations
 * @brief A wrapper class for Nfc functionality.
 *
 * This interface provides methods for enable, disable discovery etc...
 * @hide
 */
public class NfcOperations {
    private static final String TAG = "NfcOperations";

    private static NfcOperations sNfcOperations = null;

    private int PAUSE_POLLING_INDEFINITELY = 0;

    private static final int NCI_TYPE_POLL_A = 0x00;
    private static final int NCI_TYPE_POLL_B = 0x01;
    private static final int NCI_TYPE_POLL_F = 0x02;
    private static final int NCI_TYPE_POLL_V = 0x06;
    private static final int NFC_TYPE_POLL_KOVIO = 0x70;
    private static final int NCI_TYPE_LISTEN_A = 0x80;
    private static final int NCI_TYPE_LISTEN_B = 0x81;
    private static final int NCI_TYPE_LISTEN_F = 0x82;
    private static final int NCI_TYPE_LISTEN_ISO15693 = 0x86;
    private static final int FLAG_USE_ALL_TECH = 0xff;

    private boolean mIsPollingPaused = false;

    private boolean mIsRoutingSkipped = false;

    private NfcAdapter mNfcAdapter;
    private NfcOemExtension mNfcOemExtension;
    private INxpOEMCallbacks mNxpOemCallbacks = null;
    private Object mCallbackLock = new Object();

    /**
     * @brief hold callback executor.
     */
    private static final ExecutorService CALLBACK_EXECUTOR =
                        Executors.newCachedThreadPool();

    /**
     * @brief holds the value for listen tech disable
     */
    private boolean mListenTechDisabled = false;

    /**
     * @brief wait latch for enable/disable discovery
     */
    private CountDownLatch mDisCountDownLatch;

    /**
     * @brief wait latch to receive callback data
     */
    private CountDownLatch mCallbackCountDownLatch;
    /**
     * @brief wait latch for {@link #setControllerAlwaysOn(boolean)}
     */
    private CountDownLatch mControllerAlwaysOnLatch;

    /**
     * @brief holds the value of
     * {@link NfcOemExtension.Callback#onRfDiscoveryStarted()}
     */
    private boolean mIsDiscoveryStarted = false;
    /**
     * @brief holds the value of
     * {@link NfcOemExtension.Callback#onRfFieldDetected()}
     */
    private boolean mIsRfFieldDetected = false;
    /**
     * @brief holds the value of
     * {@link NfcOemExtension.Callback#onCardEmulationActivated()}
     */
    private boolean mIsCardEmulationActivated = false;
    /**
     * @brief holds the value of
     * {@link NfcOemExtension.Callback#onEeListenActivated()}
     */
    private boolean mIsEeListenActivated = false;
    /**
     * @brief holds the value of
     * {@link NfcOemExtension.Callback#onTagConnected()}
     */
    private boolean mIsTagConnected = false;

    Map<String, Boolean> mOemCallbackMap = new HashMap<>();

    /**
     * @brief Map storing NCI RF technology types as keys and the corresponding
     * NFCAdapter configuration flags as values.
     */
    Map<Integer, Integer> mNciServiceRfConfigMap = new HashMap<>();

    List<RfDiscoverConfig> rfDiscoverConfigList = new ArrayList<>();

    /**
     * @brief private constructor to create singleton object
     * @param nfcAdapter
     */
    private NfcOperations(NfcAdapter nfcAdapter) {
        this.mNfcAdapter = nfcAdapter;
        mNfcOemExtension = mNfcAdapter.getNfcOemExtension();
        mNfcAdapter.registerControllerAlwaysOnListener(Executors.newSingleThreadExecutor(),
                            mControllerAlwaysOnListener);
        initializeNciServiceRfConfig();
        // Store Discovery configs once object created.
        // Use stored config, while resetting to default discovery.
        rfDiscoverConfigList = mNfcOemExtension.getRfDiscoverConfigurations();
    }

    /**
     * @brief public function to get the instance of
     * {@link #NfcOperations(NfcAdapter)}
     * @param nfcAdapter
     * @return instance of {@link #NfcOperations(NfcAdapter)}
     */
    public static NfcOperations getInstance(NfcAdapter nfcAdapter) {
        if (sNfcOperations == null) {
            sNfcOperations = new NfcOperations(nfcAdapter);
        }
        return sNfcOperations;
    }

    /**
     * @brief Initializes the mapping between NCI RF interface types and their corresponding
     * Android NFCAdapter reader/listen flags.
     * @return None
     */
    private void initializeNciServiceRfConfig() {
        mNciServiceRfConfigMap.put(NCI_TYPE_POLL_A, NfcAdapter.FLAG_READER_NFC_A);
        mNciServiceRfConfigMap.put(NCI_TYPE_POLL_B, NfcAdapter.FLAG_READER_NFC_B);
        mNciServiceRfConfigMap.put(NCI_TYPE_POLL_F, NfcAdapter.FLAG_READER_NFC_F);
        mNciServiceRfConfigMap.put(NCI_TYPE_POLL_V, NfcAdapter.FLAG_READER_NFC_V);
        mNciServiceRfConfigMap.put(NFC_TYPE_POLL_KOVIO, NfcAdapter.FLAG_READER_NFC_BARCODE);
        mNciServiceRfConfigMap.put(NCI_TYPE_LISTEN_A, NfcAdapter.FLAG_LISTEN_NFC_PASSIVE_A);
        mNciServiceRfConfigMap.put(NCI_TYPE_LISTEN_B, NfcAdapter.FLAG_LISTEN_NFC_PASSIVE_B);
        mNciServiceRfConfigMap.put(NCI_TYPE_LISTEN_F, NfcAdapter.FLAG_LISTEN_NFC_PASSIVE_F);
    }

    /**
     * @brief enables ControllerAlwaysOn and waits till
     * {@link #mControllerAlwaysOnListener.onControllerAlwaysOnChanged}
     * callback triggers
     * @param value true for turning on else off
     * @return None
     */
    public void setControllerAlwaysOn(boolean value) {
        NxpNfcLogger.d(TAG, "setControllerAlwaysOn");
        if (value) {
            mControllerAlwaysOnLatch = new CountDownLatch(1);
            mNfcOemExtension.setControllerAlwaysOnMode(NfcOemExtension.ENABLE_TRANSPARENT);
            try {
                mControllerAlwaysOnLatch.await(NxpNfcConstants.SEND_RAW_WAIT_TIME_OUT_VAL,
                                            TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                NxpNfcLogger.e(TAG, "Error in setControllerAlwaysOn");
            }
        } else {
            mNfcOemExtension.setControllerAlwaysOnMode(NfcOemExtension.DISABLE);
        }
    }

    /**
     * @brief disable discovery and waits till
     * {@link NfcOemExtension.Callback#onRfDiscoveryStarted(boolean) callback triggers}
     * @return None
     */
    public void disableDiscovery() {
        NxpNfcLogger.d(TAG, "disableDiscovery");
        startDiscovery(false);
    }

    /**
     * @brief enable discovery and waits till
     * {@link NfcOemExtension.Callback#onRfDiscoveryStarted(boolean) callback triggers}
     * @return None
     */
    public void enableDiscovery() {
        NxpNfcLogger.d(TAG, "enableDiscovery With Keep READER|LISTEN");
        int pollTech = NfcAdapter.FLAG_READER_DISABLE;
        int listenTech = NfcAdapter.FLAG_LISTEN_DISABLE;
        if (rfDiscoverConfigList == null || rfDiscoverConfigList.isEmpty()) {
            NxpNfcLogger.d(TAG, "No previous discovery found, "
                                + "setting poll & listen tech to ALL_TECH");
            setDiscoveryTech(NfcAdapter.FLAG_SET_DEFAULT_TECH | FLAG_USE_ALL_TECH,
                    NfcAdapter.FLAG_SET_DEFAULT_TECH | FLAG_USE_ALL_TECH);
            return;
        }
        for (RfDiscoverConfig rfConfig : rfDiscoverConfigList) {
            if (rfConfig == null) {
                NxpNfcLogger.e(TAG, "rfConfig is null, skipping...: ");
                continue;
            }
            int techMode = rfConfig.getTechnologyMode();
            int techMask = techMode & FLAG_USE_ALL_TECH;
            Integer rfConfigVal = mNciServiceRfConfigMap.get(techMask);
            if (rfConfigVal == null) {
                NxpNfcLogger.d(TAG, "Skipping unknown RF Config techMask : " + techMask);
                continue;
            }
            if (techMode < 0) {
                listenTech |= rfConfigVal;
            } else {
                pollTech |= rfConfigVal;
            }
        }
        // In-case No valid discovery configs found.
        if (pollTech == NfcAdapter.FLAG_READER_DISABLE
                && listenTech == NfcAdapter.FLAG_LISTEN_DISABLE) {
            NxpNfcLogger.e(TAG, "Invalid pollTech: " + pollTech + " and listenTech: "
                                + listenTech + " , Not enabling discovery");
            return;
        }
        NxpNfcLogger.d(TAG, "enableDiscovery With poll_tech: " + Integer.toHexString(pollTech)
                            + " listen_tech: " + Integer.toHexString(listenTech));
        setDiscoveryTech(NfcAdapter.FLAG_SET_DEFAULT_TECH | pollTech,
                NfcAdapter.FLAG_SET_DEFAULT_TECH | listenTech);
    }

    /**
     * only sets the discovery technology parameters
     */
    private void setDiscoveryTechnology(int pollTechnology, int listenTechnology) {
        NxpNfcLogger.d(TAG, "setDiscoveryTechnology");
        mNfcAdapter.setDiscoveryTechnology(null, pollTechnology,
                                            listenTechnology);
        synchronized (NfcOperations.this) {
            if (listenTechnology == NfcAdapter.FLAG_LISTEN_DISABLE)  {
                NxpNfcLogger.d(TAG, "Listen Disabled");
                mListenTechDisabled = true;
            } else {
                NxpNfcLogger.d(TAG, "Listen Enabled");
                mListenTechDisabled = false;

            }
        }
    }

    /**
     * @brief sets discover Technology also starts discovery if stopped
     * @param pollTechnology Flags indicating poll technologies.
     * @param listenTechnology Flags indicating listen technologies.
     * @return None
     */
    public void setDiscoveryTech(int pollTechnology, int listenTechnology) {
        NxpNfcLogger.d(TAG, "setDiscoveryTech");
        synchronized (NfcOperations.this) {
            mIsRoutingSkipped = true;
            setDiscoveryTechnology(pollTechnology, listenTechnology);
            mIsRoutingSkipped = false;
        }
        startDiscovery(true);
    }

    /**
     * @brief Register/Unregister OEM callback based of isRegister, if not
     *        registered already
     * @param isRegister
     */
    private void conditionallyRegisterOemCallback(boolean isRegister) {
        synchronized (mCallbackLock) {
            if (mNxpOemCallbacks == null) {
                if (isRegister) {
                    mNfcOemExtension.registerCallback(CALLBACK_EXECUTOR,
                                                        mOemExtensionCallback);
                } else {
                    mNfcOemExtension.unregisterCallback(mOemExtensionCallback);
                }
            }
        }
    }
    /**
     * @brief sets discover Technology
     * @param pollTechnology Flags indicating poll technologies.
     * @param listenTechnology Flags indicating listen technologies.
     * @return None
     */
    private void startDiscovery(boolean isStart) {
        NxpNfcLogger.d(TAG, "startDiscovery isStart=" + isStart);
        try {
            synchronized (NfcOperations.this) {
                conditionallyRegisterOemCallback(true);
                if (isStart && mIsDiscoveryStarted) {
                    NxpNfcLogger.d(TAG, " discovery already started");
                    conditionallyRegisterOemCallback(false);
                    return;
                } else if (!isStart && !mIsDiscoveryStarted) {
                    NxpNfcLogger.d(TAG, " discovery already stopped");
                    conditionallyRegisterOemCallback(false);
                    return;
                }
                mDisCountDownLatch = new CountDownLatch(1);
                if (isStart)
                    mNfcOemExtension.resumePolling();
                else
                    mNfcOemExtension.pausePolling(PAUSE_POLLING_INDEFINITELY);

                mIsPollingPaused = !isStart;
                mDisCountDownLatch.await(NxpNfcConstants.SEND_RAW_WAIT_TIME_OUT_VAL,
                        TimeUnit.MILLISECONDS);
                conditionallyRegisterOemCallback(false);
            }
        } catch (InterruptedException e) {
            NxpNfcLogger.e(TAG, "Error while changing discovery " +isStart);
        } catch (IllegalArgumentException e) {
            NxpNfcLogger.e(TAG, "Oem Callback already unregistered");
        }
    }

    private void resetOemCallbackMap() {
        mOemCallbackMap.clear();
        mOemCallbackMap.put("onCardEmulationActivated", false);
        mOemCallbackMap.put("onRfFieldDetected", false);
        mOemCallbackMap.put("onRfDiscoveryStarted", false);
        mOemCallbackMap.put("onEeListenActivated", false);
        mOemCallbackMap.put("onTagConnected", false);
    }

    /**
     * @brief Updating mOemCallbackMap once callback received and when all callbacks
     *        received, countDown will reach to zero.
     * @param oemCallback
     */
    private void updateOemCallbackMap(String oemCallback) {
        if (mCallbackCountDownLatch != null && mOemCallbackMap.containsKey(oemCallback)) {
            if (!mOemCallbackMap.get(oemCallback)) {
                mOemCallbackMap.put(oemCallback, true);
                mCallbackCountDownLatch.countDown();
                if (mCallbackCountDownLatch.getCount() <= 0) {
                    resetOemCallbackMap();
                    mCallbackCountDownLatch = null;
                }
            }
        }
    }

    /**
     * @brief registers to the OEM callbacks through NXP extensions
     * @param nxpOEMCallback callback to be register
     */
    public void registerNxpOemCallback(INxpOEMCallbacks nxpOEMCallback) {
        synchronized (mCallbackLock) {
            if (nxpOEMCallback == null) {
                NxpNfcLogger.e(TAG, "registerNxpOemCallback: nxpOEMCallback is null");
                throw new IllegalArgumentException("nxpOEMCallback is null");
            }
            if (mNxpOemCallbacks == null) {
                resetOemCallbackMap();
                mCallbackCountDownLatch = new CountDownLatch(mOemCallbackMap.size());
                mNfcOemExtension.registerCallback(CALLBACK_EXECUTOR,
                                                    mOemExtensionCallback);
                try {
                    if (mCallbackCountDownLatch != null) {
                        boolean callbackFlag  = mCallbackCountDownLatch.await(
                                                    NxpNfcConstants.CALLBACK_TIME_OUT_VAL,
                                                    TimeUnit.MILLISECONDS);
                        if (!callbackFlag) {
                            NxpNfcLogger.e(TAG, "All OEM callbacks are not received");
                        }
                    }
                } catch (InterruptedException e) {
                    NxpNfcLogger.e(TAG, "Error in registerCallback");
                }
            }
            mNxpOemCallbacks = nxpOEMCallback;
        }
    }

    /**
     * @brief provides information is listen param disabled
     * @return true if disabled else false.
     */
    public synchronized boolean isListenDisabled() {
        return mListenTechDisabled;
    }

    /**
     * @brief provides information if polling paused
     * @return true if paused else false.
     */
    public synchronized boolean isPollingPaused() {
        return mIsPollingPaused;
    }


    /**
     * @brief unregisters to OEM callbacks through NXP extensions
     */
    public void unregisterNxpOemCallback() {
        synchronized (mCallbackLock) {
            if (mNxpOemCallbacks != null) {
                mNfcOemExtension.unregisterCallback(mOemExtensionCallback);
            }
            mNxpOemCallbacks = null;
        }
    }

    /**
     * @brief API to get the list  of all activated secure elements
     * @return map of active secure element SE name as key
     *         and secure element id as value.
     */
    public Map<String, Integer> getActiveEEList() {
        return mNfcOemExtension.getActiveNfceeList();
    }

    private NfcOemExtension.Callback mOemExtensionCallback = new NfcOemExtension.Callback() {

        @Override
        public void onTagConnected(boolean connected) {
            mIsTagConnected = connected;
            NxpNfcLogger.d(TAG, "onTagConnected: " + connected);
            if (mCallbackCountDownLatch != null) updateOemCallbackMap("onTagConnected");
        }

        @Override
        public void onStateUpdated(int state){
        }

        @Override
        public void onApplyRouting(Consumer<Boolean> isSkipped) {
            NxpNfcLogger.d(TAG, "onApplyRouting :");
            isSkipped.accept(mIsRoutingSkipped);
        }

        @Override
        public void onNdefRead(Consumer<Boolean> isSkipped) {
          isSkipped.accept(false);
        }

        @Override
        public void onEnableRequested(Consumer<Boolean> isAllowed) {
            NxpNfcLogger.d(TAG, "onEnableRequested :");
            isAllowed.accept(true);
        }

        @Override
        public void onDisableRequested(Consumer<Boolean> isAllowed) {
            NxpNfcLogger.d(TAG, "onDisableRequested :");
            boolean allowDisable = true;
            if (mNxpOemCallbacks != null) {
                allowDisable = mNxpOemCallbacks.onDisableRequested();
            }
            isAllowed.accept(allowDisable);
        }

        @Override
        public void onBootStarted(){
        }

        @Override
        public void onEnableStarted(){
           NxpNfcLogger.d(TAG, "onEnableStarted :");
        }

        @Override
        public void onDisableStarted(){
        }

        @Override
        public void onBootFinished(int status){
            NxpNfcLogger.d(TAG, "onBootFinished :");
            new BootFinishedTask().execute(status);
        }

        @Override
        public void onEnableFinished(int status){
            NxpNfcLogger.d(TAG, "onEnableFinished :");
            new EnableFinishedTask().execute(status);
        }

        @Override
        public void onDisableFinished(int status){
        }

        @Override
        public void onTagDispatch(Consumer<Boolean> isSkipped) {
          isSkipped.accept(false);
        }

        @Override
        public void onRoutingChanged(Consumer<Boolean> isSkipped) {
            NxpNfcLogger.d(TAG, "onRoutingChanged :");
            isSkipped.accept(false);
        }

        @Override
        public void onHceEventReceived(int action){
        }

        @Override
        public void onReaderOptionChanged(boolean enabled){
            NxpNfcLogger.d(TAG, "onReaderOptionChanged: " + enabled);
        }

        @Override
        public void onCardEmulationActivated(boolean isActivated) {
            NfcOperations.this.mIsCardEmulationActivated = isActivated;
            NxpNfcLogger.d(TAG, "onCardEmulationActivated: " + isActivated);
            if (mCallbackCountDownLatch != null) updateOemCallbackMap("onCardEmulationActivated");
        }

        @Override
        public void onRfFieldDetected(boolean isActive) {
            NfcOperations.this.mIsRfFieldDetected = isActive;
            NxpNfcLogger.d(TAG, "onRfFieldDetected: " + isActive);
            if (mNxpOemCallbacks != null) {
                mNxpOemCallbacks.onRfFieldDetected(isActive);
            }
            if (mCallbackCountDownLatch != null) updateOemCallbackMap("onRfFieldDetected");
        }

        @Override
        public void onRfDiscoveryStarted(boolean isDiscoveryStarted) {
            NxpNfcLogger.d(TAG, "onRfDiscoveryStarted: " + isDiscoveryStarted);
            NfcOperations.this.mIsDiscoveryStarted = isDiscoveryStarted;
            if (mDisCountDownLatch != null) mDisCountDownLatch.countDown();
            if (mCallbackCountDownLatch != null) updateOemCallbackMap("onRfDiscoveryStarted");
        }

        @Override
        public void onEeListenActivated(boolean isActivated) {
            NfcOperations.this.mIsEeListenActivated = isActivated;
            NxpNfcLogger.d(TAG, "mIsEeListenActivated: " + isActivated);
            if (mCallbackCountDownLatch != null) updateOemCallbackMap("onEeListenActivated");
        }

        @Override
        public void onEeUpdated() {}

        @Override
        public void onGetOemAppSearchIntent(List<String> packages,
                                    Consumer<Intent> intentConsumer) {
          intentConsumer.accept(new Intent());
        }

        @Override
        public void onNdefMessage(Tag tag, NdefMessage message,
                           Consumer<Boolean> hasOemExecutableContent) {
          hasOemExecutableContent.accept(false);
        }

        @Override
        public void onLaunchHceAppChooserActivity(String selectedAid,
                                           List<ApduServiceInfo> services,
                                           ComponentName failedComponent,
                                          String category) {
        }

        @Override
        public void onLaunchHceTapAgainDialog(ApduServiceInfo service, String category) {
        }

        @Override
        public void onRoutingTableFull(){
        }

        @Override
        public void onLogEventNotified(OemLogItems item){
        }

        @Override
        public void onExtractOemPackages(
            NdefMessage message, Consumer<List<String>> packageConsumer) {
          packageConsumer.accept(Collections.emptyList());
        }
    };

    ControllerAlwaysOnListener mControllerAlwaysOnListener = new ControllerAlwaysOnListener() {

        @Override
        public void onControllerAlwaysOnChanged(boolean isEnabled) {
            NxpNfcLogger.d(TAG, "onControllerAlwaysOnChanged: " + isEnabled);
            if (mControllerAlwaysOnLatch != null) mControllerAlwaysOnLatch.countDown();

        }
    };

    /**
     * @brief Getter of {@link #mIsTagConnected}
     */
    public boolean isTagConnected() {
        if (mNxpOemCallbacks == null) {
            NxpNfcLogger.e(TAG, "Exception: OEM callback is not registered");
            throw new IllegalStateException("OEM callback is not registered");
        }
        return this.mIsTagConnected;
    }

    /**
     * @brief Getter of {@link #mIsRfFieldActivated}
     */
    public boolean isRfFieldDetected() throws IllegalStateException {
        if (mNxpOemCallbacks == null) {
            NxpNfcLogger.e(TAG, "Exception: OEM callback is not registered");
            throw new IllegalStateException("OEM callback is not registered");
        }
        return this.mIsRfFieldDetected;
    }

    /**
     * @brief Getter of {@link #mIsCardEmulationActivated}
     */
    public boolean isCardEmulationActivated() throws IllegalStateException {
        if (mNxpOemCallbacks == null) {
            NxpNfcLogger.e(TAG, "Exception: OEM callback is not registered");
            throw new IllegalStateException("OEM callback is not registered");
        }
        return this.mIsCardEmulationActivated;
    }

    /**
     * @brief Getter of {@link #mIsEeListenActivated}
     */
    public boolean isEeListenActivated() throws IllegalStateException {
        if (mNxpOemCallbacks == null) {
            NxpNfcLogger.e(TAG, "Exception: OEM callback is not registered");
            throw new IllegalStateException("OEM callback is not registered");
        }
        return this.mIsEeListenActivated;
    }

    /**
     * @brief Getter of {@link #mIsDiscoveryStarted}
     */
    public boolean isDiscoveryStarted() throws IllegalStateException {
        if (mNxpOemCallbacks == null) {
            NxpNfcLogger.e(TAG, "Exception: OEM callback is not registered");
            throw new IllegalStateException("OEM callback is not registered");
        }
        return this.mIsDiscoveryStarted;
    }

    /**
     * @brief Query NfcAdapter Nfc is enabled or not
     * @return true if NFC is enabled else false
     */
    public boolean isEnabled() {
        return mNfcAdapter.isEnabled();
    }

    /**
     * @brief Query NfcAdapter ControllerAlwaysOn is enabled or not
     * @return true if ControllerAlwaysOn is on else false
     */
    public boolean isControllerAlwaysOn() {
        return mNfcAdapter.isControllerAlwaysOn();
    }

    private class EnableFinishedTask extends AsyncTask<Integer, Void, Void> {

        @Override
        protected Void doInBackground(Integer... params) {
            NxpNfcLogger.d(TAG, "doInBackground");
            if (params == null || params.length == 0) {
                NxpNfcLogger.e(TAG, "doInBackground: params is null or empty");
                return null;
            }
            if (mNxpOemCallbacks != null) {
                mNxpOemCallbacks.onEnableFinished(params[0]);
            }
            return null;
        }
    }

    private class BootFinishedTask extends AsyncTask<Integer, Void, Void> {

        @Override
        protected Void doInBackground(Integer... params) {
            NxpNfcLogger.d(TAG, "doInBackground");
            if (params == null || params.length == 0) {
                NxpNfcLogger.e(TAG, "doInBackground: params is null or empty");
                return null;
            }
            if (mNxpOemCallbacks != null) {
                mNxpOemCallbacks.onBootFinished(params[0]);
            }
            return null;
        }
    }
}
