/*
 *
 *  The original Work has been changed by NXP.
 *
 *  Copyright 2024 NXP
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
package com.nxp.nfc.oem.stag;
/*
 * @hide
 */
public class STagConstants {

    static final int MAX_STAG_RETRY   = 0x03;
    static final int STAG_RETRY_DELAY_MS = 70;

    static final int NXP_STAG_START_AUTH_DEFAULT_TIMEOUT_MS = 30;
    static final int NXP_STAG_TRANS1_DEFAULT_TIMEOUT_MS     = 100;
    static final int NXP_STAG_TRANS2_DEFAULT_TIMEOUT_MS     = 1000;
    static final int NXP_STAG_DEFAULT_CMD_TIMEOUT_MS        = 100;

    static final int NXP_STAG_TIMEOUT_BUF_LEN = 0x04;

    /*S-tag command timeout scaling factor of 10 in ms*/
    static final int NXP_STAG_CMD_TIMEOUT_DEFAULT_SCALING_FACTOR_MS = 10;

    static final byte MAINLINE_RES_MIN_LEN          = 0x02;
    static final byte MAINLINE_STATUS_INDEX         = 0x01;
    static final byte STATUS_INDEX                  = 0x00;
    static final byte MAINLINE_SUB_GID_OID_INDEX    = 0x00;
    static final byte STATUS_SUCCESS                = 0x00;

    static final byte STATUS_STARTED        = 0x00;
    static final byte STATUS_REJECTED       = 0x01;
    static final byte STATUS_FAILED         = 0x03;
    static final byte STATUS_FIELD_DETECTED = (byte) 0xE0;
    static final byte STATUS_NOT_STARTED    = (byte) 0xFF;

    static final byte NFC_STAG_STATUS_BYTE_0 = (byte) 0xB0;
    static final byte NFC_STAG_STATUS_BYTE_1 = (byte) 0xB1;
    static final byte NFC_STAG_STATUS_BYTE_2 = (byte) 0xB2;
    static final byte AUTH_STATUS_EFD_ON     = (byte) 0x1E;

    public static final int AUTH_CMD_TIMEOUT_SEC = 2; /* S-tag command timeout in secs*/

    public static final int AUTH_START_SUB_GID_OID    = 0x10;
    public static final int AUTH_TRANS_SUB_GID_OID    = 0x11;
    public static final int AUTH_STOP_SUB_GID_OID     = 0x12;
    public static final int AUTH_ERR_NTF_SUB_GID_OID  = 0x13;

    enum CmdType {
        START_POLL,
        STOP_POLL,
    }
}
