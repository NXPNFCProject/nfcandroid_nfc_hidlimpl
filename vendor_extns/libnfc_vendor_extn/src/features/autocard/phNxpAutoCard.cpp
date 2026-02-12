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

#define HCI_TRANSCEIVE_TIMEOUT 2000
#define NCI_GID_IDX 0
#define NCI_OID_IDX 1
#define NCI_LEN_IDX 2
#define HCI_PKT_HDR_IDX 3
#define HCI_MSG_HDR_IDX 4
#define MIN_NCI_PAYLOAD 2
#define CREDIT_NTF_CONN_ID_IDX 4
#define CREDIT_NTF_CREDITS_IDX 5
#define NCI_CORE_CREDIT_NTF 0x06
#define HCI_APDU_PIPE_ID 0x19
#define DEFAULT_MAX_WTX_COUNT 30
#define HCI_CHAIN_MASK 0x80
#define HCI_PIPE_ID_MASK 0x7F
#define HCI_MSG_TYPE_MASK 0xC0
#define HCI_MSG_INS_MASK 0x3F
#define INST_EVT_WTX 0x11
#define HCI_MSG_TYPE_EVT 1
#define HCI_MSG_TYPE_RSP 2
#define HCI_MSG_TYPE_INVALID 3
#define STATUS_MAX_WTX_REACHED 0xE0
#define NCI_GID(NciPkt) ((NciPkt[NCI_GID_IDX] & NCI_GID_MASK))
#define NCI_OID(NciPkt) ((NciPkt[NCI_OID_IDX] & NCI_OID_MASK))
#define HCI_UNCHAINED(NciPkt) ((NciPkt[HCI_PKT_HDR_IDX] & HCI_CHAIN_MASK) >> 7)
#define HCI_CHAINED(NciPkt) !HCI_UNCHAINED(NciPkt)
#define HCI_PIPE_ID(NciPkt) (NciPkt[HCI_PKT_HDR_IDX] & HCI_PIPE_ID_MASK)
#define HCI_MSG_TYPE(NciPkt)                                                   \
  ((NciPkt[HCI_MSG_HDR_IDX] & HCI_MSG_TYPE_MASK) >> 6)
#define HCI_MSG_INS(NciPkt) (NciPkt[HCI_MSG_HDR_IDX] & HCI_MSG_INS_MASK)
#define FRAME_HCI_MSG_HEADER(MsgType, Ins) ((MsgType << 6) | Ins)
#define CREDITS_AVAILABLE(NciPkt) (NciPkt[CREDIT_NTF_CREDITS_IDX])
#define INST_EVT_UNKNOWN 0x00

AutoCard *AutoCard::sAutoCard = nullptr;

AutoCard::AutoCard() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "AutoCard::%s Enter ", __func__);
  autoCardCmdType = 0;
  isPipeTobeCreated = false;
  isHciCmdSent = false;
  mFirstFragment = true;
  mWTX = false;
  mWtxCount = 0;
  mMaxWtxCount = DEFAULT_MAX_WTX_COUNT;
  mAutoCardEnableStatus = 0;
  mAutoCardCounters.clear();
  mLastSentCmd.clear();
  mResponseTimeout = std::chrono::milliseconds(0);
  mWaitForResponse = false;
  mIsStrPhoneOffEnabled = false;
  mIsPutAppletStatusBackEnabled = false;
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

NFCSTATUS AutoCard::phNxpNciHal_handleHciAutoCardRsp(uint8_t *rsp,
                                                     uint16_t rspLen) {
  uint8_t autocard_selection_mode = 0x00;

  if (PlatformAbstractionLayer::getInstance()->palGetChipType() != sn220u ||
      !mWaitForResponse)
    return NFCSTATUS_FAILED;

  if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
          NAME_NXP_AUTOCARD_SELECTION_PHONE_OFF, &autocard_selection_mode,
          sizeof(autocard_selection_mode))) {
    return NFCSTATUS_FAILED;
  }
  if (autocard_selection_mode != AUTOCARD_FEATURE_ENABLED)
    return NFCSTATUS_FAILED;

  if (isCreditNtfForHci(rsp, rspLen)) {
    return NFCSTATUS_SUCCESS;
  } else if (isValidHciPacket(rsp, rspLen)) {
    return processHciPacket(rsp, rspLen);
  }

  return NFCSTATUS_FAILED;
}

bool AutoCard::isCreditNtfForHci(uint8_t *rsp, uint16_t rspLen) {
  (void)rspLen;
  if (rsp[NCI_GID_IDX] == 0x60 && rsp[NCI_OID_IDX] == NCI_CORE_CREDIT_NTF &&
      rsp[NCI_LEN_IDX] == 0x03 && rsp[CREDIT_NTF_CONN_ID_IDX] == 0x01 &&
      rsp[CREDIT_NTF_CREDITS_IDX] == 0x01) {
    return true;
  }
  return false;
}

bool AutoCard::isValidHciPacket(uint8_t *rsp, uint16_t rspLen) {
  // Expect at least 3 bytes NCI Header & 2 bytes HCI Header.
  if (rspLen < (NCI_HEADER_LEN + MIN_NCI_PAYLOAD)) {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN, "is Not valid HCI Pkt");
    return false;
  }
  // Check if it is valid Event for apdu pipe
  if (NCI_GID(rsp) == 0x01 && NCI_OID(rsp) == 0x00 &&
      HCI_PIPE_ID(rsp) == HCI_APDU_PIPE_ID) {
    return true;
  }
  // Check if it is valid response for last sent packet.
  if (mLastSentCmd.size() > 0) {
    if (NCI_GID(mLastSentCmd) == NCI_GID(rsp) &&
        NCI_OID(mLastSentCmd) == NCI_OID(rsp) &&
        HCI_PIPE_ID(mLastSentCmd) == HCI_PIPE_ID(rsp)) {
      return true;
    }
  }
  // It is neither APDU PIPE EVT nor RESPONSE to LastSentCommand.
  // Treat it not a WiredSe Pkt
  return false;
}

NFCSTATUS AutoCard::processHciPacket(uint8_t *rsp, uint16_t rspLen) {
  if (!rsp || rspLen == 0) {
    return NFCSTATUS_FAILED;
  }

  uint8_t pipeId = HCI_PIPE_ID(rsp);
  bool isChained = HCI_CHAINED(rsp);
  int dataIdx = HCI_MSG_HDR_IDX;
  uint8_t msgType = HCI_MSG_TYPE(rsp);
  uint8_t inst = HCI_MSG_INS(rsp);

  std::unique_lock lk(mRspMutex);

  switch (msgType) {
  case HCI_MSG_TYPE_RSP:
    if (mFirstFragment) {
      mResponse.pipeId = pipeId;
      mResponse.status = inst;
    }
    break;

  case HCI_MSG_TYPE_EVT:
    if (inst == INST_EVT_WTX) {
      mWtxCount++;
      NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                     "%s EVT_WTX received, WtxCount=%u", __func__, mWtxCount);
      mWTX = true;
      mRspCond.notify_one(); // Wake up waiting thread
      return NFCSTATUS_SUCCESS;
    }
    break;

  default:
    return NFCSTATUS_FAILED;
  }
  // Common data handling for RSP and EVT
  mResponse.data.insert(mResponse.data.end(), rsp + dataIdx, rsp + rspLen);
  if (!isChained) {
    mWaitForResponse = false;
    mRspCond.notify_one();
    mFirstFragment = true;
  } else {
    mFirstFragment = false;
  }

  return NFCSTATUS_SUCCESS;
}

NFCSTATUS AutoCard::sendHciCmd(std::vector<uint8_t> &data, HciRspPkt &rsp) {
  std::vector<uint8_t> endOfApduCmd = {0x01, 0x00, 0x02, 0x99, 0x61};
  if (data.empty()) {
    return NFCSTATUS_FAILED;
  }

  NFCSTATUS status = NFCSTATUS_FAILED;
  bool isChained = HCI_CHAINED(data.data());
  mResponseTimeout = std::chrono::milliseconds(HCI_TRANSCEIVE_TIMEOUT);

  // Cache last command for potential retransmission
  mLastSentCmd.insert(mLastSentCmd.end(), data.begin(), data.end());
  mWaitForResponse = true;

  status = PlatformAbstractionLayer::getInstance()->palNfcTmlWrite(
      data.data(), (uint16_t)data.size());
  if (status != NFCSTATUS_SUCCESS) {
    usleep(1000 * 10);
    status = PlatformAbstractionLayer::getInstance()->palNfcTmlWrite(
        data.data(), (uint16_t)data.size());
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN, "Failed to write, status = %x",
                     status);
    }
  }

  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN, "Failed to write, status = %x",
                   status);
    mLastSentCmd.clear();
    return status;
  }

  if (data == endOfApduCmd) {
    // No response for End of APDU cmd.
    mLastSentCmd.clear();
    return NFCSTATUS_SUCCESS;
  }

  if (!isChained) {
    std::unique_lock lock(mRspMutex);
    mWtxCount = 0;

    // Wait for response or WTX events
    while (true) {
      mWTX = false; // Reset before wait
      if (!mRspCond.wait_for(lock, mResponseTimeout,
                             [this] { return !mWaitForResponse || mWTX; })) {
        ALOGE("Response timeout");
        mLastSentCmd.clear();
        return NFCSTATUS_FAILED;
      }

      if (!mWTX) {
        break; // Got response
      }

      mWtxCount++;
      if (mWtxCount >= mMaxWtxCount) {
        mLastSentCmd.clear();
        mWtxCount = 0;
        return STATUS_MAX_WTX_REACHED;
      }
    }

    // Copy response and cleanup
    rsp = mResponse;
    mResponse.pipeId = 0;
    mResponse.status = 0;
    mResponse.data.clear();
    mLastSentCmd.clear();
  }

  return NFCSTATUS_SUCCESS;
}

void AutoCard::phNxpNciHal_checkAndCreateApduPipe() {
  std::vector<uint8_t> powerLinkOnCmd = {0x22, 0x03, 0x02, 0xC0, 0x03};
  std::vector<uint8_t> modeSetCmd = {0x22, 0x01, 0x02, 0xC0, 0x01};
  std::vector<uint8_t> apduPipeStatusCmd = {0x20, 0x03, 0x03, 0x01, 0xA0, 0x23};
  std::vector<uint8_t> createPipeCmd = {0x01, 0x00, 0x05, 0x81,
                                        0x10, 0x11, 0xC0, 0x30};
  std::vector<uint8_t> openPipeCmd = {0x01, 0x00, 0x02, 0x99, 0x03};
  std::vector<uint8_t> hciEvtAbort = {0x01, 0x00, 0x02, 0x99, 0x51};
  std::vector<uint8_t> endOfApduCmd = {0x01, 0x00, 0x02, 0x99, 0x61};
  uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
  uint16_t rsp_len = 0;
  bool isPipeOpenCmdSent = false;

  uint8_t autocard_selection_mode = 0x00;
  HciRspPkt hciRsp;

  if (PlatformAbstractionLayer::getInstance()->palGetChipType() != sn220u ||
      !isPipeTobeCreated)
    return;

  isPipeTobeCreated = false;

  NFCSTATUS status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
      powerLinkOnCmd.size(), powerLinkOnCmd.data(), &rsp_len, rsp);
  if (status != NFCSTATUS_SUCCESS || rsp_len != 4 || rsp[3] != 0x00)
    return;

  status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
      modeSetCmd.size(), modeSetCmd.data(), &rsp_len, rsp);
  if (status != NFCSTATUS_SUCCESS || rsp_len != 4 || rsp[3] != 0x00)
    goto disable_pwr_link;

  status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
      apduPipeStatusCmd.size(), apduPipeStatusCmd.data(), &rsp_len, rsp);
  if (status == NFCSTATUS_SUCCESS && rsp_len == 9 && rsp[8] == 0x00) {
    status = AutoCard::getInstance()->sendHciCmd(createPipeCmd, hciRsp);
    if (status != NFCSTATUS_SUCCESS || hciRsp.status != 0x00)
      goto disable_pwr_link;

    isPipeOpenCmdSent = true;
    status = AutoCard::getInstance()->sendHciCmd(openPipeCmd, hciRsp);
    if (status != NFCSTATUS_SUCCESS || hciRsp.status != 0x00)
      goto disable_pwr_link;

    status = AutoCard::getInstance()->sendHciCmd(hciEvtAbort, hciRsp);
    if (status != NFCSTATUS_SUCCESS || hciRsp.status != 0x00)
      goto disable_pwr_link;
  }
// Deactivate link
disable_pwr_link:
  powerLinkOnCmd[4] = 0x01;
  status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
      powerLinkOnCmd.size(), powerLinkOnCmd.data(), &rsp_len, rsp);
  if (status != NFCSTATUS_SUCCESS)
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s Deactivate powr link failed.: %d", __func__, status);

  if (!isPipeOpenCmdSent)
    return;

  status = AutoCard::getInstance()->sendHciCmd(endOfApduCmd, hciRsp);
  if (status != NFCSTATUS_SUCCESS)
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s Failed to send End of APDU: %d", __func__, status);

  mWaitForResponse = false;
}

void AutoCard::phNxpNciHal_getAutoCardConfig() {
  constexpr uint8_t AUTOCARD_FEATURE_CONFIG_GET_INDEX = 0x05;
  constexpr uint8_t AUTOCARD_GET_CNT_RSP_LEN = 12;
  constexpr uint8_t AUTOCARD_GET_TIMER_RSP_LEN = 6;
  constexpr uint8_t COUNTER_START_INDEX = 6;
  constexpr uint8_t NO_OF_CNT_TO_UPDATE = 6;
  constexpr uint8_t AUTOCARD_TIMER_GET_INDEX = 0x05;
  constexpr uint8_t AUTOCARD_TIMER_GET_STATUS_INDEX = 0x04;
  constexpr uint8_t STR_PHONE_OFF_ENABLE_RSP_LEN = 5;
  constexpr uint8_t STR_MAX_TIME_OUT = 50;
  constexpr uint8_t STR_MAX_DEFAULT_VALUE = 30;

  uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
  uint16_t rsp_len = 0;

  if ((PlatformAbstractionLayer::getInstance()->palGetChipType() != sn220u) &&
      (PlatformAbstractionLayer::getInstance()->palGetChipType() != sn300u))
    return;

  uint8_t autocard_selection_mode = 0x00;
  if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
          NAME_NXP_AUTOCARD_SELECTION_PHONE_OFF, &autocard_selection_mode,
          sizeof(autocard_selection_mode))) {
    return;
  }

  if (autocard_selection_mode != AUTOCARD_FEATURE_ENABLED)
    return;

  uint8_t value = 0x00;
  mIsPutAppletStatusBackEnabled = false;
  if (PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
          NAME_NXP_AUTOCARD_PUT_APPLET_STATUS_BACK, &value, sizeof(value))) {
    if (value == AUTOCARD_FEATURE_ENABLED)
      mIsPutAppletStatusBackEnabled = true;
  }

  uint32_t mFwVer = 0;
  std::vector<uint8_t> mFwRsp = NciStateMonitor::getInstance()->getFwVersion();
  if (mFwRsp.size() > 2)
    mFwVer = (((uint32_t)mFwRsp[0]) << 16U) | (((uint32_t)mFwRsp[1]) << 8U) |
             mFwRsp[2];

  if ((PlatformAbstractionLayer::getInstance()->palGetChipType() == sn300u) &&
      (mFwVer > DEFAULT_STR_PHONEOFF_SUPPORT_MIN_FW_VER)) {
    uint8_t strPhoneOff = 0x00;
    uint8_t strMaxNoOfEvents = STR_MAX_DEFAULT_VALUE;
    uint8_t strReaderSelectionTimeOut = STR_MAX_DEFAULT_VALUE;
    mIsStrPhoneOffEnabled = false;
    if (PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
            NAME_NXP_STR_ENABLE_PHONE_OFF, &strPhoneOff, sizeof(strPhoneOff))) {
      if (strPhoneOff == 0x01) {
        mIsStrPhoneOffEnabled = true;
        if (PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
                NAME_NXP_MAX_NO_RF_EVENTS, &strMaxNoOfEvents,
                sizeof(strMaxNoOfEvents))) {
          if (strMaxNoOfEvents > STR_MAX_DEFAULT_VALUE)
            strMaxNoOfEvents = STR_MAX_DEFAULT_VALUE;
        }
        if (PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
                NAME_NXP_STR_READER_SELECTION_TIMEOUT, &strReaderSelectionTimeOut,
                sizeof(strReaderSelectionTimeOut))) {
          if (strReaderSelectionTimeOut > STR_MAX_TIME_OUT)
            strReaderSelectionTimeOut = STR_MAX_TIME_OUT;
        }
      }
    }
    std::vector<uint8_t> setStrAutoSelection = {0x2F,
                                                0x43,
                                                0x04,
                                                0x20,
                                                strPhoneOff,
                                                strReaderSelectionTimeOut,
                                                strMaxNoOfEvents};
    NFCSTATUS status =
        PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
            setStrAutoSelection.size(), setStrAutoSelection.data(), &rsp_len,
            rsp);
    bool validResponse =
        (status == NFCSTATUS_SUCCESS) &&
        (rsp_len == STR_PHONE_OFF_ENABLE_RSP_LEN) &&
        (rsp[NCI_MSG_INDEX_FOR_FEATURE] == STR_ACS_FEATURE_ENABLE_SUB_OID);
    if (!validResponse) {
      mIsStrPhoneOffEnabled = false;
      NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                     "%s: Failed to enable STR Phone Off detection feature",
                     __func__);
    }
  } else {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s: FW not supported for STR Phone Off detection feature",
                   __func__);
  }

  isPipeTobeCreated = true;

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
      if (mIsPutAppletStatusBackEnabled)
        mAutoCardEnableStatus |= 0x02;
      else
        mAutoCardEnableStatus &= ~0x02;

      setAutoCardCounters[NCI_MSG_INDEX_FEATURE_VALUE] = mAutoCardEnableStatus;

      status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
          setAutoCardCounters.size(), setAutoCardCounters.data(), &rsp_len,
          rsp);
      if (status == NFCSTATUS_SUCCESS)
        mAutoCardCounters = readConfCnt;
    }
  }

  uint8_t autocard_timer_val = 0x00;
  if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
          NAME_NXP_AUTOCARD_AID_SWITCH_TIME, &autocard_timer_val,
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

  if (AutoCard::getInstance()->phNxpNciHal_handleHciAutoCardRsp(
          pData, dataLen) == NFCSTATUS_SUCCESS) {
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s => %d, HCI Autocard Packet",
                   __func__, __LINE__);
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  if ((pData[NCI_GID_INDEX] != (NCI_MT_RSP | NCI_GID_PROP) &&
       (pData[NCI_GID_INDEX] != (NCI_MT_NTF | NCI_GID_PROP))) ||
      (pData[NCI_OID_INDEX] != AUTOCARD_FW_API_OID) ||
      ((dataLen > AUTOCARD_STATUS_INDEX) &&
       (pData[3] > STR_SET_ACTIVATE_AID))) {
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

  if ((dataLen <= NCI_MSG_INDEX_FEATURE_VALUE) ||
      (pData[NCI_GID_INDEX] != (NCI_MT_CMD | NCI_GID_PROP)) ||
      (pData[NCI_OID_INDEX] != NCI_ROW_PROP_OID_VAL) ||
      (pData[NCI_MSG_INDEX_FOR_FEATURE] != AUTOCARD_FEATURE_SUB_GID) ||
      (pData[AUTOCARD_SUB_OID_IDEX] > STR_SET_ACTIVATE_AID)) {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  uint8_t autocard_selection_mode = 0x00;
  uint8_t autocardStatus = NFCSTATUS_SUCCESS;
  AutoCard::getInstance()->autoCardCmdType = pData[AUTOCARD_SUB_OID_IDEX];

  if (((PlatformAbstractionLayer::getInstance()->palGetChipType() != sn220u) &&
       (PlatformAbstractionLayer::getInstance()->palGetChipType() != sn300u)) ||
      ((PlatformAbstractionLayer::getInstance()->palGetChipType() == sn300u) &&
       !mIsStrPhoneOffEnabled &&
       pData[AUTOCARD_SUB_OID_IDEX] > AUTOCARD_GET_RF_PARAM)) {
    autocardStatus = AUTOCARD_STATUS_FEATURE_NOT_SUPPORTED;
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s:AutoCard selection is not supported.", __func__);
    if (PlatformAbstractionLayer::getInstance()->palGetChipType() == sn300u)
      NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                     "%s: STR Reader profile will not support.", __func__);
  } else if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
                 NAME_NXP_AUTOCARD_SELECTION_PHONE_OFF,
                 &autocard_selection_mode, sizeof(autocard_selection_mode))) {
    autocardStatus = AUTOCARD_STATUS_NOT_CONFIGURED;
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s:AutoCard selection is not configured.", __func__);
  } else if ((!mIsStrPhoneOffEnabled &&
              pData[AUTOCARD_SUB_OID_IDEX] <= AUTOCARD_GET_RF_PARAM) &&
             (autocard_selection_mode != AUTOCARD_FEATURE_ENABLED ||
              ((mAutoCardEnableStatus & AUTOCARD_FEATURE_ENABLED) !=
                   AUTOCARD_FEATURE_ENABLED &&
               pData[AUTOCARD_SUB_OID_IDEX] !=
                   AUTOCARD_FEATURE_ENABLE_SUB_OID))) {
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
          ((mAutoCardEnableStatus & AUTOCARD_FEATURE_ENABLED) ==
           AUTOCARD_FEATURE_ENABLED)) {
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
      if (mIsPutAppletStatusBackEnabled)
        mAutoCardEnableStatus |= 0x02;
      else
        mAutoCardEnableStatus &= ~0x02;

      autocardCmd[NCI_MSG_INDEX_FEATURE_VALUE] = mAutoCardEnableStatus;

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
