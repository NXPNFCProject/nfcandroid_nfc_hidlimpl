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
#include "phNxpNTag.h"

#include <phNxpLog.h>

#include <cstdint>

#include "NciDiscoveryCommandBuilder.h"
#include "NxpNfcExtension.h"
#include "ObserveMode.h"

extern uint32_t wFwVerRsp;
NxpNTag* NxpNTag::sNxpNTag = nullptr;

NxpNTag::NxpNTag() {
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter ", __func__);
  clearNTagFlags();
}

void NxpNTag::clearNTagFlags() {
  mNtagControl.mQPOLLMode = 0x00;
  mNtagControl.mNtagDetectStatus = 0x00;
  mNtagControl.mCmdRspStatus = 0x00;
  mNtagControl.mNtagUid.clear();
  mNtagControl.mCurrentDiscCmd.clear();
  mNtagControl.mNtagEnableRequest = false;
  mNtagControl.isNTagNtfCmdReq = false;
  mNtagControl.isNTagNtfEnabled = false;
  mWaitingforDiscRsp = false;
  mNTagState = NTagState::NTAG_STATE_IDLE;
  mNTagSetSubState = NTagSetSubState::NTAG_SET_SUB_STATE_IDLE;
}

NxpNTag::~NxpNTag() {
  NXPLOG_NCIHAL_D("~NxpNTag::%s Enter ", __func__);
  clearNTagFlags();
}

NxpNTag* NxpNTag::getInstance() {
  if (sNxpNTag == nullptr) {
    sNxpNTag = new NxpNTag();
  }
  return sNxpNTag;
}

bool NxpNTag::isNtagSupported() {
  if (IS_CHIP_TYPE_NE(sn300u)) {
    NXPLOG_NCIHAL_E("NxpNTag::%s Chip will not support", __func__);
    return false;
  }

  if (wFwVerRsp < DEFAULT_NTAG_SUPPORT_MIN_FW_VER) {
    NXPLOG_NCIHAL_E("FW not supported for Ntag detection feature");
    return false;
  }

  return true;
}

void NxpNTag::phNxpNciHal_disableNtagNtfConfig() {
  NFCSTATUS status = NFCSTATUS_SUCCESS;

  if (!NxpNTag::isNtagSupported()) return;

  uint8_t getNtagNtfConfig[] = {0x20, 0x03, 0x03, 0x01, 0xA1, 0xDA};
  constexpr uint8_t PROP_NTF_STATUS_INDEX = 8;
  uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
  uint16_t rsp_len = 0;
  status = phNxpNciHal_send_ext_cmd(sizeof(getNtagNtfConfig), getNtagNtfConfig,
                                    &rsp_len, rsp);
  if ((status == NFCSTATUS_SUCCESS) &&
      (rsp[PROP_NTF_STATUS_INDEX] == NTAG_NTF_ENABLE_STATE)) {
    uint8_t setNtagNtfConfig[] = {0x20, 0x02, 0x05, 0x01,
                                  0xA1, 0xDA, 0x01, 0x00};
    status = phNxpNciHal_send_ext_cmd(sizeof(setNtagNtfConfig),
                                      setNtagNtfConfig, &rsp_len, rsp);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("Ntag Set Config : Failed");
    }
  }
}

NFCSTATUS NxpNTag::waitForRfDiscRsp(NFCSTATUS status) {
  NXPLOG_NCIHAL_E("NxpNTag::%s Enter", __func__);
  constexpr uint8_t NCI_CMD_TIMEOUT_IN_SEC = 2;
  mNTagDiscRspCv.lock();
  mNTagDiscRspCv.timedWait(NCI_CMD_TIMEOUT_IN_SEC);
  mNTagDiscRspCv.unlock();

  if (mWaitingforDiscRsp) status = NFCSTATUS_FAILED;

  if (status == NFCSTATUS_FAILED && IDLE == phNxpExtn_NfcGetRfState()) {
    phNxpHal_EnqueueWrite(mNtagControl.mCurrentDiscCmd.data(),
                          mNtagControl.mCurrentDiscCmd.size());
  }
  return status;
}

NFCSTATUS NxpNTag::setNTagMode(uint16_t dataLen, uint8_t* pData) {
  NFCSTATUS status = NFCSTATUS_FAILED;

  switch ((pData[NCI_MSG_INDEX_FOR_FEATURE] & SUB_OID_MASK)) {
    case NTAG_ENABLE_PROP_OID:
      mWaitingforDiscRsp = true;
      mNtagControl.mNtagEnableRequest = true;
      mNtagControl.mQPOLLMode = NFC_RF_DISC_REPLACE_QPOLL;
      mNtagControl.mCurrentDiscCmd =
          NciDiscoveryCommandBuilderInstance.getDiscoveryCommand();
      for (auto it = mNtagControl.mCurrentDiscCmd.begin();
           it != mNtagControl.mCurrentDiscCmd.end();) {
        if (*it == NCI_TECH_Q_POLL_VAL &&
            (it + 1 != mNtagControl.mCurrentDiscCmd.end()) &&
            *(it + 1) == QTAG_ENABLE_OID)
          it = mNtagControl.mCurrentDiscCmd.erase(
              it, it + 2);  // Erase 0x71 and the following 0x01
        else
          ++it;
      }
      if (!mNtagControl.mNTagTimer.create(NULL, QPollTimerTimeoutCallback))
        NXPLOG_NCIHAL_E("%s Timer create failed", __func__);

      updateState(NTagState::NTAG_STATE_ENABLE);
      mNTagSetSubState = NTagSetSubState::NTAG_SET_SUB_STATE_IDLE;
      status = processNTagEvent(NTagEvent::ACTION_NTAG_ENABLE_REQUEST);
      break;

    case NTAG_DISABLE_PROP_OID:
      mWaitingforDiscRsp = true;
      mNtagControl.mNtagEnableRequest = false;
      mNtagControl.mQPOLLMode = NFC_RF_DISC_START;
      mNtagControl.mNtagDetectStatus = 0x00;
      mNtagControl.mNTagTimer.kill();
      updateState(NTagState::NTAG_STATE_DISABLE);
      mNTagSetSubState = NTagSetSubState::NTAG_SET_SUB_STATE_IDLE;
      status = processNTagEvent(NTagEvent::ACTION_NTAG_DISABLE_REQUEST);
      break;

    case NTAG_MODE_ENABLED_STATUS_OID: {
      // Compose response based on current enable status
      uint8_t NTAG_MODE_ENABLED_STATUS[] = {
          (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
          (QTAG_FEATURE_SUB_GID | NTAG_MODE_ENABLED_STATUS_OID),
          (mNtagControl.mNtagEnableRequest ? NTAG_STATUS_SUCCESS
                                           : NTAG_STATUS_FAILED)};
      phNxpHal_NfcDataCallback(sizeof(NTAG_MODE_ENABLED_STATUS),
                               NTAG_MODE_ENABLED_STATUS);
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
    }
    default:
      NXPLOG_NCIHAL_E("NxpNTag::%s Invalid action", __func__);
      return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  if (status == NFCSTATUS_FAILED) {
    NXPLOG_NCIHAL_E(
        "NxpNTag::%s setNTagMode failed and revert to default RF discovery",
        __func__);
    mNtagControl.mNtagEnableRequest = false;
  }

  // Common vendor response for enable/disable
  uint8_t NTAG_NCI_VENDOR_RSP[] = {
      (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      pData[NCI_MSG_INDEX_FOR_FEATURE],
      (status == NFCSTATUS_SUCCESS ? NTAG_STATUS_SUCCESS : NTAG_STATUS_FAILED)};

  phNxpHal_NfcDataCallback(sizeof(NTAG_NCI_VENDOR_RSP), NTAG_NCI_VENDOR_RSP);

  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

NFCSTATUS NxpNTag::processNTagSetSubState(NTagEvent event) {
  NXPLOG_NCIHAL_D("%s : Current sub state %d, Event: %d", __func__,
                  mNTagSetSubState, event);
  NFCSTATUS status = NFCSTATUS_FAILED;
  bool notifyOnError = false;

  switch (mNTagSetSubState) {
    case NTagSetSubState::NTAG_SET_SUB_STATE_IDLE: {
      status = sendRfDeactivate();
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("%s : sendRfDeactivate failed", __func__);
        notifyOnError = true;
        break;
      }
      mNTagSetSubState =
          NTagSetSubState::NTAG_SET_SUB_STATE_WAIT_FOR_RF_IDLE_RSP;
      status = waitForRfDiscRsp(status);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("%s : waitForRfDiscRsp failed", __func__);
        notifyOnError = true;
        break;
      }
      updateState(NTagState::NTAG_STATE_RF_DISCOVERY);
      break;
    }
    case NTagSetSubState::NTAG_SET_SUB_STATE_WAIT_FOR_RF_IDLE_RSP: {
      if (event == NTagEvent::ACTION_NTAG_RF_DEACTIVATE_IDLE &&
          (mNtagControl.mCmdRspStatus != NFCSTATUS_SUCCESS)) {
        bool enable = (mNTagState == NTagState::NTAG_STATE_ENABLE);
        status = sendNTagPropConfig(enable);
        if (status != NFCSTATUS_SUCCESS) {
          NXPLOG_NCIHAL_E("%s : sendNTagPropConfig(%s) failed", __func__,
                          enable ? "true" : "false");
          notifyOnError = true;
          break;
        }
        mNTagSetSubState =
            NTagSetSubState::NTAG_SET_SUB_STATE_WAIT_FOR_PROP_CMD_RSP;
      } else {
        NXPLOG_NCIHAL_E("%s : Unexpected event %d in WAIT_FOR_RF_IDLE_RSP",
                        __func__, event);
        notifyOnError = true;
      }
      break;
    }
    case NTagSetSubState::NTAG_SET_SUB_STATE_WAIT_FOR_PROP_CMD_RSP: {
      if (event == NTagEvent::ACTION_NTAG_PROP_NTF_SET_STATUS &&
          (mNtagControl.mCmdRspStatus != NFCSTATUS_SUCCESS)) {
        status = sendRfDiscCmd(mNtagControl.mQPOLLMode);
        if (status != NFCSTATUS_SUCCESS) {
          NXPLOG_NCIHAL_E("%s : sendRfDiscCmd failed", __func__);
          notifyOnError = true;
          break;
        }
        mNTagSetSubState =
            NTagSetSubState::NTAG_SET_SUB_STATE_WAIT_FOR_RF_DISC_RSP;
      } else {
        NXPLOG_NCIHAL_E("%s : Unexpected event %d in WAIT_FOR_PROP_CMD_RSP",
                        __func__, event);
        notifyOnError = true;
      }
      break;
    }

    case NTagSetSubState::NTAG_SET_SUB_STATE_WAIT_FOR_RF_DISC_RSP: {
      if (mNtagControl.mCmdRspStatus != NFCSTATUS_SUCCESS)
        NXPLOG_NCIHAL_E(
            "%s : RF discovery command response:%d Event:%d in "
            "WAIT_FOR_RF_DISC_RSP",
            __func__, mNtagControl.mCmdRspStatus, event);

      if (mWaitingforDiscRsp) {
        mWaitingforDiscRsp = false;
        mNTagDiscRspCv.signal();
        NXPLOG_NCIHAL_D("%s : mNTagDiscRspCv signaled", __func__);
      }

      updateState(NTagState::NTAG_STATE_RF_DISCOVERY);
      status = NFCSTATUS_SUCCESS;
      break;
    }

    default: {
      NXPLOG_NCIHAL_E("%s : Invalid sub state %d", __func__, mNTagSetSubState);
      notifyOnError = true;
      break;
    }
  }

  // Notify waiting thread in case of error
  if (notifyOnError && mWaitingforDiscRsp) {
    mWaitingforDiscRsp = false;
    mNTagDiscRspCv.signal();
    NXPLOG_NCIHAL_D("%s : mNTagDiscRspCv signaled due to error", __func__);
  }

  return status;
}

void NxpNTag::updateState(NTagState state) {
  NXPLOG_NCIHAL_D("%s : NTag state old:%d new:%d", __func__, mNTagState, state);
  mNTagState = state;
}

NFCSTATUS NxpNTag::processNTagEvent(NTagEvent event) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  NTagState mState = NTagState::NTAG_STATE_IDLE;
  NXPLOG_NCIHAL_D("%s : Current state %d Event:%d", __func__, mNTagState,
                  event);

  switch (mNTagState) {
    case NTagState::NTAG_STATE_ENABLE:
    case NTagState::NTAG_STATE_DISABLE:
      if (event == NTagEvent::ACTION_NTAG_ENABLE_REQUEST ||
          event == NTagEvent::ACTION_NTAG_DISABLE_REQUEST ||
          event == NTagEvent::ACTION_NTAG_RF_DEACTIVATE_IDLE ||
          event == NTagEvent::ACTION_NTAG_PROP_NTF_SET_STATUS ||
          event == NTagEvent::ACTION_NTAG_RF_DISCOVERY) {
        status = processNTagSetSubState(event);
        mState = getState();
      }

      break;
    case NTagState::NTAG_STATE_RF_DEACTIVATE_IDLE:
      if (event == NTagEvent::ACTION_NTAG_RF_DEACTIVATE_IDLE ||
          event == NTagEvent::ACTION_NTAG_PROP_NTF_SET_STATUS) {
        status = sendRfDiscCmd(mNtagControl.mQPOLLMode);
        mState = NTagState::NTAG_STATE_RF_DISCOVERY;
      }
      break;
    case NTagState::NTAG_STATE_RF_DISCOVERY: {
      switch (event) {
        case NTagEvent::ACTION_NTAG_DISABLE_REQUEST:
          status = sendRfDeactivate();
          mState = NTagState::NTAG_STATE_DISABLE;
          break;
        case NTagEvent::ACTION_NTAG_RF_DISCOVERY:
          if (!mNtagControl.mNtagEnableRequest) {
            status = NFCSTATUS_SUCCESS;
            mState = NTagState::NTAG_STATE_IDLE;
          }
          if (mWaitingforDiscRsp) {
            mWaitingforDiscRsp = false;
            mNTagDiscRspCv.signal();
            NXPLOG_NCIHAL_D("%s : mNTagDiscRspCv signal", __func__);
          }
          break;
        case NTagEvent::ACTION_NTAG_REMOVAL_DETECTED:
        case NTagEvent::ACTION_NTAG_RF_LOAD_CHANGE_NTF:
          status = sendRfDeactivate();
          mState = NTagState::NTAG_STATE_RF_DEACTIVATE_IDLE;
          break;
        default:
          break;
      }
      break;
    }
    case NTagState::NTAG_STATE_SAME_UID_DETECTED: {
      if (event == NTagEvent::ACTION_NTAG_UID_MATCHED) {
        status = sendRfDeactivate();
        mState = NTagState::NTAG_STATE_RF_DEACTIVATE_IDLE;
      }
      break;
    }
    case NTagState::NTAG_STATE_PRESENCE_CHECK: {
      if (event == NTagEvent::ACTION_NTAG_RF_DEACTIVATE_IDLE) {
        status = sendRfDeactivate();
        mState = NTagState::NTAG_STATE_RF_DEACTIVATE_IDLE;
      }
      break;
    }
    default:
      status = NFCSTATUS_FAILED;
      NXPLOG_NCIHAL_E("%s : Invalid state:%d", __func__, mNTagState);
      break;
  }
  if (status == NFCSTATUS_SUCCESS && mNTagState != mState) updateState(mState);
  return status;
}

NFCSTATUS NxpNTag::handleNTagPropNtf(uint16_t dataLen, uint8_t* pData) {
  constexpr uint8_t NTAG_LOAD_CHANGE_NTF_LEN = 7;
  constexpr uint8_t NTAG_LOAD_CHANGE_NTF_INDEX = 6;
  constexpr uint8_t NTAG_LOAD_CHANGE_VAL = 0xA9;
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);

  if (dataLen == NTAG_LOAD_CHANGE_NTF_LEN &&
      pData[NTAG_LOAD_CHANGE_NTF_INDEX] == NTAG_LOAD_CHANGE_VAL) {
    mNtagControl.mQPOLLMode = NFC_RF_DISC_REPLACE_QPOLL;
    NFCSTATUS status =
        processNTagEvent(NTagEvent::ACTION_NTAG_RF_LOAD_CHANGE_NTF);
    if (status != NFCSTATUS_SUCCESS)
      NXPLOG_NCIHAL_E("NxpNTag::%s Failed to trigger RF discovery", __func__);
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }
  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

void NxpNTag::handleNTagPresenceCheckRsp() {
  if (!(mNtagControl.mNtagDetectStatus & NTAG_ACTIVATED_STATUS)) return;

  if (!(mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHECK_TIMER_STATUS)) {
    unsigned long timeout = NTAG_PRESENCE_CHECK_DEFAULT_CONF_VAL;
    if (!GetNxpNumValue(NAME_NXP_NTAG_PRESENCE_CHECK_TIMEOUT, &timeout,
                        sizeof(timeout))) {
      NXPLOG_NCIHAL_W(
          "%s: Failed to get NTAG timeout config, using default: %lu", __func__,
          timeout);
    }

    if (!(mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHECK_TIMEOUT)) {
      if (!QPollTimerStart(timeout)) {
        NXPLOG_NCIHAL_E("NxpNTag::%s Failed to start timer", __func__);
      }
      updateState(NTagState::NTAG_STATE_PRESENCE_CHECK);
    }
    mNtagControl.mNtagDetectStatus |= NTAG_PRESENCE_CHECK_TIMER_STATUS;
  }
  mNtagControl.mNtagDetectStatus |= NTAG_PRESENCE_CHK_STATUS;
}

void NxpNTag::handleNTagPresenceCheckNtf(uint8_t* pData) {
  if (!(mNtagControl.mNtagDetectStatus & NTAG_ACTIVATED_STATUS)) return;

  if (pData[NCI_MSG_INDEX_FOR_FEATURE] == NCI_MSG_RF_DISCOVER) {
    updateNTagRemoveStatus();
    mNtagControl.mNTagTimer.kill();
  } else if ((mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHECK_TIMEOUT) &&
             (mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHK_STATUS)) {
    mNtagControl.mNtagDetectStatus &= ~NTAG_PRESENCE_CHECK_TIMEOUT;
    pData[NCI_MSG_INDEX_FOR_FEATURE] = NCI_MSG_RF_DISCOVER;
  }
}

NFCSTATUS NxpNTag::handleNTagNciRsp(uint8_t* pData, uint16_t dataLen) {
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);
  const int NCI_CMD_STATUS_INDEX = 3;
  switch (pData[NCI_OID_INDEX] & NCI_OID_MASK) {
    case NCI_MSG_RF_DISCOVER:
      if (!mNtagControl.mQPOLLMode) return NFCSTATUS_EXTN_FEATURE_FAILURE;

      mNtagControl.mQPOLLMode = 0x00;
      mNtagControl.mCmdRspStatus = pData[NCI_CMD_STATUS_INDEX];
      processNTagEvent(NTagEvent::ACTION_NTAG_RF_DISCOVERY);
      mNtagControl.isNTagNtfCmdReq = false;

      if ((mNtagControl.mNtagDetectStatus &
           (NTAG_READ_COMPLETE | NTAG_PRESENCE_CHK_STATUS |
            NTAG_REMOVAL_STATUS)) != 0) {
        if ((mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS) ==
            NTAG_REMOVAL_STATUS)
          mNtagControl.mNtagDetectStatus = 0;

        vector<uint8_t> rfDeact_Ntf = {0x61, 0x06, 0x02, 0x03, 0x00};
        phNxpHal_NfcDataCallback(rfDeact_Ntf.size(), &rfDeact_Ntf[0]);
      }
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
      break;

    case NCI_MSG_RF_DEACTIVATE: {
      if (!mNtagControl.mQPOLLMode) return NFCSTATUS_EXTN_FEATURE_FAILURE;

      bool isQPollMode = mNtagControl.mQPOLLMode == NFC_RF_DISC_START ||
                         mNtagControl.mQPOLLMode == NFC_RF_DISC_REPLACE_QPOLL;

      processNTagEvent(NTagEvent::ACTION_NTAG_RF_DEACTIVATE_IDLE);

      if (mNtagControl.mQPOLLMode == NFC_RF_DISC_RESTART &&
          (mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS))
        return NFCSTATUS_EXTN_FEATURE_FAILURE;

      if (isQPollMode) return NFCSTATUS_EXTN_FEATURE_SUCCESS;

      if (mNtagControl.mQPOLLMode == NFC_RF_DISC_RESTART &&
          (mNtagControl.mNtagDetectStatus &
           (NTAG_READ_COMPLETE | NTAG_PRESENCE_CHK_STATUS))) {
        vector<uint8_t> rfDeAct_Rsp = {0x41, 0x06, 0x01, 0x00};
        phNxpHal_NfcDataCallback(rfDeAct_Rsp.size(), &rfDeAct_Rsp[0]);
        return NFCSTATUS_EXTN_FEATURE_SUCCESS;
      }
      break;
    }

    case NCI_MSG_CORE_SET_CONFIG:
      processNTagEvent(NTagEvent::ACTION_NTAG_PROP_NTF_SET_STATUS);
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
      break;
    case NCI_MSG_CORE_SET_POWER_SUB_STATE:
      stopNTagTimerInternal();
      // Clearing the NTAG detected status on screen state change.
      mNtagControl.mNtagDetectStatus = 0;
      return NFCSTATUS_EXTN_FEATURE_FAILURE;
      break;

    case NCI_MSG_RF_ISO_DEP_NAK_PRESENCE:
      handleNTagPresenceCheckRsp();
      break;
  }
  phNxpNciHal_client_data_callback(dataLen, pData);
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

NFCSTATUS NxpNTag::handleNTagNciNtf(uint8_t* pData, uint16_t dataLen) {
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);
  const int NCI_RF_IDLE_NFC_LEN = 5;
  switch (pData[NCI_OID_INDEX] & NCI_OID_MASK) {
    case NCI_MSG_RF_INTF_ACTIVATED:
      return handleRfIntfActivated(pData, dataLen);

    case NCI_MSG_RF_DEACTIVATE:
      if (dataLen >= NCI_RF_IDLE_NFC_LEN &&
          pData[NCI_MSG_INDEX_FOR_FEATURE] == NCI_MSG_RF_DISCOVER)
        updateState(NTagState::NTAG_STATE_RF_DISCOVERY);

      if ((!(mNtagControl.mNtagDetectStatus & NTAG_ACTIVATED_STATUS) ||
           mNtagControl.mQPOLLMode != NFC_RF_DISC_RESTART) &&
          ((mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS) !=
           NTAG_REMOVAL_STATUS))
        return NFCSTATUS_EXTN_FEATURE_FAILURE;
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
      break;

    case NCI_MSG_RF_ISO_DEP_NAK_PRESENCE:
      handleNTagPresenceCheckNtf(pData);
      break;

    case NCI_MSG_RF_INTF_PROP_NTF:
      if (handleNTagPropNtf(dataLen, pData) == NFCSTATUS_EXTN_FEATURE_FAILURE)
        return NFCSTATUS_EXTN_FEATURE_FAILURE;
      break;
  }
  phNxpNciHal_client_data_callback(dataLen, pData);
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

NFCSTATUS NxpNTag::handleVendorNciRspNtf(uint16_t dataLen, uint8_t* pData) {
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);

  if (!isNtagSupported()) return NFCSTATUS_EXTN_FEATURE_FAILURE;

  if (mNtagControl.mNtagEnableRequest != mNtagControl.isNTagNtfEnabled)
    mNtagControl.isNTagNtfCmdReq = true;

  if (!mNtagControl.mNtagEnableRequest && !mNtagControl.isNTagNtfCmdReq)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  const uint8_t mHeader = pData[NCI_GID_INDEX] & (NCI_MT_MASK | NCI_GID_MASK);
  const uint8_t msgType = pData[NCI_GID_INDEX] & NCI_MT_MASK;

  const bool isValidHeader = (mHeader == (NCI_MT_RSP | NCI_GID_RF_MANAGE)) ||
                             (mHeader == (NCI_MT_NTF | NCI_GID_RF_MANAGE)) ||
                             (mHeader == (NCI_MT_NTF | NCI_GID_PROP));

  if (!isValidHeader && !mNtagControl.isNTagNtfCmdReq)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  NFCSTATUS status = NFCSTATUS_EXTN_FEATURE_SUCCESS;
  if (msgType == NCI_MT_RSP)
    status = handleNTagNciRsp(pData, dataLen);
  else if (msgType == NCI_MT_NTF)
    status = handleNTagNciNtf(pData, dataLen);

  NXPLOG_NCIHAL_D("NxpNTag::%s Exit status:%d", __func__, status);
  return status;
}

NFCSTATUS NxpNTag::handleVendorNciMessage(uint16_t dataLen, uint8_t* pData) {
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);

  if (!isNtagSupported()) return NFCSTATUS_EXTN_FEATURE_FAILURE;

  // Handle vendor-specific NCI command
  if (dataLen >= 5 && (pData[NCI_GID_INDEX] == (NCI_MT_CMD | NCI_GID_PROP)) &&
      (pData[NCI_OID_INDEX] == NCI_ROW_PROP_OID_VAL) &&
      (pData[NCI_MSG_LEN_INDEX] == PAYLOAD_TWO_LEN) &&
      ((pData[NCI_MSG_INDEX_FOR_FEATURE] & SUB_GID_MASK) ==
       QTAG_FEATURE_SUB_GID)) {
    if (setNTagMode(dataLen, pData) == NFCSTATUS_EXTN_FEATURE_SUCCESS)
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  if (!mNtagControl.mNtagEnableRequest) return NFCSTATUS_EXTN_FEATURE_FAILURE;

  // Check if it's an RF Discovery/Idle cmd only.
  if ((pData[NCI_GID_INDEX] == (NCI_MT_CMD | NCI_GID_RF_MANAGE)) &&
      ((pData[NCI_OID_INDEX] == NCI_MSG_RF_DISCOVER) ||
       (pData[NCI_OID_INDEX] == NCI_MSG_RF_DEACTIVATE))) {
    vector<uint8_t> rfDiscCmd(pData, pData + dataLen);
    if (NFCSTATUS_EXTN_FEATURE_SUCCESS == processRfDiscCmd(rfDiscCmd))
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

NFCSTATUS NxpNTag::processRfDiscCmd(std::vector<uint8_t>& rfDiscCmd) {
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);
  constexpr uint8_t NFC_A_PASSIVE_POLL_MODE = 0x00;
  constexpr uint8_t NFC_B_PASSIVE_POLL_MODE = 0x01;
  constexpr uint8_t NFC_F_PASSIVE_POLL_MODE = 0x02;
  constexpr uint8_t NFC_ACTIVE_POLL_MODE = 0x03;
  constexpr uint8_t NFC_V_PASSIVE_POLL_MODE = 0x06;
  constexpr uint8_t POLL_MODE_ENABLE_STATE = 0x01;

  uint8_t msgType = rfDiscCmd[NCI_OID_INDEX] & NCI_GID_MASK;

  if (msgType == NCI_MSG_RF_DISCOVER) {
    if (!mNtagControl.mNtagUid.empty()) return NFCSTATUS_EXTN_FEATURE_FAILURE;

    bool isRfPollEnabled = false;

    for (auto it = rfDiscCmd.begin(); it + 1 != rfDiscCmd.end(); ++it) {
      uint8_t mode = *it;
      uint8_t state = *(it + 1);

      if ((mode == NFC_A_PASSIVE_POLL_MODE || mode == NFC_B_PASSIVE_POLL_MODE ||
           mode == NFC_F_PASSIVE_POLL_MODE || mode == NFC_ACTIVE_POLL_MODE ||
           mode == NFC_V_PASSIVE_POLL_MODE) &&
          state == POLL_MODE_ENABLE_STATE) {
        isRfPollEnabled = true;
        break;
      }
    }

    if (!isRfPollEnabled) return NFCSTATUS_EXTN_FEATURE_FAILURE;

    // Prepare RF Discover command with Q-Poll configuration
    constexpr uint8_t NCI_QTAG_PAYLOAD_LEN = 2;
    constexpr uint8_t NCI_RF_DISC_PAYLOAD_LEN_INDEX = 2;
    constexpr uint8_t NCI_RF_DISC_NUM_OF_CONFIG_INDEX = 3;

    std::vector<uint8_t> rfCmd = rfDiscCmd;
    rfCmd[NCI_RF_DISC_NUM_OF_CONFIG_INDEX]++;
    rfCmd[NCI_RF_DISC_PAYLOAD_LEN_INDEX] += NCI_QTAG_PAYLOAD_LEN;
    rfCmd.push_back(NCI_TECH_Q_POLL_VAL);
    rfCmd.push_back(QTAG_ENABLE_OID);

    // Start Q-Poll timer
    unsigned long timeout = NTAG_DETECT_TIMER_VALUE;
    if (!GetNxpNumValue(NAME_NXP_NTAG_DETECTION_TIMEOUT_VALUE, &timeout,
                        sizeof(timeout))) {
      NXPLOG_NCIHAL_W(
          "%s: Failed to get NTAG detect timer , using default: %lu", __func__,
          timeout);
    }

    if (!QPollTimerStart(timeout)) {
      NXPLOG_NCIHAL_E(
          "NxpNTag::%s Failed to start timer, reverting to default discovery",
          __func__);
      mNtagControl.mNtagDetectStatus = 0;
      return NFCSTATUS_EXTN_FEATURE_FAILURE;
    }
    mNtagControl.mNtagDetectStatus &= ~NTAG_ACTIVATED_STATUS;
    mNtagControl.mNtagDetectStatus |= NTAG_DETECT_TIMER_STATUS;

    // Send extended RF Discover command
    NFCSTATUS status = phNxpHal_EnqueueWrite(&rfCmd[0], rfCmd.size());
    if (status != NFCSTATUS_SUCCESS) return NFCSTATUS_EXTN_FEATURE_FAILURE;

    updateState(NTagState::NTAG_STATE_RF_DISCOVERY);
    mNtagControl.mQPOLLMode = 0x00;
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  if (msgType == NCI_MSG_RF_DEACTIVATE) {
    if (!(mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHK_STATUS) &&
        !(mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS))
      return NFCSTATUS_EXTN_FEATURE_FAILURE;

    uint8_t deactivateType = rfDiscCmd[RF_DISC_CMD_NO_OF_CONFIG_INDEX];

    if (deactivateType == NCI_DEACTIVATE_TYPE_DISCOVERY) {
      mNtagControl.mQPOLLMode = NFC_RF_DISC_RESTART;
      if (!(mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS))
        mNtagControl.mNtagDetectStatus |= NTAG_READ_COMPLETE;

      NXPLOG_NCIHAL_D("NxpNTag::%s RF Deactivate Discovery to idle", __func__);
      processNTagEvent(NTagEvent::ACTION_NTAG_RF_DEACTIVATE_IDLE);
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
    }
    if (deactivateType == NCI_DEACTIVATE_TYPE_SLEEP) {
      mNtagControl.mQPOLLMode = NFC_RF_DISC_RESTART;
      NXPLOG_NCIHAL_D("NxpNTag::%s RF Deactivate Sleep to Discovery", __func__);
      processNTagEvent(NTagEvent::ACTION_NTAG_RF_DEACTIVATE_IDLE);
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
    }
  }

  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

NFCSTATUS NxpNTag::sendNTagPropConfig(bool flag) {
  vector<uint8_t> setPropNtfEnable = {0x20, 0x02, 0x05, 0x01,
                                      0xA1, 0xDA, 0x01, 0x01};
  constexpr uint8_t PROP_NTF_SET_INDEX = 7;
  NFCSTATUS status;
  NXPLOG_NCIHAL_D("NxpNTag::%s Flag: %d", __func__, flag);
  if (!flag) {
    setPropNtfEnable[PROP_NTF_SET_INDEX] = 0x00;
    mNtagControl.isNTagNtfEnabled = false;
  } else {
    mNtagControl.isNTagNtfEnabled = true;
  }

  status = phNxpHal_EnqueueWrite(&setPropNtfEnable[0], setPropNtfEnable.size());
  if (status != NFCSTATUS_SUCCESS) return NFCSTATUS_FAILED;

  return status;
}

NFCSTATUS NxpNTag::sendRfDeactivate() {
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);

  if (IDLE == phNxpExtn_NfcGetRfState()) return NFCSTATUS_SUCCESS;

  vector<uint8_t> rfIdleCmd = {0x21, 0x06, 0x01, 0x00};
  mNtagControl.mCmdRspStatus = NFCSTATUS_FAILED;
  if (NFCSTATUS_SUCCESS ==
      phNxpHal_EnqueueWrite(&rfIdleCmd[0], rfIdleCmd.size()))
    return NFCSTATUS_SUCCESS;

  return NFCSTATUS_FAILED;
}

NFCSTATUS NxpNTag::sendRfDiscCmd(uint8_t pollMode) {
  constexpr uint8_t NCI_NTAG_PAYLOAD_LEN = 2;
  constexpr uint8_t NCI_RF_DISC_PAYLOAD_LEN_INDEX = 2;
  constexpr uint8_t NCI_RF_DISC_NUM_OF_CONFIG_INDEX = 3;
  vector<uint8_t> rfDiscCmd = {0x21, 0x03, 0x01, 0x00};
  bool sendRfDiscCmdFlag = true;

  if (IDLE != phNxpExtn_NfcGetRfState()) {
    if (NFCSTATUS_SUCCESS != sendRfDeactivate())
      NXPLOG_NCIHAL_E("%s Failed to send RF deactivate cmd", __func__);

    return NFCSTATUS_FAILED;
  }

  unsigned long timeout = NTAG_DETECT_TIMER_VALUE;
  if (!GetNxpNumValue(NAME_NXP_NTAG_DETECTION_TIMEOUT_VALUE, &timeout,
                      sizeof(timeout))) {
    NXPLOG_NCIHAL_W("%s: Failed to get NTAG detect timer , using default: %lu",
                    __func__, timeout);
  }
  switch (pollMode) {
    case NFC_RF_DISC_RESTART:
    case NFC_RF_DISC_START:
      rfDiscCmd = mNtagControl.mCurrentDiscCmd;
      break;
    case NFC_RF_DISC_REPLACE_QPOLL:
      rfDiscCmd[NCI_RF_DISC_NUM_OF_CONFIG_INDEX]++;
      rfDiscCmd[NCI_RF_DISC_PAYLOAD_LEN_INDEX] += NCI_NTAG_PAYLOAD_LEN;
      rfDiscCmd.push_back(NCI_TECH_Q_POLL_VAL);
      rfDiscCmd.push_back(QTAG_ENABLE_OID);
      if (isObserveModeEnabled()) {
        rfDiscCmd[NCI_RF_DISC_NUM_OF_CONFIG_INDEX]++;
        rfDiscCmd[NCI_RF_DISC_PAYLOAD_LEN_INDEX] += NCI_NTAG_PAYLOAD_LEN;
        rfDiscCmd.push_back(0xFF);
        rfDiscCmd.push_back(0x01);
      }
      mNtagControl.mNtagDetectStatus &= ~NTAG_ACTIVATED_STATUS;
      mNtagControl.mNtagDetectStatus |= NTAG_DETECT_TIMER_STATUS;
      mNtagControl.mNTagTimer.kill();

      if (!QPollTimerStart(timeout)) {
        NXPLOG_NCIHAL_E(
            "NxpNTag::%s Failed to start timer reverting to default discovery",
            __func__);
        mNtagControl.mNtagDetectStatus = 0;
        mNtagControl.mQPOLLMode = NFC_RF_DISC_START;
      }
      break;
    default:
      sendRfDiscCmdFlag = false;
      break;
  }
  if (sendRfDiscCmdFlag) {
    mNtagControl.mCmdRspStatus = NFCSTATUS_FAILED;
    if (NFCSTATUS_SUCCESS ==
        phNxpHal_EnqueueWrite(&rfDiscCmd[0], rfDiscCmd.size())) {
      return NFCSTATUS_SUCCESS;
    } else {
      QPollTimerStop();
    }
  }
  return NFCSTATUS_FAILED;
}

void NxpNTag::stopNTagTimerInternal() {
  bool stopTimer = false;

  if ((mNtagControl.mNtagDetectStatus & NTAG_DETECT_TIMER_STATUS) ==
      NTAG_DETECT_TIMER_STATUS) {
    mNtagControl.mNtagDetectStatus &= ~NTAG_DETECT_TIMER_STATUS;
    stopTimer = true;
  }

  if ((mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHECK_TIMER_STATUS) ==
      NTAG_PRESENCE_CHECK_TIMER_STATUS) {
    mNtagControl.mNtagDetectStatus &= ~NTAG_PRESENCE_CHECK_TIMER_STATUS;
    stopTimer = true;
  }

  if (stopTimer) QPollTimerStop();
}

void NxpNTag::processNTagDetectNtf(const std::vector<uint8_t>& uid) {
  mNtagControl.mNtagUid = uid;
  std::vector<uint8_t> ntagDetectNtf = {
      static_cast<uint8_t>(NCI_MT_NTF | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL,
      static_cast<uint8_t>(PAYLOAD_TWO_LEN + uid.size()),
      static_cast<uint8_t>(QTAG_FEATURE_SUB_GID | NTAG_DETECTION_OID),
      NTAG_STATUS_SUCCESS};

  ntagDetectNtf.insert(ntagDetectNtf.end(), uid.begin(), uid.end());
  phNxpHal_NfcDataCallback(ntagDetectNtf.size(), ntagDetectNtf.data());
}

bool NxpNTag::isNTagReadComplete(const std::vector<uint8_t>& uid) {
  bool isSameUid = (mNtagControl.mNtagUid == uid);

  if (isSameUid && (mNtagControl.mNtagDetectStatus &
                    (NTAG_READ_COMPLETE | NTAG_PRESENCE_CHK_STATUS))) {
    mNtagControl.mQPOLLMode = NFC_RF_DISC_START;
    mNtagControl.mNtagDetectStatus &= ~NTAG_ACTIVATED_STATUS;
    updateState(NTagState::NTAG_STATE_SAME_UID_DETECTED);
    NFCSTATUS status = processNTagEvent(NTagEvent::ACTION_NTAG_UID_MATCHED);
    if (status != NFCSTATUS_SUCCESS)
      NXPLOG_NCIHAL_E("NxpNTag::%s Failed to trigger RF discovery", __func__);

    return true;
  }

  if (!isSameUid) processNTagDetectNtf(uid);

  return false;
}

NFCSTATUS NxpNTag::handleRfIntfActivated(uint8_t* pData, uint16_t dataLen) {
  constexpr uint8_t TECH_TYPE_INDEX = 6;
  constexpr uint8_t RF_INTF_ACT_LEN = 0x1E;
  constexpr uint8_t NCI_TECH_A_POLL_VAL = 0x00;
  constexpr uint8_t NCI_RF_INTF_ACT_AID_LEN_INDEX = 12;
  constexpr uint8_t NTAG_UID_START_INDEX = 13;

  // Validate input length and expected values
  if (dataLen <= RF_INTF_ACT_LEN ||
      pData[NCI_MSG_LEN_INDEX] != RF_INTF_ACT_LEN ||
      pData[TECH_TYPE_INDEX] != NCI_TECH_Q_POLL_VAL) {
    if (mNtagControl.mNtagDetectStatus & NTAG_DETECT_TIMER_STATUS)
      mNtagControl.mNtagDetectStatus &=
          (NTAG_DETECT_TIMER_STATUS | NTAG_READ_COMPLETE);
    else
      mNtagControl.mNtagDetectStatus &= NTAG_READ_COMPLETE;

    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  // Modify the tech type to A Poll
  pData[TECH_TYPE_INDEX] = NCI_TECH_A_POLL_VAL;

  uint8_t uidLength = pData[NCI_RF_INTF_ACT_AID_LEN_INDEX];

  // Validate UID length
  if (dataLen <= (NTAG_UID_START_INDEX + uidLength)) {
    NXPLOG_NCIHAL_E("NxpNTag::%s Invalid RF Interface Notification", __func__);
    mNtagControl.mNtagDetectStatus = 0x00;
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  // Extract UID directly from pData
  std::vector<uint8_t> extractedUid(pData + NTAG_UID_START_INDEX,
                                    pData + NTAG_UID_START_INDEX + uidLength);

  // Update tag control status
  if ((mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS) ==
      NTAG_REMOVAL_STATUS)
    mNtagControl.mNtagDetectStatus &= ~NTAG_REMOVAL_STATUS;

  mNtagControl.mQPOLLMode = 0x00;
  stopNTagTimerInternal();
  mNtagControl.mNtagDetectStatus |= NTAG_ACTIVATED_STATUS;

  // Check if UID has changed
  if (isNTagReadComplete(extractedUid)) return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

void NxpNTag::updateNTagRemoveStatus() {
  static constexpr uint8_t NTAG_MODE_REMOVED_STATUS[] = {
      (NCI_MT_NTF | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      (QTAG_FEATURE_SUB_GID | NTAG_REMOVED_STATUS_OID), NTAG_STATUS_FAILED};

  // Notify removal and reset NTAG state
  phNxpHal_NfcDataCallback(sizeof(NTAG_MODE_REMOVED_STATUS),
                           NTAG_MODE_REMOVED_STATUS);
  mNtagControl.mNtagUid.clear();
  mNtagControl.mNtagDetectStatus = NTAG_REMOVAL_STATUS;
}

void NxpNTag::checkNTagRemoveStatus() {
  // Transition presence check timer to timeout if active
  if (mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHECK_TIMER_STATUS) {
    // Transition presence check timer to timeout status
    mNtagControl.mNtagDetectStatus &= ~NTAG_PRESENCE_CHECK_TIMER_STATUS;
    mNtagControl.mNtagDetectStatus |= NTAG_PRESENCE_CHECK_TIMEOUT;
  }

  mNtagControl.mNTagTimer.kill();

  // Proceed only if tag is not activated and detection timer is active
  if ((mNtagControl.mNtagDetectStatus & NTAG_ACTIVATED_STATUS) ||
      !(mNtagControl.mNtagDetectStatus & NTAG_DETECT_TIMER_STATUS))
    return;

  if ((mNtagControl.mNtagDetectStatus & NTAG_DETECT_TIMER_STATUS) ==
      NTAG_DETECT_TIMER_STATUS)
    mNtagControl.mNtagDetectStatus &= ~NTAG_DETECT_TIMER_STATUS;

  if (!mNtagControl.mNtagUid.empty() &&
      (mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS) !=
          NTAG_REMOVAL_STATUS) {
    NXPLOG_NCIHAL_E("NxpNTag::%s: NTAG removed and clearing the status",
                    __func__);
    updateNTagRemoveStatus();
  }

  mNtagControl.mQPOLLMode = NFC_RF_DISC_START;
  mWaitingforDiscRsp = true;
  NFCSTATUS status = processNTagEvent(NTagEvent::ACTION_NTAG_REMOVAL_DETECTED);
  waitForRfDiscRsp(status);
}

void NxpNTag::QPollTimerTimeoutCallback(union sigval val) {
  NXPLOG_NCIHAL_D("NxpNTag::%s Timer expired...", __func__);
  NxpNTag::getInstance()->checkNTagRemoveStatus();
}

bool NxpNTag::QPollTimerStart(unsigned long sec) {
  NXPLOG_NCIHAL_D("%s Starting %ld milliseconds timer", __func__, sec * 1000);
  if (!mNtagControl.mNTagTimer.set((sec * 1000), NULL,
                                   QPollTimerTimeoutCallback)) {
    NXPLOG_NCIHAL_E("%s Timer start failed", __func__);
    return false;
  }
  return true;
}

void NxpNTag::QPollTimerStop() {
  NXPLOG_NCIHAL_D("NxpNTag::%s  Stopping timer...", __func__);
  mNtagControl.mNTagTimer.kill();
}
