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
#pragma once

#include "WiredSeService.h"
#include "phNfcStatus.h"

#define NFCEE_ESE_ID 0xC0
#define NFCEE_EUICC_ID 0xC1
#define NFCEE_EUICC2_ID 0xC2
#define NFCEE_INVALID_ID 0x00

typedef int32_t (*WiredSeStartFunc_t)(WiredSeService** pWiredSeService);
typedef int32_t (*WiredSeDispatchEventFunc_t)(WiredSeService* pWiredSeService,
                                              WiredSeEvt event);

/**
 * Handle to the Power Tracker stack implementation.
 */
typedef struct {
  // Function to start wired-se.
  WiredSeStartFunc_t start;
  // Function to dispatch events to wired-se subsystem.
  WiredSeDispatchEventFunc_t dispatchEvent;
  // WiredSeService instance
  WiredSeService* pWiredSeService;
  // WiredSe.so dynamic library handle.
  void* dlHandle;
} WiredSeHandle;

/*******************************************************************************
**
** Function         createWiredSeEvtData()
**
** Description      Helper function to create wired se event data from
**                  Nfc state.
**
** Returns          WiredSeEvtData object.
*******************************************************************************/
static inline WiredSeEvtData createWiredSeEvtData(NfcState nfcState) {
  return (WiredSeEvtData)nfcState;
}

/*******************************************************************************
**
** Function         createWiredSeEvtData()
**
** Description      Helper function to create wired se event data from
**                  Nfc packet.
**
** Returns          WiredSeEvtData object.
*******************************************************************************/
static inline WiredSeEvtData createWiredSeEvtData(uint8_t* nciPkt,
                                                  uint16_t len) {
  return (WiredSeEvtData)std::make_shared<NfcPkt>(nciPkt, len);
}

/*******************************************************************************
**
** Function         phNxpNciHal_WiredSeStart()
**
** Description      Starts wired-se HAL. This is the first Api to be invoked.
**                  Once it is started it will run throughout the process
**                  lifecycle.
**                  It is recommended to call from main() of service.
**
** Returns          Pointer to WiredSeHandle if WiredSe HAL is started.
**                  NULL otherwise
*******************************************************************************/
WiredSeHandle* phNxpNciHal_WiredSeStart();

/*******************************************************************************
**
** Function         phNxpNciHal_WiredSeDispatchEvent()
**
** Description      Dispatch events to wired-se subsystem.
**
** Parameters       inHandle - WiredSe Handle
** Returns          NFCSTATUS_SUCCESS if success.
**                  NFCSTATUS_FAILURE otherwise
*******************************************************************************/
NFCSTATUS phNxpNciHal_WiredSeDispatchEvent(
    WiredSeHandle* inHandle, WiredSeEvtType evtType,
    WiredSeEvtData evtData = WiredSeEvtData());
