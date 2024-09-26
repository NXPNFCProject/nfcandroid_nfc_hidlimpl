/******************************************************************************
 *
 *  Copyright 2024 NXP
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

// Opaque WiredSe Service object.
struct WiredSeService;

typedef enum WiredSeEvtType {
  NFC_STATE_CHANGE,
  NFC_PKT_RECEIVED,
  SENDING_HCI_PKT,
  DISABLING_NFCEE,
  NFC_EVT_UNKNOWN
} WiredSeEvtType;

typedef enum { NFC_ON, NFC_OFF, NFC_STATE_UNKNOWN } NfcState;

typedef struct NfcPkt {
  uint8_t* data;
  uint16_t len;
  NfcPkt() {
    data = NULL;
    len = 0;
  }
  // Constructor
  NfcPkt(uint8_t* inData, uint16_t inLen) {
    data = inData;
    len = inLen;
  }
} NfcPkt;

typedef union WiredSeEvtData {
  NfcState nfcState;
  NfcPkt nfcPkt;
  // Default
  WiredSeEvtData() {}
  // For typecasting from NfcState to WiredSeEvtData
  WiredSeEvtData(NfcState inNfcState) { nfcState = inNfcState; }
  // For typecasting from NfcPkt to WiredSeEvtData
  WiredSeEvtData(NfcPkt inNfcPkt) { nfcPkt = inNfcPkt; }

} WiredSeEvtData;

typedef struct WiredSeEvt {
  WiredSeEvtType event;
  WiredSeEvtData eventData;

  WiredSeEvt() { event = NFC_EVT_UNKNOWN; }
} WiredSeEvt;

extern "C" int32_t WiredSeService_Start(WiredSeService** wiredSeService);
extern "C" int32_t WiredSeService_DispatchEvent(WiredSeService* wiredSeService,
                                                WiredSeEvt evt);
