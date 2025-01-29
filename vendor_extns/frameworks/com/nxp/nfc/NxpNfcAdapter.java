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
import android.nfc.Tag;
import com.nxp.nfc.vendor.mpos.MposHandler;
import com.nxp.nfc.vendor.qtag.QTagHandler;
import java.io.IOException;

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
    private MposHandler mMposHandler;
    private QTagHandler mQTagHandler;

    /**
     * @brief private constructor to create the instance of {@link NxpNfcAdapter}
     * @param nfcAdapter
     */
    private NxpNfcAdapter(NfcAdapter nfcAdapter) {
        mNfcAdapter = nfcAdapter;
        mMposHandler = new MposHandler(nfcAdapter);
        mQTagHandler = new QTagHandler(nfcAdapter);
    }

    public interface NxpReaderCallback {
      void onNxpTagDiscovered(Tag tag, boolean isNxpTagDetected);
    }

    /**
     * @brief NxpNfcAdapter for application context,
     * or throws UnsupportedOperationException nfcAdapter is null.
     *
     * @param nfcAdapter
     * @return {@link NxpNfcAdapter} instance
     */
    public static synchronized NxpNfcAdapter getNxpNfcAdapter(NfcAdapter nfcAdapter) {
        if (sNxpNfcAdapter == null) {
            if (nfcAdapter == null) {
                NxpNfcLogger.e(TAG, "nfcAdapter is null");
                throw new UnsupportedOperationException();
            }
            sNxpNfcAdapter = new NxpNfcAdapter(nfcAdapter);
        }
        return sNxpNfcAdapter;
    }

    /**
     * @brief getter for accessing {@link INxpNfcAdapter}
     * make sure to call {@link #getNxpNfcAdapter()} before calling this
     * throws UnsupportedOperationException {@link #sNxpNfcAdapter} is null.
     * @return {@link INxpNfcAdapter} instance
     */
    public static INxpNfcAdapter getNxpNfcAdapterInterface() {
        if (sNxpNfcAdapter == null) {
            throw new UnsupportedOperationException(
                "You need a reference from NxpNfcAdapter to use the "
                + " NXP NFC APIs");
        }
        return ((INxpNfcAdapter) sNxpNfcAdapter);
    }

    /**
     * @brief To be called to start or stop the mPOS reader mode
     * @return {@link INxpNfcAdapter.mPOSSetReaderMode} instance
     */
    @Override
    public int mPOSSetReaderMode(String pkg, boolean on) throws IOException {
      return mMposHandler.mPOSSetReaderMode(pkg, on);
    }

    /**
     * @brief This is provides the info whether mPOS mode is activated or not
     * @return {@link INxpNfcAdapter.mPOSGetReaderMode} instance
     */
    @Override
    public boolean mPOSGetReaderMode(String pkg) throws IOException {
      return mMposHandler.mPOSGetReaderMode(pkg);
    }

    /**
     * @brief To be called to enable QTag
     * @return {@link INxpNfcAdapter.enableQTag} instance
     */
    @Override
    public int enableQTag(Activity activity, NxpReaderCallback mQTagCallback,
                          int mode, int pollTech, int delay_value)
        throws IOException {
      return mQTagHandler.enableQTag(activity, mode, mQTagCallback, pollTech,
                                     delay_value);
    }
}
