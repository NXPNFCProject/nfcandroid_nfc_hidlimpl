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

import android.app.Activity;
import android.nfc.NfcAdapter;

/**
 * @class NxpNfcAdapter
 * @brief Concrete implementation of NFC Extension features
 *
 */
public final class NxpNfcAdapter implements INxpNfcAdapter {
    private static final String TAG = "NxpNfcAdapter";

    /**
     * @brief singleton instance object
     */
    private static NxpNfcAdapter sNxpNfcAdapter;

    private NfcAdapter mNfcAdapter;

    /**
     * @brief private constructor to create the instance of {@link NxpNfcAdapter}
     * @param nfcAdapter
     * @param activity
     */
    private NxpNfcAdapter(NfcAdapter nfcAdapter, Activity activity) {
        mNfcAdapter = nfcAdapter;
    }

    /**
     * @brief NxpNfcAdapter for application context,
     * or throws UnsupportedOperationException nfcAdapter is null.
     *
     * @param nfcAdapter
     * @param activity
     * @return {@link NxpNfcAdapter} instance
     */
    public static synchronized NxpNfcAdapter getNxpNfcAdapter(NfcAdapter nfcAdapter,
            Activity activity) {
        if (sNxpNfcAdapter == null) {
            if (nfcAdapter == null) {
                NxpNfcLogger.e(TAG, "nfcAdapter is null");
                throw new UnsupportedOperationException();
            }
            sNxpNfcAdapter = new NxpNfcAdapter(nfcAdapter, activity);
        }
        return sNxpNfcAdapter;
    }

    /**
     * @brief getter for accessing {@link INxpNfcAdapter}
     * @return {@link INxpNfcAdapter} instance
     */
    public INxpNfcAdapter getNxpNfcAdapterInterface() {
        return ((INxpNfcAdapter) this);
    }
}
