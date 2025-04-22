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

/**
 * @class NxpNfcUtils
 * @brief A utility class providing various helper function
 *
 * This class contains static methods for common utility operations
 */
public final class NxpNfcUtils {
    private static String sDigits = "0123456789abcdef";

    /**
     * @brief converts byte buffer to hex string
     * @param data required to hex string
     * @return string in hex format
     */
    public static String toHexString(byte[] data) {
        StringBuffer buf = new StringBuffer();
        if (data == null) {
            return null;
        }
        for (int i = 0; i != data.length; i++) {
            int v = data[i] & 0xff;
            buf.append(sDigits.charAt(v >> 4));
            buf.append(sDigits.charAt(v & 0xf));
        }
        return buf.toString();
    }
}
