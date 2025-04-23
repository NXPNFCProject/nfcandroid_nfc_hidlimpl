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

package com.nxp.nfc.vendor.lxdebug;

/**
 * @interface ILxDebugDataCallbacks
 * @brief Interface to perform LxDebug releated callbacks to apps.
 *
 */
public interface ILxDebugCallbacks {
    /**
     * This callback triggers when EFDM timer got expired .
     */
    void onEFDMTimedout();

    /**
     * This callback triggers on receiving the LX Debug data.
     * @param lxDebugData : LX Dbug data received from NFCC
     */
    void onLxDebugDataReceived(byte[] lxDebugData);
}
