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

package com.nxp.nfc;

/**
 * @class NxpNfcConstants
 * @brief A utility class for global constants used across the module
 *
 */
public interface NxpNfcConstants {

    int SEND_RAW_WAIT_TIME_OUT_VAL   = 4000;

    int RF_PROTOCOL_ERR_CODE         = 0xB1;
    int TIMEOUT_ERR_CODE             = 0xB2;
    int RF_UNEXPECTED_DATA_ERR_CODE  = 0xB3;

    int NFC_NCI_PROP_GID = 0x2F;
    int NXP_NFC_PROP_OID = 0x70;

    int STR_SET_FLAG_GID = 0x2F;
    int STR_SET_FLAG_OID = 0x48;
}
