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

import android.util.Log;

/**
 * @class NxpNfcLogger
 * @brief A wrapper class for logging functionality.
 *
 * This interface provides methods for logging messages of various security levels.
 * @hide
 */
public class NxpNfcLogger {

    private static final String TAG = "ComNxpNfc";
    private static int sLogLevel = Log.VERBOSE;

    public static void setLogLevel(int logLevel) {
        sLogLevel = logLevel;
    }

    public static void log(int priority, String tag, String message) {
        if (priority >= sLogLevel) {
            Log.println(priority, TAG, tag + ":" + message);
        }
    }

    public static void v(String tag, String message) {
        log(Log.VERBOSE, tag, message);
    }

    public static void d(String tag, String message) {
        log(Log.DEBUG, tag, message);
    }

    public static void i(String tag, String message) {
        log(Log.INFO, tag, message);
    }

    public static void w(String tag, String message) {
        log(Log.WARN, tag, message);
    }

    public static void e(String tag, String message) {
        log(Log.ERROR, tag, message);
    }
}
