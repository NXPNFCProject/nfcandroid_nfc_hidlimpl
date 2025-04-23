/*
 * Copyright 2025 NXP
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

/**
 * @interface INxpOEMCallbacks
 * @brief Interface to perform the NFC OEM callbacks.
 *
 * @hide
 */
public interface INxpOEMCallbacks {

    String TAG = "INxpOEMCallbacks";

    /**
     * callback when tag connects/disconnects
     * @param connected : true in case of connects
     *                  false when disconnects
     */
    default void onTagConnected(boolean connected) {}

    /**
     * callback when nfc state is upadated
     *@param state : nfc state
     */
    default void onStateUpdated(int state) {}

    /**
     * callback before apply routing command
     * to be skipped or contintue
     * @return : true if oem wants to skip apply routing
     *           false if oem wants to continue apply routing
     */
    default boolean onApplyRouting() {
        return false;
    }

    /**
     * callback before ndef read to check if need to skipped
     * @return : true if oem wants to skip
     *           false if oem wants to continue
     */
    default boolean onNdefRead() {
        return false;
    }

    /**
     * callback before nfc enable
     * @return : true if oem allows
     *           false if oem do not allow
     */
    default boolean onEnableRequested() {
        return true;
    }

    /**
     * callback before nfc disable
     * @return : true if oem allows
     *           false if oem do not allow
     */
    default boolean onDisableRequested() {
        return true;
    }

    /**
     * callback when boot started
     */
    default void onBootStarted() {}

    /**
     * callback when nfc enable started
     */
    default void onEnableStarted() {}

    /**
     * callback when nfc disable started
     */
    default void onDisableStarted() {}

    /**
     * callback when boot completed
     */
    default void onBootFinished(int status) {}

    /**
     * callback when enable nfc finished
     */
    default void onEnableFinished(int status) {}

    /**
     * callback when disable nfc finished
     */
    default void onDisableFinished(int status) {}

    /**
     * callback before tag dispatch to check if need to skipped
     * @return : true if oem wants to skip
     *           false if oem wants to continue
     */
    default boolean onTagDispatch() {
        return false;
    }

    /**
     * callback before routing option change
     * to be skipped or contintue
     * @return : true if oem wants to skip routing change
     *           false if oem wants to continue routing change
     */
    default boolean onRoutingChanged() {
        return false;
    }

    /**
     * callback when HCE event received
     * @param action : event actions
     */
    default void onHceEventReceived(int action) {}

    /**
     * callback when reader optioned changed
     * @param enabled :  true when enabled false for disabled
     */
    default void onReaderOptionChanged(boolean enabled) {
        NxpNfcLogger.d(TAG, "onReaderOptionChanged: " + enabled);
    }

    /**
     * callback when card emulation activated or deactivated
     * @param isActivated :  true for activated false for deactivated
     */
    default void onCardEmulationActivated(boolean isActivated) {
        NxpNfcLogger.d(TAG, "onCardEmulationActivated: " + isActivated);
    }

    /**
     * callback when RF field detacted
     * @param isActive : true when rf activated
     *                             false when deactivated
     */
    default void onRfFieldDetected(boolean isActive) {
        NxpNfcLogger.d(TAG, "onRfFieldDetected: " + isActive);
    }

    /**
     * callback when discovery started or stopped
     * @param isDiscoveryStarted :  true for started false for stopped
     */
    default void onRfDiscoveryStarted(boolean isDiscoveryStarted) {
        NxpNfcLogger.d(TAG, "onRfDiscoveryStarted: " + isDiscoveryStarted);
    }

    /**
     * callback when ee listen activated
     * @param isActivated :  true for activate false for deactivate
     */
    default void onEeListenActivated(boolean isActivated) {}

    /**
     * callback when ee updated
     */
    default void onEeUpdated() {}

    /**
     * callback when routing table gets full
     */
    default void onRoutingTableFull() {}
}
