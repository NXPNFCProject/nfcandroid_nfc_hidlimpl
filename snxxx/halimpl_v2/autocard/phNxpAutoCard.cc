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
#include <phNxpNciHal_ext.h>
#include "NfcExtension.h"

AutoCard* AutoCard::sAutoCard = nullptr;

AutoCard::AutoCard() {
  NXPLOG_NCIHAL_D("AutoCard::%s Enter ", __func__);
  autoCardCmdType = 0;
}

AutoCard::~AutoCard() { NXPLOG_NCIHAL_D("AutoCard::%s Enter ", __func__); }

AutoCard* AutoCard::getInstance() {
  if (sAutoCard == nullptr) {
    sAutoCard = new AutoCard();
  }
  return sAutoCard;
}

NFCSTATUS AutoCard::handleVendorNciRspNtf(uint16_t dataLen, uint8_t* pData) {
  NXPLOG_NCIHAL_D("AutoCard::%s Enter ", __func__);

  if ((pData[NCI_GID_INDEX] != (NCI_MT_RSP | NCI_GID_PROP)) ||
      (pData[NCI_OID_INDEX] != AUTOCARD_FW_API_OID) ||
      ((dataLen > AUTOCARD_STATUS_INDEX) && (pData[3] != AUTOCARD_SET_AID) &&
       (pData[3] != AUTOCARD_GET_AID))) {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  uint8_t status = (dataLen > AUTOCARD_STATUS_INDEX)
                       ? pData[AUTOCARD_STATUS_INDEX]
                       : pData[3];
  vector<uint8_t> autocardRsp = {(NCI_MT_RSP | NCI_GID_PROP),
                                 NCI_ROW_MAINLINE_OID, AUTOCARD_PAYLOAD_LEN,
                                 AUTOCARD_FEATURE_SUB_GID};

  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("%s Set autocard failed. Error: %d", __func__, status);
    autocardRsp.push_back(AUTOCARD_HEADER_LEN);
    autocardRsp.push_back(AutoCard::getInstance()->autoCardCmdType);
    autocardRsp.push_back(status);
  } else {
    autocardRsp[NCI_MSG_LEN_INDEX] =
        pData[NCI_MSG_LEN_INDEX] + AUTOCARD_HEADER_LEN;
    autocardRsp.insert(autocardRsp.end(), pData + NCI_MSG_LEN_INDEX,
                       pData + dataLen);
  }
  phNxpHal_NfcDataCallback(autocardRsp.size(), &autocardRsp[0]);
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

NFCSTATUS AutoCard::handleVendorNciMessage(uint16_t dataLen, uint8_t* pData) {
  NXPLOG_NCIHAL_D("AutoCard::%s Enter", __func__);

  if ((pData[NCI_GID_INDEX] != (NCI_MT_CMD | NCI_GID_PROP)) ||
      (pData[NCI_OID_INDEX] != NCI_ROW_PROP_OID_VAL) ||
      (pData[NCI_MSG_INDEX_FOR_FEATURE] !=
       AutoCard::getInstance()->AUTOCARD_FEATURE_SUB_GID) ||
      ((pData[4] != AUTOCARD_SET_AID) && (pData[4] != AUTOCARD_GET_AID))) {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  uint8_t autocard_selection_mode = 0x00;
  uint8_t autocardStatus = NFCSTATUS_SUCCESS;

  if (!IS_CHIP_TYPE_GE(sn220u)) {
    autocardStatus = AUTOCARD_STATUS_FEATURE_NOT_SUPPORTED;
    NXPLOG_NCIHAL_E("AutoCard selection is not supported.");
  } else if (!GetNxpNumValue(NAME_NXP_AUTOCARD_SELECTION_PHONE_OFF,
                             &autocard_selection_mode,
                             sizeof(autocard_selection_mode))) {
    autocardStatus = AUTOCARD_STATUS_NOT_CONFIGURED;
    NXPLOG_NCIHAL_E("AutoCard selection is not configured.");
  } else if (autocard_selection_mode != 0x01) {
    autocardStatus = AUTOCARD_STATUS_DISABLED;
    NXPLOG_NCIHAL_E("AutoCard selection is Disabled.");
  } else {
    std::vector<uint8_t> autocardCmd(pData, pData + dataLen);
    autocardCmd[NCI_OID_INDEX] = AUTOCARD_FW_API_OID;
    autocardCmd[NCI_MSG_LEN_INDEX]--;
    autocardCmd.erase(autocardCmd.begin() + NCI_MSG_INDEX_FOR_FEATURE);

    NFCSTATUS status =
        phNxpHal_EnqueueWrite(autocardCmd.data(), autocardCmd.size());
    if (status != NFCSTATUS_SUCCESS) {
      autocardStatus = AUTOCARD_STATUS_CMD_FAIL;
      NXPLOG_NCIHAL_D("AutoCard::%s failed status: %d", __func__, status);
    }
    AutoCard::getInstance()->autoCardCmdType = pData[4];
  }

  if (autocardStatus != NFCSTATUS_SUCCESS) {
    std::vector<uint8_t> autocardRsp = {
        (NCI_MT_RSP | NCI_GID_PROP),
        NCI_ROW_MAINLINE_OID,
        AUTOCARD_PAYLOAD_LEN,
        AUTOCARD_FEATURE_SUB_GID,
        AUTOCARD_HEADER_LEN,
        AutoCard::getInstance()->autoCardCmdType,
        autocardStatus};
    phNxpHal_NfcDataCallback(autocardRsp.size(), autocardRsp.data());
  }

  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}
