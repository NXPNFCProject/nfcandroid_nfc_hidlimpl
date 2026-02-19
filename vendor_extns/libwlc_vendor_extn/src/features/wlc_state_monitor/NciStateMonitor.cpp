/**
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
 **/

#include "NciStateMonitor.h"
#include "WlcStateMonitor.h"
#include "WlcExtensionConstants.h"
#include "phNxpConfig.h"
#include <phNxpLog.h>
#include <stdio.h>

using std::vector;
std::unique_ptr<NciStateMonitor> NciStateMonitor::instance = nullptr;

NciStateMonitor::NciStateMonitor() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
}

NciStateMonitor::~NciStateMonitor() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  WlcStateMonitor::finalize();
}

NciStateMonitor *NciStateMonitor::getInstance() {
  if (!instance) {
    instance = std::unique_ptr<NciStateMonitor>(new NciStateMonitor());
  }
  return instance.get();
}

vector<uint8_t> NciStateMonitor::getFwVersion() { return firmwareVersion; }

WLCSTATUS NciStateMonitor::handleVendorNciMessage(uint16_t dataLen,
                                               const uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "NciStateMonitor %s Enter dataLen:%d",
                 __func__, dataLen);
  return WLCSTATUS_EXTN_FEATURE_FAILURE;
}

WLCSTATUS
NciStateMonitor::processCoreResetNtfReceived(vector<uint8_t> coreResetNtf) {
  constexpr uint16_t NCI_MIN_CORE_RESET_NTF_LEN = 0x0C;
  if (coreResetNtf.size() >= NCI_MIN_CORE_RESET_NTF_LEN) {
    firmwareVersion.clear();
    firmwareVersion.assign(coreResetNtf.end() - 3, coreResetNtf.end());
  } else {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Invalid Ntf pkt length %zu",
                   __func__, coreResetNtf.size());
  }
  return WLCSTATUS_EXTN_FEATURE_FAILURE;
}

WLCSTATUS NciStateMonitor::handleVendorNciRspNtf(uint16_t dataLen,
                                              uint8_t *pData) {

  WLCSTATUS status = WlcStateMonitor::getInstance()->handleWlcNciRspNtf(dataLen, pData);
  if (status == WLCSTATUS_EXTN_FEATURE_SUCCESS) {
    return status;
  }

  vector<uint8_t> nciRspNtf(pData, pData + dataLen);
  const uint16_t mGidOid = ((nciRspNtf[0] << 8) | nciRspNtf[1]);

  switch (mGidOid) {
    case NCI_CORE_RESET_NTF_GID_OID: {
      status = processCoreResetNtfReceived(nciRspNtf);
      break;
    }
    default: {
      return status;
    }
  }

  return status;
}

WLCSTATUS NciStateMonitor::processNciCmd(uint16_t dataLen,
                                         const uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "NciStateMonitor %s Enter", __func__);

  uint16_t mGidOid = ((pData[0] << 8) | pData[1]);
  if (mGidOid == NCI_RF_DISC_MAP_CMD_GID_OID) {
    return WlcStateMonitor::getInstance()->processDiscMapCmd(dataLen, pData);
  } else if (mGidOid == NCI_RF_DISC_CMD_GID_OID) {
    return WlcStateMonitor::getInstance()->processRfDiscCmd(dataLen, pData);
  }
  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

WLCSTATUS NciStateMonitor::handleHalEvent(uint8_t event) {
  NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN, "NciStateMonitor %s Enter",
                 __func__);
return WLCSTATUS_EXTN_FEATURE_FAILURE;
}
