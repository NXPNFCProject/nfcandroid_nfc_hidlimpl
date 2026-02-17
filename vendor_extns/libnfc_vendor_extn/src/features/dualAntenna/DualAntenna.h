/******************************************************************************
 *
 *  Copyright 2025-2026 NXP
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
 ******************************************************************************/
#pragma once

#include <condition_variable>
#include "NciDef.h"
#include "NfcExtensionConstants.h"
#include "PlatformAbstractionLayer.h"
#include "RfStateMonitor.h"
#include "phNfcStatus.h"
#include "phNfcTypes.h"
#include "phNxpLog.h"
#include <cstdint>
#include <vector>

struct DualAntennaContext {
  /*DualAntenna freature presence in the device*/
  bool mDualAntennaFeature;
  /*DualAntenna feature request*/
  bool mDualAntennaRequest;
  /*DualAntenna feature enabled/disabled*/
  bool mDualAntennaSetting;
  /*DualAntenna Q poll enabled*/
  bool mAppendQpoll;
  /*Type of DualAntenna feature
  0 --> Feature Disable
  1 --> Feature Enable
  2 --> Is Feature supported
  3 --> Set_disc_tech on both antennas
  4 --> Set reader mode
  */
  uint8_t mAntennaFeature;
  /*antenna one configuration*/
  uint8_t mAntOneConfig;
  /*antenna two configuration*/
  uint8_t mAntTwoConfig;
  /*DualAntenna readermode configuration*/
  uint8_t mConfigReaderMode;
};

enum DualAntennaSubOid : uint8_t {
  DUAL_ANTENNA_DISABLE = 0x40,
  DUAL_ANTENNA_ENABLE = 0x41,
  DUAL_ANTENNA_IS_SUPPORTED = 0x42,
  DUAL_ANTENNA_SET_DISCOVERY = 0x43,
  DUAL_ANTENNA_SET_READER_MODE = 0x44,
  DUAL_ANTENNA_GET_DISCOVERY_TECH = 0x45,
  DUAL_ANTENNA_GET_READER_MODE = 0x46
};

class DualAntenna {
public:
  /**
   * @brief Get the singleton instance of DualAntenna.
   * @return NxpDualAntenna* Pointer to the instance.
   */
  static DualAntenna *getInstance();

  /**
   * @brief Releases all the resources
   * @return None
   *
   */
  static inline void finalize() {
    if (mDualAntenna != nullptr) {
      mDualAntenna.reset();
    }
  }

  /**
   * @brief Process NCI message for DualAntenna.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleVendorNciMessage(uint16_t dataLen, uint8_t *pData);

  /**
   * @brief Process NCI response/notification for DualAntenna.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleVendorNciRspNtf(uint16_t dataLen, uint8_t *pData);

  /**
   * @brief To enable or disable DualAntenna feature.
   * @param flag True to enable the feature or false to disable the DualAntenna
   * feature
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS applyDualAntennaSettings(bool feature);

  /**
   * @brief To check DualAntenna feature is enabled or not.
   * @return true if feature is enabled else false
   */
  bool isDualAntennaSupported();

  /**
   * @brief To check whether the phone is foldable or not.
   * @return true if device is foldable else false
   */
  bool isDualAntennaFoldable();

  /**
   * @brief Send the Discovery tech command to start polling .
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS setDiscoveryTechnology_DualAntenna();

  /**
   * @brief Send the core set config command to enable reader mode on either
   * antenna's.
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS setDualAntennaPollMode();

  /**
   * @brief send RF deactivate to idle command.
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS sendRfDeactivate(const uint8_t *pData);

  /**
   * @brief send RF discovery command.
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS sendRfDiscCmd();

  /**
   * @brief send CON_DISC_PARAM command.
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS sendConDiscParamCmd();

  constexpr static uint8_t DUAL_ANTENNA_SUB_GID_OID = 0x40;

private:
  static std::unique_ptr<DualAntenna>
      mDualAntenna; /* Singleton instance of DualAntenna */
  DualAntennaContext mDualAntennaContext;
  constexpr static uint8_t DUAL_ANTENNA_FEATURE_SUPPORTED = 0x01;
  constexpr static uint8_t NCI_PROP_RSP_VAL = 0x4F;
  constexpr static uint8_t DUAL_ANTENNA_PAYLOAD_TWO_LEN = 0x02;
  constexpr static uint8_t DUAL_ANTENNA_GET_DISC_PAYLOAD_TWO_LEN = 0x03;
  constexpr static uint8_t DUAL_ANTENNA_FOLDABLE = 0X03;
  constexpr static uint8_t RESPONSE_STATUS_OK = 0x00;
  constexpr static uint8_t RESPONSE_STATUS_FAILED = 0x01;
  constexpr static uint8_t DUAL_ANTENNA_SUB_GID_OID_INDEX = 3;
  constexpr static uint8_t DUAL_ANTENNA_PHONE_STATE_INDEX = 4;
  constexpr static uint8_t DUAL_ANTENNA_ANTENNA_ONE_CONF_INDEX = 4;
  constexpr static uint8_t DUAL_ANTENNA_ANTENNA_TWO_CONF_INDEX = 5;
  constexpr static uint8_t DUAL_ANTENNA_NFC_PASSIVE_ABFKQ = 0x05;
  constexpr static uint8_t DUAL_ANTENNA_NFC_PASSIVE_Q = 0x0D;
  constexpr static uint8_t READER_MODE_CONFIG_INDEX = 6;
  constexpr static uint8_t CON_DISC_PARAM_LENGTH = 7;
  constexpr static uint8_t CMD_MIN_DATA_LENGTH = 4;
  DualAntenna();
  ~DualAntenna();
  friend struct std::default_delete<DualAntenna>;
};
