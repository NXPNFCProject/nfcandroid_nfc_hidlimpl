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
#ifndef NFC_AUTOCARD_H
#define NFC_AUTOCARD_H

#include <condition_variable>
#include "NciDef.h"
#include "NciStateMonitor.h"
#include "NfcExtensionConstants.h"
#include "PlatformAbstractionLayer.h"
#include "phNfcStatus.h"
#include "phNfcTypes.h"
#include "phNxpLog.h"
#include <cstdint>
#include <mutex>
#include <vector>

#define DEFAULT_STR_PHONEOFF_SUPPORT_MIN_FW_VER 0x022052

typedef struct {
  uint8_t pipeId;
  uint8_t status;
  std::vector<uint8_t> data;
} HciRspPkt;
typedef struct {
  uint8_t pipeId;
  uint8_t event;
  std::vector<uint8_t> data;
} HciEvtPkt;

/**
 * @brief Manager class to handle the AutoCard operations.
 */
class AutoCard {
public:
  /**
   * @brief Get the singleton instance of AutoCard.
   * @return AutoCard* Pointer to the instance.
   */
  static AutoCard *getInstance();

  /**
   * @brief handle Autocard command.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleVendorNciMessage(uint16_t dataLen, uint8_t *pData);

  /**
   * @brief Process NCI response/notification for ntag.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleVendorNciRspNtf(uint16_t dataLen, uint8_t *pData);

  /**
   * @brief get autocard configurations from config and enable/disable the
   * feature .
   */
  void phNxpNciHal_getAutoCardConfig();

  /**
   * @brief Create APDU pipe if wiredSE is enabled.
   */
  void phNxpNciHal_checkAndCreateApduPipe();
  /**
   * @brief handle HCI response.
   * @param rspLen Length of the hci response
   * @param rsp Pointer to the response data
   * @return returns NFCSTATUS_SUCCESS,
   */
  NFCSTATUS phNxpNciHal_handleHciAutoCardRsp(uint8_t *rsp, uint16_t rspLen);
  /**
   * @brief send HCI cpmmand.
   * @param data HCI command
   * @param rsp hci response will be update
   * @return returns NFCSTATUS_SUCCESS,
   */
  NFCSTATUS sendHciCmd(std::vector<uint8_t> &data, HciRspPkt &rsp);
  /**
   * @brief validated the received response is HCI credit notification.
   * @param rspLen Length of the hci response
   * @param rsp Pointer to the response data
   * @return returns true if HCI credit ntf, else false,
   */
  bool isCreditNtfForHci(uint8_t *rsp, uint16_t rspLen);
  /**
   * @brief validated the received response is HCI packet.
   * @param rspLen Length of the hci response
   * @param rsp Pointer to the response data
   * @return returns true if HCI rsp, else false,
   */
  bool isValidHciPacket(uint8_t *rsp, uint16_t rspLen);
  /**
   * @brief process the received HCI packet.
   * @param rspLen Length of the hci response
   * @param rsp Pointer to the response data
   * @return returns NFCSTATUS_SUCCESS,
   */
  NFCSTATUS processHciPacket(uint8_t *rsp, uint16_t rspLen);

  /**
   * @brief Releases all the resources
   * @return None
   *
   */
  static inline void finalize() {
    if (sAutoCard != nullptr) {
      sAutoCard.reset();
    }
  }

private:
  static std::unique_ptr<AutoCard>
      sAutoCard; /* Singleton instance of AutoCard */
  /* Auto card command type GET/SET*/
  uint8_t autoCardCmdType;
  bool isHciCmdSent;
  bool isPipeTobeCreated;
  bool mFirstFragment;
  bool mWTX;
  std::mutex mRspMutex;
  HciRspPkt mResponse;
  bool mWaitForResponse;
  bool mIsStrPhoneOffEnabled;
  unsigned long mMaxWtxCount;
  uint32_t mWtxCount;
  std::vector<uint8_t> mLastSentCmd;
  std::condition_variable mRspCond;
  std::chrono::milliseconds mResponseTimeout;
  /**
   * @brief maintains autocard enabled status.
   * Bit-0 1b for enable and 0b for Disable.
   */
  uint8_t mAutoCardEnableStatus;
  bool mIsPutAppletStatusBackEnabled;
  /* Autocard counters */
  std::vector<uint8_t> mAutoCardCounters;
  constexpr static uint8_t AUTOCARD_STATUS_SUCCESS = 0x00;
  constexpr static uint8_t CNT_CONFIG_BUFF_MAX_SIZE = 0x06;
  constexpr static uint8_t AUTOCARD_SUB_OID_IDEX = 0x04;
  constexpr static uint8_t AUTOCARD_FEATURE_SUB_GID = 0x50;
  constexpr static uint8_t AUTOCARD_FW_API_OID = 0x43;
  constexpr static uint8_t AUTOCARD_FEATURE_ENABLED = 0x01;
  constexpr static uint8_t AUTOCARD_SET_COUNTERS_SUB_OID = 0x01;
  constexpr static uint8_t AUTOCARD_GET_COUNTERS_SUB_OID = 0x02;
  constexpr static uint8_t AUTOCARD_SET_AID_SUB_OID = 0x03;
  constexpr static uint8_t AUTOCARD_GET_AID_SUB_OID = 0x04;
  constexpr static uint8_t AUTOCARD_SET_APPLET_STATUS_SUB_OID = 0x05;
  constexpr static uint8_t AUTOCARD_SUSPEND_SUB_OID = 0x06;
  constexpr static uint8_t AUTOCARD_SET_TIMER_SUB_OID = 0x07;
  constexpr static uint8_t AUTOCARD_GET_TIMER_SUB_OID = 0x08;
  constexpr static uint8_t AUTOCARD_SET_RF_PARAM = 0x0A;
  constexpr static uint8_t AUTOCARD_GET_RF_PARAM = 0x0B;
  constexpr static uint8_t STR_ACS_FEATURE_ENABLE_SUB_OID = 0x20;
  constexpr static uint8_t STR_SET_READER_PROFILE = 0x22;
  constexpr static uint8_t STR_SET_ACTIVATE_AID = 0x24;
  constexpr static uint8_t AUTOCARD_FEATURE_ENABLE_SUB_OID = 0x07;
  constexpr static uint8_t AUTOCARD_FEATURE_DISABLE_SUB_OID = 0x08;
  constexpr static uint8_t AUTOCARD_STATUS_INDEX = 0x04;
  constexpr static uint8_t AUTOCARD_PAYLOAD_LEN = 0x04;
  constexpr static uint8_t AUTOCARD_HEADER_LEN = 0x02;
  constexpr static uint8_t AUTOCARD_STATUS_CMD_FAIL = 0x01;
  constexpr static uint8_t AUTOCARD_STATUS_DISABLED = 0x0B;
  constexpr static uint8_t AUTOCARD_STATUS_NOT_CONFIGURED = 0x0C;
  constexpr static uint8_t AUTOCARD_STATUS_FEATURE_NOT_SUPPORTED = 0x0D;
  AutoCard();
  ~AutoCard();
  friend struct std::default_delete<AutoCard>;
};

#endif // NFC_AUTOCARD_H
