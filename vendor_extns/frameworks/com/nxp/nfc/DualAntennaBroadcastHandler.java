/*
 * Copyright 2026 NXP
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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.nfc.NfcAdapter;
import android.util.Log;
import com.nxp.nfc.NxpNfcAdapter.DualAntennaCallback;
import com.nxp.nfc.vendor.dualAntenna.DualAntennaHandler;

public class DualAntennaBroadcastHandler extends BroadcastReceiver {

  public static final String ACTION_CUSTOM_EVENT =
      "com.nxp.nfc.DualAntennaBroadcastHandler";
  public static final String EXTRA_MESSAGE = "extra_message";
  public static final String EXTRA_INT_VALUE = "EXTRA_INT_VALUE";

  public static final String EXTRA_ANTENNA1_VALUE = "EXTRA_ANTENNA1_VALUE";
  public static final String EXTRA_ANTENNA2_VALUE = "EXTRA_ANTENNA2_VALUE";
  public static final String EXTRA_ACTION_TYPE = "EXTRA_ACTION_TYPE";
  public static final String EXTRA_READER_MODE_ANTENNA1_VALUE =
      "EXTRA_READER_MODE_ANTENNA1_VALUE";
  public static final String EXTRA_READER_MODE_ANTENNA2_VALUE =
      "EXTRA_READER_MODE_ANTENNA2_VALUE";
  public static int readerModeAntenna1Value = 0x00;
  public static int readerModeAntenna2Value = 0x00;
  private static final String TAG = "DualAntenna";
  private NfcAdapter mNfcAdapter;
  private DualAntennaHandler mDualAntennaHandler;
  private boolean mIsReaderModeApplied = false;

  public enum ModeSetting {
    Disable(0x00),
    Enable(0x01);
    public final int value;
    ModeSetting(int value) { this.value = (int)value; }
    public int getValue() { return value; }
  }

  public enum FlipEvent {
    Open(0x01),
    Close(0x00);
    public final int value;
    FlipEvent(int value) { this.value = (int)value; }
    public int getValue() { return value; }
  }

  public DualAntennaBroadcastHandler(NfcAdapter nfcAdapter) {
    mNfcAdapter = nfcAdapter;
    NxpNfcLogger.d(TAG, "initializing the DualAntenna handler");
    mDualAntennaHandler = new DualAntennaHandler(nfcAdapter);
  }

  public DualAntennaBroadcastHandler() {}

  @Override
  public void onReceive(Context context, Intent intent) {
    if (intent != null && intent.getAction() != null) {
      switch (intent.getAction()) {
      case ACTION_CUSTOM_EVENT:
        String message = intent.getStringExtra(EXTRA_MESSAGE);
        String actionType = intent.getStringExtra(EXTRA_ACTION_TYPE);
        NxpNfcLogger.d(TAG, "Received message: " + message);
        // Handle different types of actions
        if ("ANTENNA_CONFIG".equals(actionType)) {
          int antennaOneValue = intent.getIntExtra(EXTRA_ANTENNA1_VALUE, -1);
          int antennaTwoValue = intent.getIntExtra(EXTRA_ANTENNA2_VALUE, -1);
          handleAntennaConfiguration(antennaOneValue, antennaTwoValue);
        } else if ("READER_MODE_CONFIG".equals(actionType)) {
          readerModeAntenna1Value =
              intent.getIntExtra(EXTRA_READER_MODE_ANTENNA1_VALUE, -1);
          readerModeAntenna2Value =
              intent.getIntExtra(EXTRA_READER_MODE_ANTENNA2_VALUE, -1);
          handleReaderModeConfiguration(readerModeAntenna1Value,
                                        readerModeAntenna2Value);
        } else if ("FLIP_EVENT_CONFIG".equals(actionType)) {
          // Handle flip events
          int value = intent.getIntExtra(EXTRA_INT_VALUE, -1);
          handleFlipEvent(message, value);
        } else if ("GET_DISCOVERY_TECH".equals(actionType)) {
          int value = intent.getIntExtra(EXTRA_INT_VALUE, -1);
          handleGetDiscoveryTech();
        } else if ("GET_POLLING_MODE".equals(actionType)) {
          handleGetPollingMode();
        }
        break;
      }
    }
  }

  private void handleFlipEvent(String message, int value) {
    // Process the flip broadcast event
    NxpNfcLogger.d(TAG, "Processing flip event with value: " + value);

    if (mDualAntennaHandler != null) {
      try {
        if (value == FlipEvent.Open.value) {
          // Flip close to open
          if (!mIsReaderModeApplied) {
            mDualAntennaHandler.setPollingMode_DualAntenna(
                ModeSetting.Enable.value, ModeSetting.Enable.value);
          } else {
            handleReaderModeConfiguration(readerModeAntenna1Value,
                                          readerModeAntenna2Value);
          }
        } else if (value == FlipEvent.Close.value) {
          // Flip open to close
          mDualAntennaHandler.setPollingMode_DualAntenna(
              ModeSetting.Enable.value, ModeSetting.Disable.value);
        }
      } catch (Exception e) {
        NxpNfcLogger.e(TAG, "Exception in flip event: " + e.getMessage());
      }
    } else {
      NxpNfcLogger.e(TAG,
                     "DualAntenna handler is not initialized for flip event");
    }
  }

  private void handleAntennaConfiguration(int antennaOneValue,
                                          int antennaTwoValue) {
    // Process antenna configuration
    NxpNfcLogger.d(TAG, "Processing antenna configuration - Antenna1: 0x" +
                            Integer.toHexString(antennaOneValue) +
                            ", Antenna2: 0x" +
                            Integer.toHexString(antennaTwoValue));

    if (mDualAntennaHandler != null) {
      try {
        mDualAntennaHandler.setDiscoveryTechnology_DualAntenna(antennaOneValue,
                                                               antennaTwoValue);
      } catch (Exception e) {
        NxpNfcLogger.e(TAG,
                       "Exception in antenna configuration: " + e.getMessage());
      }
    } else {
      NxpNfcLogger.e(
          TAG, "DualAntenna handler is not initialized for antenna config");
    }
  }

  private void handleReaderModeConfiguration(int readerModeAntenna1Value,
                                             int readerModeAntenna2Value) {
    NxpNfcLogger.d(TAG, "Processing reader mode configuration");
    if (mDualAntennaHandler != null) {
      try {
        // Validate reader mode values
        if (readerModeAntenna1Value < 0 || readerModeAntenna2Value < 0) {
          return;
        }
        // Apply reader mode configuration for both antennas
        mDualAntennaHandler.setPollingMode_DualAntenna(readerModeAntenna1Value,
                                                       readerModeAntenna2Value);
        mIsReaderModeApplied = true;
      } catch (Exception e) {
        NxpNfcLogger.e(TAG, "Exception in reader mode configuration: " +
                                e.getMessage());
      }
    } else {
      NxpNfcLogger.e(
          TAG, "DualAntenna handler is not initialized for reader mode config");
    }
  }

  private void handleGetDiscoveryTech() {
    NxpNfcLogger.d(TAG, "Processing get discovery tech");
    if (mDualAntennaHandler != null) {
      try {
        int[] antennaConf =
            mDualAntennaHandler.getDiscoveryTechnology_DualAntenna();
      } catch (Exception e) {
        NxpNfcLogger.e(TAG,
                       "Exception in get discovery tech " + e.getMessage());
      }
    } else {
      NxpNfcLogger.e(
          TAG, "DualAntenna handler is not initialized for get discovery tech");
    }
  }

  private void handleGetPollingMode() {
    NxpNfcLogger.d(TAG, "Processing get polling mode");
    if (mDualAntennaHandler != null) {
      try {
        int antennaPolling = mDualAntennaHandler.getPollingMode_DualAntenna();
      } catch (Exception e) {
        NxpNfcLogger.e(TAG, "Exception in get polling mode " + e.getMessage());
      }
    } else {
      NxpNfcLogger.e(
          TAG, "DualAntenna handler is not initialized for get polling mode");
    }
  }

  public void register(Context context) {
    IntentFilter filter = new IntentFilter();
    filter.addAction(ACTION_CUSTOM_EVENT);
    context.registerReceiver(this, filter);
  }

  public void unregister(Context context) { context.unregisterReceiver(this); }
}
