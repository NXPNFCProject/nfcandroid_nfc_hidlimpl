/******************************************************************************
 *
 *  Copyright 2024-2025 NXP
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

#include <stdint.h>
#include <stdlib.h>
#include <memory>

// Opaque WiredSe Service object.
struct WiredSeService;

enum WiredSeEvtTypeEnum: uint8_t {
  NFC_STATE_CHANGE,
  NFC_PKT_RECEIVED,
  SENDING_HCI_PKT,
  DISABLING_NFCEE,
  NFC_EVT_UNKNOWN
};

enum NfcStateEnum: uint8_t { NFC_ON, NFC_OFF, NFC_STATE_UNKNOWN };

using WiredSeEvtType = WiredSeEvtTypeEnum;
using NfcState = NfcStateEnum;

using WiredSeEvtType = WiredSeEvtTypeEnum;
using NfcState = NfcStateEnum;

typedef struct NfcPkt {
  uint8_t* data;
  uint16_t len;

  NfcPkt() : data(nullptr), len(0) {}

  NfcPkt(uint8_t* inData, uint16_t inLen) : data(nullptr), len(0) {
    if (inData == nullptr || inLen == 0) {
      return;
    }
    data = static_cast<uint8_t*>(calloc(1, inLen));
    if (data != nullptr) {
      memcpy(data, inData, inLen);
      len = inLen;
    }
  }

  NfcPkt(const NfcPkt& other) = delete;
  NfcPkt& operator=(const NfcPkt& other) = delete;

  ~NfcPkt() {
    if (data != nullptr) {
      free(data);
    }
    data = nullptr;
  }

} NfcPkt;

typedef union WiredSeEvtData {
  NfcState nfcState;
  std::shared_ptr<NfcPkt> nfcPkt;

  WiredSeEvtData() : nfcPkt(nullptr) {}
  WiredSeEvtData(NfcState inNfcState) : nfcState(inNfcState) {}
  WiredSeEvtData(std::shared_ptr<NfcPkt> inNfcPkt) : nfcPkt(std::move(inNfcPkt)) {}

  WiredSeEvtData(const WiredSeEvtData& evtData) = delete;

  WiredSeEvtData& operator=(const WiredSeEvtData& evtData) {
    if (this == &evtData) {
      return *this;
    }
    nfcState = evtData.nfcState;
    nfcPkt = evtData.nfcPkt;
    return *this;
  }

  ~WiredSeEvtData() { nfcPkt = nullptr; }
} WiredSeEvtData;

typedef struct WiredSeEvt {
  WiredSeEvtType event;
  WiredSeEvtData eventData;

  WiredSeEvt() { event = NFC_EVT_UNKNOWN; }
  WiredSeEvt(const WiredSeEvt& evt) {
    event = evt.event;
    eventData = evt.eventData;
  }
  ~WiredSeEvt() {}
} WiredSeEvt;

extern "C" int32_t WiredSeService_Start(WiredSeService** wiredSeService);
extern "C" int32_t WiredSeService_DispatchEvent(WiredSeService* wiredSeService,
                                                WiredSeEvt evt);
