/******************************************************************************
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
 ******************************************************************************/

#include "phNxpAutoCard.h"

AutoCard *AutoCard::sAutoCard = nullptr;

AutoCard::AutoCard() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "AutoCard::%s Enter ", __func__);
  autoCardCmdType = 0;
  mAutoCardEnableStatus = 0;
  mAutoCardCounters.clear();
}

AutoCard::~AutoCard() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "AutoCard::%s Enter ", __func__);
}

AutoCard *AutoCard::getInstance() {
  if (sAutoCard == nullptr) {
    sAutoCard = new AutoCard();
  }
  return sAutoCard;
}

void AutoCard::phNxpNciHal_getAutoCardConfig() {
  constexpr uint8_t AUTOCARD_FEATURE_CONFIG_GET_INDEX = 0x05;
  constexpr uint8_t AUTOCARD_FEATURE_CONFIG_SET_INDEX = 0x04;
  constexpr uint8_t AUTOCARD_GET_CNT_RSP_LEN = 12;
  constexpr uint8_t AUTOCARD_GET_TIMER_RSP_LEN = 6;
  constexpr uint8_t COUNTER_START_INDEX = 6;
  constexpr uint8_t NO_OF_CNT_TO_UPDATE = 6;
  constexpr uint8_t AUTOCARD_TIMER_GET_INDEX = 0x05;
  constexpr uint8_t AUTOCARD_TIMER_GET_STATUS_INDEX = 0x04;

  uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
  uint16_t rsp_len = 0;

  if ((PlatformAbstractionLayer::getInstance()->palGetChipType() != sn220u) &&
      (PlatformAbstractionLayer::getInstance()->palGetChipType() != sn300u))
    return;

  // if (IS_CHIP_TYPE_NE(sn220u) && IS_CHIP_TYPE_NE(sn300u)) return;

  uint8_t autocard_selection_mode = 0x00;
  if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
          NAME_NXP_AUTOCARD_SELECTION_PHONE_OFF, &autocard_selection_mode,
          sizeof(autocard_selection_mode))) {
    return;
  }

  if (autocard_selection_mode != AUTOCARD_FEATURE_ENABLED)
    return;

  std::vector<uint8_t> getAutoCardCounters = {0x2F, 0x43, 0x01, 0x02};
  NFCSTATUS status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
      getAutoCardCounters.size(), getAutoCardCounters.data(), &rsp_len, rsp);

  bool validResponse =
      (status == NFCSTATUS_SUCCESS) && (rsp_len == AUTOCARD_GET_CNT_RSP_LEN) &&
      (rsp[NCI_MSG_INDEX_FOR_FEATURE] == AUTOCARD_GET_COUNTERS_SUB_OID);

  if (!validResponse)
    return;

  mAutoCardEnableStatus = rsp[AUTOCARD_FEATURE_CONFIG_GET_INDEX];

  mAutoCardCounters.assign(rsp + COUNTER_START_INDEX,
                           rsp + COUNTER_START_INDEX + NO_OF_CNT_TO_UPDATE);

  uint8_t buffer[CNT_CONFIG_BUFF_MAX_SIZE] = {0};
  const long bufflen = CNT_CONFIG_BUFF_MAX_SIZE;
  long retlen = 0;

  const int isFound =
      PlatformAbstractionLayer::getInstance()->palGetNxpByteArrayValue(
          NAME_NXP_AUTOCARD_COUNTERS, reinterpret_cast<char *>(buffer), bufflen,
          &retlen);
  if (isFound > 0 && retlen == NO_OF_CNT_TO_UPDATE) {
    std::vector<uint8_t> readConfCnt(buffer, buffer + NO_OF_CNT_TO_UPDATE);

    if (readConfCnt != mAutoCardCounters) {
      std::vector<uint8_t> setAutoCardCounters = {0x2F, 0x43, 0x08, 0x01, 0x00};
      setAutoCardCounters.insert(setAutoCardCounters.end(), readConfCnt.begin(),
                                 readConfCnt.end());
      setAutoCardCounters[AUTOCARD_FEATURE_CONFIG_SET_INDEX] =
          mAutoCardEnableStatus;

      status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
          setAutoCardCounters.size(), setAutoCardCounters.data(), &rsp_len,
          rsp);
      if (status == NFCSTATUS_SUCCESS)
        mAutoCardCounters = readConfCnt;
    }
  }

  uint8_t autocard_timer_val = 0x00;
  if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
          NAME_NXP_AUTOCARD_TIMER_VALUE, &autocard_timer_val,
          sizeof(autocard_timer_val)) ||
      !autocard_timer_val) {
    return;
  }
  std::vector<uint8_t> getTimerValue = {0x2F, 0x43, 0x01, 0x08};
  status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
      getTimerValue.size(), getTimerValue.data(), &rsp_len, rsp);
  validResponse =
      (status == NFCSTATUS_SUCCESS) &&
      (rsp_len == AUTOCARD_GET_TIMER_RSP_LEN) &&
      (rsp[NCI_MSG_INDEX_FOR_FEATURE] == AUTOCARD_GET_TIMER_SUB_OID) &&
      (rsp[AUTOCARD_TIMER_GET_STATUS_INDEX] == AUTOCARD_STATUS_SUCCESS);

  if (!validResponse)
    return;
  if (rsp[AUTOCARD_TIMER_GET_INDEX] == autocard_timer_val)
    return;
  std::vector<uint8_t> setTimerValue = {0x2F, 0x43, 0x02, 0x07,
                                        autocard_timer_val};
  status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
      setTimerValue.size(), setTimerValue.data(), &rsp_len, rsp);
  if (status != NFCSTATUS_SUCCESS)
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s Set autocard timer value failed. Error: %d", __func__,
                   status);
}

NFCSTATUS AutoCard::handleVendorNciRspNtf(uint16_t dataLen, uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "AutoCard:: %s Enter ", __func__);

  if ((pData[NCI_GID_INDEX] != (NCI_MT_RSP | NCI_GID_PROP) &&
       (pData[NCI_GID_INDEX] != (NCI_MT_NTF | NCI_GID_PROP))) ||
      (pData[NCI_OID_INDEX] != AUTOCARD_FW_API_OID) ||
      ((dataLen > AUTOCARD_STATUS_INDEX) &&
       (pData[3] > AUTOCARD_GET_RF_PARAM))) {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  if (pData[NCI_GID_INDEX] == (NCI_MT_NTF | NCI_GID_PROP)) {
    std::vector<uint8_t> autocardNtf = {
        (NCI_MT_NTF | NCI_GID_PROP), NCI_ROW_MAINLINE_OID, AUTOCARD_PAYLOAD_LEN,
        AUTOCARD_FEATURE_SUB_GID};
    autocardNtf[NCI_MSG_LEN_INDEX] =
        pData[NCI_MSG_LEN_INDEX] + AUTOCARD_HEADER_LEN;
    autocardNtf.insert(autocardNtf.end(), pData + NCI_MSG_LEN_INDEX,
                       pData + dataLen);
    PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
        autocardNtf.size(), &autocardNtf[0]);
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }
  const uint8_t status = (dataLen > AUTOCARD_STATUS_INDEX)
                             ? pData[AUTOCARD_STATUS_INDEX]
                             : pData[3];
  std::vector<uint8_t> autocardRsp = {
      (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_MAINLINE_OID, AUTOCARD_PAYLOAD_LEN,
      AUTOCARD_FEATURE_SUB_GID};

  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s Set autocard failed. Error: %d", __func__, status);
    autocardRsp.push_back(AUTOCARD_HEADER_LEN);
    autocardRsp.push_back(AutoCard::getInstance()->autoCardCmdType);
    autocardRsp.push_back(status);
  } else {
    autocardRsp[NCI_MSG_LEN_INDEX] =
        pData[NCI_MSG_LEN_INDEX] + AUTOCARD_HEADER_LEN;
    autocardRsp.insert(autocardRsp.end(), pData + NCI_MSG_LEN_INDEX,
                       pData + dataLen);
    if (autocardRsp[AUTOCARD_SUB_OID_IDEX] == AUTOCARD_SET_COUNTERS_SUB_OID)
      autocardRsp[AUTOCARD_SUB_OID_IDEX] =
          AutoCard::getInstance()->autoCardCmdType;
  }
  PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
      autocardRsp.size(), &autocardRsp[0]);
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

NFCSTATUS AutoCard::handleVendorNciMessage(uint16_t dataLen, uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "AutoCard::%s Enter", __func__);

  if ((pData[NCI_GID_INDEX] != (NCI_MT_CMD | NCI_GID_PROP)) ||
      (pData[NCI_OID_INDEX] != NCI_ROW_PROP_OID_VAL) ||
      (pData[NCI_MSG_INDEX_FOR_FEATURE] != AUTOCARD_FEATURE_SUB_GID) ||
      (pData[AUTOCARD_SUB_OID_IDEX] > AUTOCARD_GET_RF_PARAM)) {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  uint8_t autocard_selection_mode = 0x00;
  uint8_t autocardStatus = NFCSTATUS_SUCCESS;
  AutoCard::getInstance()->autoCardCmdType = pData[AUTOCARD_SUB_OID_IDEX];

  if (PlatformAbstractionLayer::getInstance()->palGetChipType() != sn220u) {
    autocardStatus = AUTOCARD_STATUS_FEATURE_NOT_SUPPORTED;
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s:AutoCard selection is not supported.", __func__);
  } else if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
                 NAME_NXP_AUTOCARD_SELECTION_PHONE_OFF,
                 &autocard_selection_mode, sizeof(autocard_selection_mode))) {
    autocardStatus = AUTOCARD_STATUS_NOT_CONFIGURED;
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s:AutoCard selection is not configured.", __func__);
  } else if (autocard_selection_mode != AUTOCARD_FEATURE_ENABLED ||
             (mAutoCardEnableStatus != AUTOCARD_FEATURE_ENABLED &&
              pData[AUTOCARD_SUB_OID_IDEX] !=
                  AUTOCARD_FEATURE_ENABLE_SUB_OID)) {
    autocardStatus = AUTOCARD_STATUS_DISABLED;
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s:AutoCard selection is Disabled.", __func__);
  } else {
    std::vector<uint8_t> autocardCmd(pData, pData + dataLen);
    autocardCmd[NCI_OID_INDEX] = AUTOCARD_FW_API_OID;
    autocardCmd[NCI_MSG_LEN_INDEX]--;
    autocardCmd.erase(autocardCmd.begin() + NCI_MSG_INDEX_FOR_FEATURE);

    if (pData[AUTOCARD_SUB_OID_IDEX] == AUTOCARD_FEATURE_ENABLE_SUB_OID ||
        pData[AUTOCARD_SUB_OID_IDEX] == AUTOCARD_FEATURE_DISABLE_SUB_OID) {
      if (pData[AUTOCARD_SUB_OID_IDEX] == AUTOCARD_FEATURE_ENABLE_SUB_OID &&
          mAutoCardEnableStatus == AUTOCARD_FEATURE_ENABLED) {
        std::vector<uint8_t> autocardRsp = {
            (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_MAINLINE_OID,
            AUTOCARD_PAYLOAD_LEN,        AUTOCARD_FEATURE_SUB_GID,
            AUTOCARD_HEADER_LEN,         AUTOCARD_FEATURE_ENABLE_SUB_OID,
            AUTOCARD_STATUS_SUCCESS};
        PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
            autocardRsp.size(), autocardRsp.data());
        NXPLOG_EXTNS_D(
            NXPLOG_ITEM_NXP_GEN_EXTN,
            "%s: AutoCard is enabled in NFCC and skipping the set counters "
            "command.",
            __func__);
        return NFCSTATUS_EXTN_FEATURE_SUCCESS;
      }
      autocardCmd[NCI_MSG_INDEX_FOR_FEATURE] = AUTOCARD_SET_COUNTERS_SUB_OID;
      autocardCmd[NCI_MSG_LEN_INDEX] += CNT_CONFIG_BUFF_MAX_SIZE;
      mAutoCardEnableStatus = pData[dataLen - 1];
      autocardCmd.insert(autocardCmd.end(), mAutoCardCounters.begin(),
                         mAutoCardCounters.end());
    }
    const NFCSTATUS status =
        PlatformAbstractionLayer::getInstance()->palenQueueWrite(
            autocardCmd.data(), autocardCmd.size());
    if (status != NFCSTATUS_SUCCESS) {
      autocardStatus = AUTOCARD_STATUS_CMD_FAIL;
      NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "AutoCard::%s failed status: %d",
                     __func__, status);
    }
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
    PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
        autocardRsp.size(), autocardRsp.data());
  }

  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}
