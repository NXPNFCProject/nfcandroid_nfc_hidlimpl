/*
 *
 *  The original Work has been changed by NXP.
 *
 *  Copyright 2025 NXP
 *
 *  Licensed under the Apache License, Version 2.0  = the "License");
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

package com.nxp.nfc.vendor.transit;

import java.util.Map;

public class RfRegisterConstants {

  public static final byte UPDATE_DLMA_ID_TX_ENTRY = 0x00;
  public static final byte UPDATE_MIFARE_NACK_TO_RATS_ENABLE= 0x01;
  public static final byte UPDATE_MIFARE_MUTE_TO_RATS_ENABLE= 0x02;
  public static final byte UPDATE_ISO_DEP_MERGE_SAK = 0x03;
  public static final byte UPDATE_PHONEOFF_TECH_DISABLE = 0x04;
  public static final byte UPDATE_RF_CM_TX_UNDERSHOOT_CONFIG = 0x05;
  public static final byte UPDATE_CHINA_TIANJIN_RF_ENABLED = 0x06;
  public static final byte UPDATE_CN_TRANSIT_CMA_BYPASSMODE_ENABLE = 0x07;
  public static final byte UPDATE_CN_TRANSIT_BLK_NUM_CHECK_ENABLE = 0x08;
  public static final byte UPDATE_RF_PATTERN_CHK = 0x09;
  public static final byte UPDATE_GUARD_TIMEOUT_TX2RX = 0x0A;
  public static final byte UPDATE_INITIAL_TX_PHASE = 0x0B;
  public static final byte UPDATE_LPDET_THRESHOLD = 0x0C;
  public static final byte UPDATE_NFCLD_THRESHOLD = 0x0D;

  static Map<String, Byte> RfConfigMap = Map.ofEntries(
    Map.entry("UPDATE_DLMA_ID_TX_ENTRY", UPDATE_DLMA_ID_TX_ENTRY),
    Map.entry("UPDATE_MIFARE_NACK_TO_RATS_ENABLE", UPDATE_MIFARE_NACK_TO_RATS_ENABLE),
    Map.entry("UPDATE_MIFARE_MUTE_TO_RATS_ENABLE", UPDATE_MIFARE_MUTE_TO_RATS_ENABLE),
    Map.entry("UPDATE_ISO_DEP_MERGE_SAK", UPDATE_ISO_DEP_MERGE_SAK),
    Map.entry("UPDATE_PHONEOFF_TECH_DISABLE", UPDATE_PHONEOFF_TECH_DISABLE),
    Map.entry("UPDATE_RF_CM_TX_UNDERSHOOT_CONFIG", UPDATE_RF_CM_TX_UNDERSHOOT_CONFIG),
    Map.entry("UPDATE_CHINA_TIANJIN_RF_ENABLED", UPDATE_CHINA_TIANJIN_RF_ENABLED),
    Map.entry("UPDATE_CN_TRANSIT_CMA_BYPASSMODE_ENABLE", UPDATE_CN_TRANSIT_CMA_BYPASSMODE_ENABLE),
    Map.entry("UPDATE_CN_TRANSIT_BLK_NUM_CHECK_ENABLE", UPDATE_CN_TRANSIT_BLK_NUM_CHECK_ENABLE),
    Map.entry("UPDATE_RF_PATTERN_CHK", UPDATE_RF_PATTERN_CHK),
    Map.entry("UPDATE_GUARD_TIMEOUT_TX2RX", UPDATE_GUARD_TIMEOUT_TX2RX),
    Map.entry("UPDATE_INITIAL_TX_PHASE", UPDATE_INITIAL_TX_PHASE),
    Map.entry("UPDATE_LPDET_THRESHOLD", UPDATE_LPDET_THRESHOLD),
    Map.entry("UPDATE_NFCLD_THRESHOLD", UPDATE_NFCLD_THRESHOLD)
  );
}
