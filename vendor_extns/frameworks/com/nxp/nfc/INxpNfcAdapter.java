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

import com.nxp.nfc.INxpNfcExtras;

/**
 * @class INxpNfcAdapter
 * @brief Interface to perform the NFC Extension functionality.
 *
 * @hide
 */
interface INxpNfcAdapter {

    /**
     * @brief gets the OEM Extension interface
     *
     * @return OEM Extension interface
     */
    INxpNfcExtras getNxpNfcExtrasInterface();
}
