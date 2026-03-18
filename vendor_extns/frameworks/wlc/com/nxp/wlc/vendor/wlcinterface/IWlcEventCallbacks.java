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

package com.nxp.wlc.vendor.wlcinterface;

/**
 * Interface to perform WLC releated callbacks to apps.
 *
 */
public interface IWlcEventCallbacks {
    /**
     * This callback triggers once proprietary
     * WLC_STATUS_NTF arrived in autonomous mode
     * with {@link WlcEventHandler#NCI_WLC_PROP_NTF_GID_OID_VAL}
     * @param wlcStatusNfc : WLC_STATUS_NTF
     */
    void onPropWlcStatusNtf(byte[] wlcStatusNfc);

    /**
     * This callback triggers WLC status notifications
     * based on NCI.23 specification
     * with {@link WlcEventHandler#NCI_WLC_RF_NTF_GID_OID_VAL}
     * @param wlcStatusNfc : WLC_STATUS_NTF
     */
    void onRfWlcStatusNtf(byte[] wlcStatusNfc);

    /**
     * This callback triggers on receiving Data.
     * @param wlcData : Data packet received from NFCC
     */
    void onWlcDataReceived(byte[] wlcData);

    /**
     * This callback triggers once Wlc RF interface is Activated.
     */
    void onWlcListnerDetected();

    /**
     * This callback triggers once Wlc RF interface is DeActivated.
     */
    void onWlcListnerRemoved();
}
