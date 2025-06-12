/******************************************************************************
 *
 *  Copyright 2025 NXP
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

#include <phNfcStatus.h>
#include <phNfcTypes.h>

/**
 * @brief Manager class to handle the AutoCard operations.
 */
class AutoCard {
 public:
  /**
   * @brief Get the singleton instance of AutoCard.
   * @return AutoCard* Pointer to the instance.
   */
  static AutoCard* getInstance();

  /**
   * @brief handle Autocard command.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS processAutoCardNciMsg(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief Process NCI response/notification for ntag.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS processAutoCardNciRspNtf(uint16_t dataLen, uint8_t* pData);

  void notifyAutocardRsp(uint8_t status, uint8_t cmdType);

  /**
   * @brief Releases all the resources
   * @return None
   *
   */
  static inline void finalize() {
    if (sAutoCard != nullptr) {
      delete (sAutoCard);
      sAutoCard = nullptr;
    }
  }

 private:
  static AutoCard* sAutoCard;
  /* Auto card command type GET/SET*/
  uint8_t autoCardCmdType;
  constexpr static uint8_t AUTOCARD_FEATURE_SUB_GID = 0x50;
  constexpr static uint8_t AUTOCARD_FW_API_OID = 0x43;
  constexpr static uint8_t AUTOCARD_SET_AID = 0x01;
  constexpr static uint8_t AUTOCARD_GET_AID = 0x02;
  constexpr static uint8_t AUTOCARD_STATUS_INDEX = 0x04;
  constexpr static uint8_t AUTOCARD_PAYLOAD_LEN = 0x04;
  constexpr static uint8_t AUTOCARD_HEADER_LEN = 0x02;
  constexpr static uint8_t AUTOCARD_STATUS_CMD_FAIL = 0x01;
  constexpr static uint8_t AUTOCARD_STATUS_DISABLED = 0x0B;
  constexpr static uint8_t AUTOCARD_STATUS_NOT_CONFIGURED = 0x0C;
  constexpr static uint8_t AUTOCARD_STATUS_FEATURE_NOT_SUPPORTED = 0x0D;
  AutoCard();
  ~AutoCard();
};

#endif  // NFC_AUTOCARD_H
