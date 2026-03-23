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
#include "WlcStateMonitor.h"
#include <cstring>
#include <phNxpLog.h>
#include <stdlib.h>

using std::vector;

std::unique_ptr<WlcStateMonitor> WlcStateMonitor::instance = nullptr;

WlcStateMonitor::WlcStateMonitor() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mIsAutonomousModeEn = false;
}

WlcStateMonitor::~WlcStateMonitor() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mIsAutonomousModeEn = false;
}

WlcStateMonitor *WlcStateMonitor::getInstance() {
  if (!instance) {
    instance = std::unique_ptr<WlcStateMonitor>(new WlcStateMonitor());
  }
  return instance.get();
}

WLCSTATUS WlcStateMonitor::processDiscMapCmd(uint16_t dataLen,
                                         const uint8_t *pData) {

  uint8_t wlcDiscMspCmd[] = {0x21, 0x00, 0x04, 0x01, 0x02, 0x01, 0x01};
  uint8_t wlcAutonomousEn = 0;

  if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(NAME_NXP_WLC_AUTONOMOUS_CHARGING_ENABLED,
                      &wlcAutonomousEn,
                      sizeof(wlcAutonomousEn)) || (wlcAutonomousEn > 1) || (wlcAutonomousEn == 0)) {
    wlcDiscMspCmd[4] = 0x02;
    wlcDiscMspCmd[6] = 0x01;
    mIsAutonomousModeEn = false;
  } else {
    wlcDiscMspCmd[4] = 0x08;
    wlcDiscMspCmd[6] = 0x07;
    mIsAutonomousModeEn = true;
  }

  WlcExtensionWriter::getInstance()->write(wlcDiscMspCmd, sizeof(wlcDiscMspCmd));
  return WLCSTATUS_EXTN_FEATURE_SUCCESS;
}

WLCSTATUS WlcStateMonitor::processRfDiscCmd(uint16_t dataLen,
                                         const uint8_t *pData) {

  uint8_t wlcRfDiscCmd[] = {0x21, 0x03, 0x09, 0x04,
    0x00, 0x01, // A Type
    0x01, 0x01, // B Type
    0x02, 0x01, // F Type
    0x06, 0x01  // V Type
    };
  WlcExtensionWriter::getInstance()->write(wlcRfDiscCmd, sizeof(wlcRfDiscCmd));
  return WLCSTATUS_EXTN_FEATURE_SUCCESS;
}

WLCSTATUS WlcStateMonitor::handleWlcNciRspNtf(uint16_t dataLen, const uint8_t *pData){

  WLCSTATUS status = WLCSTATUS_EXTN_FEATURE_FAILURE;
  vector<uint8_t> nciRspNtf(pData, pData + dataLen);
  const uint16_t mGidOid = ((nciRspNtf[0] << 8) | nciRspNtf[1]);
  vector<uint8_t> wlcNtf = {NCI_PROP_NTF_VAL, NCI_WLC_PROP_OID_VAL};

  if(!mIsAutonomousModeEn){
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Non-Autonomous Mode configured", __func__);
    return status;
  }

  wlcNtf.push_back(static_cast<uint8_t>( dataLen + 2)); // +2 byte for SUB GID & OID
  wlcNtf.push_back(static_cast<uint8_t>(NCI_WLC_PROP_SUB_GID_VAL));

  switch (mGidOid) {
    case NCI_RF_DEACTD_NTF_GID_OID:
      if (mIsWlcRfActivated) {
        mIsWlcRfActivated = false;
        status = WLCSTATUS_EXTN_FEATURE_SUCCESS;
        wlcNtf.push_back(static_cast<uint8_t>(NCI_WLC_RF_INTF_DEACT_NTF_SUB_OID));
          wlcNtf.insert(wlcNtf.end(), pData, pData + dataLen);
          PlatformAbstractionLayer::getInstance()->palSendWlcDataCallback(
                    wlcNtf.size(), wlcNtf.data());
      } else {
        NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Ignored RF DEACT NTF", __func__);
      }
      break;
    case NCI_CORE_INTERFACE_ERROR_NTF_GID_OID:
      [[fallthrough]];
    case NCI_RF_INTF_ACTD_NTF_GID_OID:
        if (nciRspNtf[NCI_RF_INTF_VAL_INDEX] == NCI_RF_INTF_WLC_AUTON_VAL) {
          mIsWlcRfActivated = true;
          status = WLCSTATUS_EXTN_FEATURE_SUCCESS;
          wlcNtf.push_back(static_cast<uint8_t>(NCI_WLC_RF_INTF_ACT_NTF_SUB_OID));
          wlcNtf.insert(wlcNtf.end(), pData, pData + dataLen);
          PlatformAbstractionLayer::getInstance()->palSendWlcDataCallback(
                    wlcNtf.size(), wlcNtf.data());
          break;
        } else {
          NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Ignored RF ACT NTF", __func__);
        }
    case NCI_PROP_WLC_STATUS_NTF_GID_OID:
    case NCI_ROW_WLC_STATUS_NTF_GID_OID: {
        status = WLCSTATUS_EXTN_FEATURE_SUCCESS;
        wlcNtf.push_back(static_cast<uint8_t>(NCI_WLC_STATUS_NTF_SUB_OID));
        wlcNtf.insert(wlcNtf.end(), pData, pData + dataLen);
        PlatformAbstractionLayer::getInstance()->palSendWlcDataCallback(
                  wlcNtf.size(), wlcNtf.data());
      break;
    }
    case NCI_DATA_PKT_GID_OID:{
        status = WLCSTATUS_EXTN_FEATURE_SUCCESS;
        wlcNtf.push_back(static_cast<uint8_t>(NCI_WLC_OBS_DATA_PKT_CMD_SUB_OID));
        wlcNtf.insert(wlcNtf.end(), pData, pData + dataLen);
        PlatformAbstractionLayer::getInstance()->palSendWlcDataCallback(
                  wlcNtf.size(), wlcNtf.data());
      break;
    }
    default: {
      return status;
    }
  }
  return status;
}
