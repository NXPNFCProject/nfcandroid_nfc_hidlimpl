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

#define NxpAutoCardInstance (AutoCard::getInstance())

class AutoCard {
 public:
  static AutoCard& getInstance();
  NFCSTATUS handleNciMessage(uint16_t dataLen, uint8_t* pData);
  void notifyAutocardRsp(uint8_t status, uint8_t cmdType);
  constexpr static uint8_t AUTOCARD_FEATURE_SUB_OID = 0x50;

 private:
  constexpr static uint8_t AUTOCARD_FW_API_OID = 0x43;
  constexpr static uint8_t AUTOCARD_SET_SUB_GID = 0x01;
  constexpr static uint8_t AUTOCARD_GET_SUB_GID = 0x02;
  constexpr static uint8_t AUTOCARD_STATUS_INDEX = 0x04;
  constexpr static uint8_t AUTOCARD_PAYLOAD_LEN = 0x04;
  constexpr static uint8_t AUTOCARD_HEADER_LEN = 0x02;
  constexpr static uint8_t AUTOCARD_CMD_FAIL = 0x01;
  constexpr static uint8_t AUTOCARD_DISABLED = 0x0B;
  constexpr static uint8_t AUTOCARD_NOT_CONFIGURED = 0x0C;
  constexpr static uint8_t AUTOCARD_FEATURE_NOT_SUPPORTED = 0x0D;
  AutoCard();
  ~AutoCard();
};

#endif  // NFC_AUTOCARD_H
