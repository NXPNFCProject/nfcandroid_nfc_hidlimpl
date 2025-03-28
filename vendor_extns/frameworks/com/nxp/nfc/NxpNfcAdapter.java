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

import android.app.Activity;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import com.nxp.nfc.vendor.mpos.MposHandler;
import com.nxp.nfc.vendor.qtag.QTagHandler;
import com.nxp.nfc.vendor.transit.TransitConfigHandler;

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
    private TransitConfigHandler mTransitHandler;

    /**
     * @brief supported chipsets
     */
    private static final int NXP_EN_SN110U = 1;
    private static final int NXP_EN_SN100U = 1;
    private static final int NXP_EN_SN220U = 1;
    private static final int NXP_EN_PN557 = 1;
    private static final int NXP_EN_PN560 = 1;
    private static final int NXP_EN_SN300U = 1;
    private static final int NXP_EN_SN330U = 1;

    private static final int NFC_NXP_MW_ANDROID_VER = 16; // Android version used by NFC MW
    private static final int NFC_NXP_MW_VERSION_MAJ = 0x05; // MW Major Version
    private static final int NFC_NXP_MW_VERSION_MIN = 0x00; // MW Minor Version
    private static final int NFC_NXP_MW_CUSTOMER_ID = 0x00; // MW Customer ID
    private static final int NFC_NXP_MW_RC_VERSION = 0x00; // MW RC Version

    private static void printComNxpNfcVersion() {
        int validation = (NXP_EN_SN100U << 13);
        validation |= (NXP_EN_SN110U << 14);
        validation |= (NXP_EN_SN220U << 15);
        validation |= (NXP_EN_PN560 << 16);
        validation |= (NXP_EN_SN300U << 17);
        validation |= (NXP_EN_SN330U << 18);
        validation |= (NXP_EN_PN557 << 11);

        String logMessage = String.format(
            "NxpNfcJar Version: NXP_AR_%02X_%05X_%02d.%02X.%02X",
            NFC_NXP_MW_CUSTOMER_ID, validation, NFC_NXP_MW_ANDROID_VER,
            NFC_NXP_MW_VERSION_MAJ, NFC_NXP_MW_VERSION_MIN);
        NxpNfcLogger.d(TAG, logMessage);
    }

    /**
     * @brief private constructor to create the instance of {@link NxpNfcAdapter}
     * @param nfcAdapter
     */
    private NxpNfcAdapter(NfcAdapter nfcAdapter) {
        printComNxpNfcVersion();
        mNfcAdapter = nfcAdapter;
        mMposHandler = new MposHandler(nfcAdapter);
        mQTagHandler = new QTagHandler(nfcAdapter);
        mTransitHandler = new TransitConfigHandler(nfcAdapter);
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

    /**
     * @brief To be called to set NCI configuration
     * @return {@link INxpNfcAdapter.mTransitHandler} instance
     */
    @Override
    public boolean setConfig(String configs) throws IOException {
      return mTransitHandler.setConfig(configs);
    }
}
