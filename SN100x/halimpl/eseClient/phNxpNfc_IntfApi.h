/*
 * Copyright (C) 2012-2018 NXP Semiconductors
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
#include <phNfcStatus.h>
#include <cutils/log.h>
#include "nfa_api.h"
#include "NfcAdaptation.h"
#include "nfa_ee_api.h"
#include "nfa_hci_api.h"
#include <phNxpLog.h>


#include "phNxpNciHal_Adaptation.h"
#include <phNxpNciHal_utils.h>

#ifndef _PHNXPNFC_INTF_API_H_
#define _PHNXPNFC_INTF_API_H_

struct tHAL_NFC_CB {
    uint8_t state;
};
extern bool nfc_debug_enabled;
NFCSTATUS phNxpNfc_InitLib();
NFCSTATUS phNxpNfc_DeInitLib();
NFCSTATUS phNxpNfc_ResetEseJcopUpdate();
NFCSTATUS phNxpNfc_openEse();
NFCSTATUS phNxpNfc_closeEse();
bool phNxpNfc_EseTransceive(uint8_t* xmitBuffer, int32_t xmitBufferSize, uint8_t* recvBuffer,
                     int32_t recvBufferMaxSize, int32_t& recvBufferActualSize, int32_t timeoutMillisec);
#endif
