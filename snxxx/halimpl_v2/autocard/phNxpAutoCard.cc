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

#include "phNxpAutoCard.h"
#include <phNfcNciConstants.h>
#include <phNxpLog.h>
#include <phNxpNciHal.h>
#include <phNxpNciHal_ext.h>
#include "NfcExtension.h"

extern phNxpNciHal_Control_t nxpncihal_ctrl;
AutoCard::AutoCard() {}

AutoCard::~AutoCard() {}

AutoCard& AutoCard::getInstance() {
  static AutoCard msNxpAutoCard;
  return msNxpAutoCard;
}

void AutoCard::notifyAutocardRsp(uint8_t status, uint8_t cmdType) {
  vector<uint8_t> autocardRsp = {(NCI_MT_RSP | NCI_GID_PROP),
                                 NCI_ROW_MAINLINE_OID, AUTOCARD_PAYLOAD_LEN,
                                 AUTOCARD_FEATURE_SUB_OID};
  NXPLOG_NCIHAL_D("AutoCard::%s Enter", __func__);

  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("AutoCard::%s failed status: %d ", __func__, status);

    autocardRsp.push_back(AUTOCARD_HEADER_LEN);
    autocardRsp.push_back(cmdType);
    autocardRsp.push_back(status);
  } else {
    autocardRsp[NCI_MSG_LEN_INDEX] =
        nxpncihal_ctrl.p_rx_data[NCI_MSG_LEN_INDEX] + AUTOCARD_HEADER_LEN;
    autocardRsp.insert(autocardRsp.end(),
                       &nxpncihal_ctrl.p_rx_data[NCI_MSG_LEN_INDEX],
                       (&nxpncihal_ctrl.p_rx_data[NCI_MSG_LEN_INDEX] +
                        (nxpncihal_ctrl.rx_data_len - AUTOCARD_HEADER_LEN)));
  }
  phNxpHal_NfcDataCallback(autocardRsp.size(), &autocardRsp[0]);
}

NFCSTATUS AutoCard::handleNciMessage(uint16_t dataLen, uint8_t* pData) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  uint8_t autocard_selection_mode = 0x00;
  uint8_t autocardStatus = NFCSTATUS_FAILED;

  NXPLOG_NCIHAL_D("AutoCard::%s Enter ", __func__);

  if ((pData[4] == AUTOCARD_SET_SUB_GID) ||
      (pData[4] == AUTOCARD_GET_SUB_GID)) {
    if (!IS_CHIP_TYPE_GE(sn220u)) {
      autocardStatus = AUTOCARD_FEATURE_NOT_SUPPORTED;
      NXPLOG_NCIHAL_E("AutoCard selection is not supported.");
    } else if (!GetNxpNumValue(NAME_AUTOCARD_SELECTION_PHONE_OFF,
                               &autocard_selection_mode,
                               sizeof(autocard_selection_mode))) {
      autocardStatus = AUTOCARD_NOT_CONFIGURED;
      NXPLOG_NCIHAL_E("AutoCard selection is not configured.");
    } else if (autocard_selection_mode != 0x01) {
      autocardStatus = AUTOCARD_DISABLED;
      NXPLOG_NCIHAL_E("AutoCard selection is Disabled.");
    } else {
      std::vector<uint8_t> autocardCmd(pData, pData + dataLen);
      autocardCmd[NCI_OID_INDEX] = AUTOCARD_FW_API_OID;
      autocardCmd[NCI_MSG_LEN_INDEX]--;
      autocardCmd.erase(autocardCmd.begin() + NCI_MSG_INDEX_FOR_FEATURE);
      status = phNxpNciHal_send_ext_cmd(autocardCmd.size(), &autocardCmd[0]);
      if (status == NFCSTATUS_SUCCESS) {
        if (nxpncihal_ctrl.rx_data_len <= AUTOCARD_STATUS_INDEX) {
          autocardStatus = nxpncihal_ctrl.p_rx_data[3];
        } else if ((nxpncihal_ctrl.rx_data_len > AUTOCARD_STATUS_INDEX) &&
            (nxpncihal_ctrl.p_rx_data[AUTOCARD_STATUS_INDEX] !=
             NFCSTATUS_SUCCESS)) {
          autocardStatus = nxpncihal_ctrl.p_rx_data[AUTOCARD_STATUS_INDEX];
          NXPLOG_NCIHAL_E("%s Set autocard failed. Error: %d", __func__,
                          autocardStatus);
        } else {
          autocardStatus = NFCSTATUS_SUCCESS;
        }
      } else {
        autocardStatus = AUTOCARD_CMD_FAIL;
      }
    }
    notifyAutocardRsp(autocardStatus, pData[4]);
  }

  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}
