/**
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
 **/
#include "NfceeStateMonitor.h"
#include <cstring>
#include <phNxpLog.h>
#include <stdlib.h>

using std::vector;

std::unique_ptr<NfceeStateMonitor> NfceeStateMonitor::instance = nullptr;

NfceeStateMonitor::NfceeStateMonitor() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  currentEE = NCI_ROUTE_HOST;
  eseRecoveryCount = 0;
}

NfceeStateMonitor::~NfceeStateMonitor() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
}

NfceeStateMonitor *NfceeStateMonitor::getInstance() {
  if (!instance) {
    instance = std::unique_ptr<NfceeStateMonitor>(new NfceeStateMonitor());
  }
  return instance.get();
}

void NfceeStateMonitor::setCurrentEE(uint8_t ee) { currentEE = ee; }

bool NfceeStateMonitor::isEeRecoveryRequired(uint8_t eeErrorCode) {
  switch (eeErrorCode) {
  case RESPONSE_STATUS_FAILED: {
    if (currentEE == NCI_ROUTE_ESE_ID)
      return true;
    return false;
  }
  case NCI_NFCEE_TRANSMISSION_ERROR: {
    if ((PlatformAbstractionLayer::getInstance()->palGetChipType() == pn557) &&
        (currentEE == NCI_ROUTE_UICC1_ID || currentEE == NCI_ROUTE_UICC2_ID))
      return false;
    return true;
  }
  default: {
    return false;
  }
  }
}

NFCSTATUS
NfceeStateMonitor::processNfceeModeSetNtf(vector<uint8_t> &nfceeModeSetNtf) {
  NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN,
                 "NfceeStateMonitor %s Enter, eseRecoveryCount: %d", __func__,
                 eseRecoveryCount);
  const uint8_t nfceeModeSetNtf_len = nfceeModeSetNtf.size();
  if (nfceeModeSetNtf_len != 0x04) {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
      "NfceeStateMonitor %s Nfcee message invalid length", __func__);
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  const uint8_t errorCode = nfceeModeSetNtf[NCI_MODE_SET_NTF_REASON_CODE_INDEX];
  if (isEeRecoveryRequired(errorCode)) {
    if (eseRecoveryCount >= MAX_ESE_RECOVERY_COUNT) {
      nfceeModeSetNtf[nfceeModeSetNtf_len - 1] = RESPONSE_STATUS_FAILED;
      NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN,
                     "NfceeStateMonitor %s Max recovery count reached for 0x%x",
                     __func__, currentEE);
      return NFCSTATUS_EXTN_FEATURE_FAILURE;
    }

    vector<uint8_t> eeUnrecoverableNtf = {0x62, 0x02, 0x02};
    eseRecoveryCount++;
    nfceeModeSetNtf[nfceeModeSetNtf_len - 1] = RESPONSE_STATUS_OK;
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "NfceeStateMonitor %s Sending UNRECOVERABLE ERROR for 0x%x",
                   __func__, currentEE);
    eeUnrecoverableNtf.push_back(currentEE);
    eeUnrecoverableNtf.push_back(NCI_NFCEE_STS_UNRECOVERABLE_ERROR);
    PlatformAbstractionLayer::getInstance()->palenQueueRspNtf(
        eeUnrecoverableNtf.data(), eeUnrecoverableNtf.size());
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }
  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}
