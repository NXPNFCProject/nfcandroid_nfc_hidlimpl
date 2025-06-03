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
#include "NciDiscoveryCommandBuilder.h"
#include "NxpNfcExtension.h"
#include "ObserveMode.h"

extern bool sendRspToUpperLayer;
extern phNxpNciHal_Control_t nxpncihal_ctrl;

NxpNTag::NxpNTag() {
  mNtagControl.mQPOLLMode = 0x00;
  mNtagControl.mNtagDetectStatus = 0x00;
  mNtagControl.mNtagUid.clear();
  mNtagControl.mCurrentDiscCmd.clear();
  mNtagControl.mNtagEnableRequest = false;
  mNtagControl.isNTagNtfCmdReq = false;
  mNtagControl.isNTagNtfEnabled = false;
}

NxpNTag::~NxpNTag() {}

NxpNTag& NxpNTag::getInstance() {
  static NxpNTag msNxpNTag;
  return msNxpNTag;
}

void NxpNTag::phNxpNciHal_disableNtagNtfConfig() {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  if (IS_CHIP_TYPE_EQ(sn300u)) {
    uint8_t getNtagNtfConfig[] = {0x20, 0x03, 0x03, 0x01, 0xA1, 0xDA};
    uint8_t setNtagNtfConfig[] = {0x20, 0x02, 0x05, 0x01,
                                  0xA1, 0xDA, 0x01, 0x00};
    status =
        phNxpNciHal_send_ext_cmd(sizeof(getNtagNtfConfig), getNtagNtfConfig);
    if ((status == NFCSTATUS_SUCCESS) &&
        (nxpncihal_ctrl.p_rx_data[8] == NTAG_NTF_ENABLE_STATE)) {
      mNtagControl.isNTagNtfEnabled = true;
      status =
          phNxpNciHal_send_ext_cmd(sizeof(setNtagNtfConfig), setNtagNtfConfig);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Ntag Set Config : Failed");
      }
    }
  }
}

NFCSTATUS NxpNTag::handleNciMessage(uint16_t dataLen, uint8_t* pData) {
  NFCSTATUS status;
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter ", __func__);
  uint8_t NTAG_NCI_VENDOR_RSP_SUCCESS[] = {
      (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      (QTAG_FEATURE_SUB_GID | QTAG_ENABLE_OID), NTAG_STATUS_SUCCESS};
  uint8_t NTAG_NCI_VENDOR_RSP_FAILURE[] = {
      (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      (QTAG_FEATURE_SUB_GID | QTAG_ENABLE_OID), NTAG_STATUS_FAILED};
  uint8_t NTAG_MODE_ENABLED_STATUS_SUCCESS[] = {
      (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      (QTAG_FEATURE_SUB_GID | NTAG_MODE_ENABLED_STATUS_OID),
      NTAG_STATUS_SUCCESS};
  uint8_t NTAG_MODE_ENABLED_STATUS_FAILURE[] = {
      (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      (QTAG_FEATURE_SUB_GID | NTAG_MODE_ENABLED_STATUS_OID),
      NTAG_STATUS_FAILED};

  if ((pData[0] == (NCI_MT_CMD | NCI_GID_PROP)) &&
      (pData[1] == NCI_ROW_PROP_OID_VAL) && (pData[2] == PAYLOAD_TWO_LEN) &&
      ((pData[3] == (QTAG_FEATURE_SUB_GID | QTAG_ENABLE_OID)) ||
       (pData[3] == (QTAG_FEATURE_SUB_GID | NTAG_MODE_ENABLED_STATUS_OID)))) {
    switch (pData[4]) {
      case NTAG_ENABLE_PROP_OID: {
        NXPLOG_NCIHAL_D("NxpNTag::%s enable", __func__);
        mNtagControl.mNtagEnableRequest = true;

        status = sendRfDeactivate();
        mNtagControl.mCurrentDiscCmd.clear();
        mNtagControl.mCurrentDiscCmd =
            NciDiscoveryCommandBuilderInstance.getDiscoveryCommand();
        mNtagControl.mQPOLLMode = NFC_RF_DISC_APPEND_QPOLL;
        if (status != NFCSTATUS_SUCCESS)
          phNxpHal_NfcDataCallback(sizeof(NTAG_NCI_VENDOR_RSP_FAILURE),
                                   NTAG_NCI_VENDOR_RSP_FAILURE);
        else
          phNxpHal_NfcDataCallback(sizeof(NTAG_NCI_VENDOR_RSP_SUCCESS),
                                   NTAG_NCI_VENDOR_RSP_SUCCESS);

        return NFCSTATUS_EXTN_FEATURE_SUCCESS;
        break;
      }
      case NTAG_DISABLE_PROP_OID: {
        NXPLOG_NCIHAL_D("NxpNTag::%s Disable", __func__);
        mNtagControl.mNtagEnableRequest = false;

        status = sendRfDeactivate();
        mNtagControl.mQPOLLMode = NFC_RF_DISC_START;
        mNtagControl.mNtagDetectStatus = 0x00;

        if (status != NFCSTATUS_SUCCESS)
          phNxpHal_NfcDataCallback(sizeof(NTAG_NCI_VENDOR_RSP_FAILURE),
                                   NTAG_NCI_VENDOR_RSP_FAILURE);
        else
          phNxpHal_NfcDataCallback(sizeof(NTAG_NCI_VENDOR_RSP_SUCCESS),
                                   NTAG_NCI_VENDOR_RSP_SUCCESS);

        return NFCSTATUS_EXTN_FEATURE_SUCCESS;
        break;
      }
      case NTAG_MODE_ENABLED_STATUS_OID: {
        if (!mNtagControl.mNtagEnableRequest)
          phNxpHal_NfcDataCallback(sizeof(NTAG_MODE_ENABLED_STATUS_FAILURE),
                                   NTAG_MODE_ENABLED_STATUS_FAILURE);
        else
          phNxpHal_NfcDataCallback(sizeof(NTAG_MODE_ENABLED_STATUS_SUCCESS),
                                   NTAG_MODE_ENABLED_STATUS_SUCCESS);

        return NFCSTATUS_EXTN_FEATURE_SUCCESS;
        break;
      }
    }
  }
  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

NFCSTATUS NxpNTag::phNxpNciHal_HandleNtagRspNtf(uint16_t dataLen,
                                                uint8_t* pData) {
  uint8_t NTAG_NCI_VENDOR_RSP_SUCCESS[] = {
      (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      (QTAG_FEATURE_SUB_GID | QTAG_ENABLE_OID), NTAG_STATUS_SUCCESS};
  uint8_t NCI_RF_INTF_ACT_TECH_TYPE_INDEX = 6;
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);

  if (mNtagControl.mNtagEnableRequest != mNtagControl.isNTagNtfEnabled)
    mNtagControl.isNTagNtfCmdReq = true;

  if (!mNtagControl.mNtagEnableRequest && !mNtagControl.isNTagNtfCmdReq)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  uint8_t mHeader = pData[0] & (NCI_MT_MASK | NCI_GID_MASK);
  bool isValidHeader = ((mHeader == (NCI_MT_RSP | NCI_GID_RF_MANAGE)) ||
                        (mHeader == (NCI_MT_NTF | NCI_GID_RF_MANAGE)) ||
                        (mHeader == (NCI_MT_NTF | NCI_GID_PROP)));

  if (!isValidHeader && !mNtagControl.isNTagNtfCmdReq)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  switch (pData[0] & NCI_MT_MASK) {
    case NCI_MT_RSP: {
      switch (pData[1] & NCI_OID_MASK) {
        case NCI_MSG_RF_DISCOVER:
          switch (mNtagControl.mQPOLLMode) {
            case NFC_RF_DISC_RESTART: {
              if (((mNtagControl.mNtagDetectStatus &
                    (NTAG_READ_COMPLETE | NTAG_PRESENCE_CHK_STATUS)) != 0)) {
                vector<uint8_t> rfDeact_Ntf = {0x61, 0x06, 0x02, 0x03, 0x00};
                phNxpHal_NfcDataCallback(rfDeact_Ntf.size(), &rfDeact_Ntf[0]);
                mNtagControl.mQPOLLMode = 0x00;
              }
              break;
            }
            case NFC_RF_DISC_START:
            case NFC_RF_DISC_APPEND_QPOLL: {
              mNtagControl.mQPOLLMode = 0x00;
              mNtagControl.isNTagNtfCmdReq = false;
              break;
            }
          }
          break;
        case NCI_MSG_RF_DEACTIVATE:
          if (mNtagControl.mQPOLLMode == NFC_RF_DISC_APPEND_QPOLL ||
              mNtagControl.mQPOLLMode == NFC_RF_DISC_START ||
              mNtagControl.mQPOLLMode == NFC_RF_DISC_REPLACE_QPOLL) {
            if (mNtagControl.isNTagNtfCmdReq)
              sendNtagPropConfig(mNtagControl.mNtagEnableRequest);
            else
              sendRfDiscCmd(mNtagControl.mQPOLLMode);

            return NFCSTATUS_EXTN_FEATURE_SUCCESS;
          } else if ((mNtagControl.mQPOLLMode == NFC_RF_DISC_RESTART) &&
                     ((mNtagControl.mNtagDetectStatus &
                       (NTAG_READ_COMPLETE | NTAG_PRESENCE_CHK_STATUS)) != 0)) {
            vector<uint8_t> rfDeAct_Rsp = {0x41, 0x06, 0x01, 0x00};
            phNxpHal_NfcDataCallback(rfDeAct_Rsp.size(), &rfDeAct_Rsp[0]);
            sendRfDiscCmd(mNtagControl.mQPOLLMode);
            return NFCSTATUS_EXTN_FEATURE_SUCCESS;
          }
          break;
        case NCI_MSG_CORE_SET_CONFIG:
          if (mNtagControl.mQPOLLMode == NFC_RF_DISC_APPEND_QPOLL ||
              mNtagControl.mQPOLLMode == NFC_RF_DISC_START) {
            sendRfDiscCmd(mNtagControl.mQPOLLMode);
          }
          break;

        case NCI_MSG_RF_ISO_DEP_NAK_PRESENCE:
          if ((mNtagControl.mNtagDetectStatus &
               NTAG_PRESENCE_CHECK_TIMER_STATUS) !=
              NTAG_PRESENCE_CHECK_TIMER_STATUS) {
            unsigned long timeout = NTAG_PRESENCE_CHECK_DEFAULT_CONF_VAL;
            if (GetNxpNumValue(NAME_NXP_NTAG_PRESENCE_CHECK_TIMEOUT,
                               (void*)&timeout, sizeof(timeout))) {
              NXPLOG_NCIHAL_D("NxpNTag::%s ntag delay conf val  %lx", __func__,
                              timeout);
            }
            if ((mNtagControl.mNtagDetectStatus &
                 NTAG_PRESENCE_CHECK_TIMEOUT) != NTAG_PRESENCE_CHECK_TIMEOUT) {
              QPollTimerStart(timeout);  // Presnce checktimer
            }
            mNtagControl.mNtagDetectStatus |= NTAG_PRESENCE_CHECK_TIMER_STATUS;
          }
          mNtagControl.mNtagDetectStatus |= NTAG_PRESENCE_CHK_STATUS;
          break;
      }
    } break;
    case NCI_MT_NTF: {
      switch (pData[1] & NCI_OID_MASK) {
        case NCI_MSG_RF_INTF_ACTIVATED:
          if ((pData[2] == 0x1E) &&
              (pData[NCI_RF_INTF_ACT_TECH_TYPE_INDEX] == NCI_TECH_Q_POLL_VAL)) {
            pData[NCI_RF_INTF_ACT_TECH_TYPE_INDEX] = NCI_TECH_A_POLL_VAL;

            vector<uint8_t> rfIntfNtf(pData, pData + dataLen);
            if (processNtagUid(rfIntfNtf)) {
              // Early exit if NTAG read complete or presence check status is
              // set
              if ((mNtagControl.mNtagDetectStatus &
                   (NTAG_READ_COMPLETE | NTAG_PRESENCE_CHK_STATUS)) != 0) {
                mNtagControl.mQPOLLMode = NFC_RF_DISC_START;
                mNtagControl.mNtagDetectStatus &= ~NTAG_ACTIVATED_STATUS;
                NxpNTagInstance.sendRfDeactivate();
                return NFCSTATUS_EXTN_FEATURE_SUCCESS;
              }
            }
          } else {
            // Clear timer if any running.
            mNtagControl.mNtagDetectStatus = 0x00;
            return NFCSTATUS_EXTN_FEATURE_FAILURE;
          }
          break;
        case NCI_MSG_RF_DEACTIVATE:
          if (((mNtagControl.mNtagDetectStatus & NTAG_ACTIVATED_STATUS) !=
               NTAG_ACTIVATED_STATUS) ||
              (mNtagControl.mQPOLLMode != NFC_RF_DISC_RESTART))
            return NFCSTATUS_EXTN_FEATURE_FAILURE;

          if (mNtagControl.mQPOLLMode == NFC_RF_DISC_RESTART)
            return NFCSTATUS_EXTN_FEATURE_SUCCESS;
          break;
        case NCI_MSG_RF_ISO_DEP_NAK_PRESENCE:
          if ((mNtagControl.mNtagDetectStatus & NTAG_ACTIVATED_STATUS) !=
              NTAG_ACTIVATED_STATUS)
            return NFCSTATUS_EXTN_FEATURE_FAILURE;
          // Handle Presence Check Notification
          if (pData[3] == NCI_MSG_RF_DISCOVER) {
            updateNTagRemoveStatus();
            mNtagControl.mQPOLLMode = 0x00;
          } else if ((mNtagControl.mNtagDetectStatus &
                      NTAG_PRESENCE_CHECK_TIMEOUT) ==
                         NTAG_PRESENCE_CHECK_TIMEOUT &&
                     (mNtagControl.mNtagDetectStatus &
                      NTAG_PRESENCE_CHK_STATUS) == NTAG_PRESENCE_CHK_STATUS) {
            mNtagControl.mNtagDetectStatus &= ~NTAG_PRESENCE_CHECK_TIMEOUT;
            pData[3] = NCI_MSG_RF_DISCOVER;
          }
          break;
        case NCI_MSG_RF_INTF_PROP_NTF:
          // Handle NTAG Property Notification
          uint8_t NTAG_LOAD_CHANGE_NTF_LEN = 7;
          uint8_t NTAG_LOAD_CHANGE_INDEX = 6;
          if (dataLen == NTAG_LOAD_CHANGE_NTF_LEN &&
              pData[NTAG_LOAD_CHANGE_INDEX] == NTAG_LOAD_CHANGE_VAL) {
            mNtagControl.mQPOLLMode = NFC_RF_DISC_APPEND_QPOLL;
            NxpNTagInstance.sendRfDeactivate();

            return NFCSTATUS_EXTN_FEATURE_SUCCESS;
          }
          break;
      }
    } break;
  }

  NXPLOG_NCIHAL_D("NxpNTag::%s Exit", __func__);
  phNxpNciHal_client_data_callback();
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

NFCSTATUS NxpNTag::processRfDiscCmd(vector<uint8_t>& rfDiscCmd) {
  uint8_t NCI_QTAG_PAYLOAD_LEN = 2;
  uint8_t NCI_RF_DISC_PAYLOAD_LEN_INDEX = 2;
  uint8_t NCI_RF_DISC_NUM_OF_CONFIG_INDEX = 3;
  NFCSTATUS status;
  bool sendRfIdleCmd = false;
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);

  if ((rfDiscCmd[0] & NCI_OID_MASK) == (NCI_MT_CMD | NCI_GID_RF_MANAGE)) {
    switch (rfDiscCmd[1] & NCI_GID_MASK) {
      case NCI_MSG_RF_DISCOVER:
        if (mNtagControl.mNtagUid.size() == 0) {
          vector<uint8_t> rfCmd = rfDiscCmd;
          rfCmd[NCI_RF_DISC_NUM_OF_CONFIG_INDEX]++;
          rfCmd[NCI_RF_DISC_PAYLOAD_LEN_INDEX] += NCI_QTAG_PAYLOAD_LEN;
          rfCmd.push_back(NCI_TECH_Q_POLL_VAL);
          rfCmd.push_back(QTAG_ENABLE_OID);
          status = phNxpNciHal_send_ext_cmd(rfCmd.size(), &rfCmd[0]);
          phNxpHal_NfcDataCallback(nxpncihal_ctrl.cmd_len,
                                   nxpncihal_ctrl.p_rx_data);
          mNtagControl.mQPOLLMode = 0x00;
          return NFCSTATUS_EXTN_FEATURE_SUCCESS;
        }
        break;
      case NCI_MSG_RF_DEACTIVATE:
        if (((mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHK_STATUS) ==
             NTAG_PRESENCE_CHK_STATUS) &&
            rfDiscCmd[3] == 0x03) {
          mNtagControl.mQPOLLMode = NFC_RF_DISC_RESTART;
          mNtagControl.mNtagDetectStatus |= NTAG_READ_COMPLETE;
          NXPLOG_NCIHAL_D("NxpNTag::%s RF Deactivate Discovery to idle",
                          __func__);
          sendRfIdleCmd = true;
        } else if (((mNtagControl.mNtagDetectStatus &
                     NTAG_PRESENCE_CHK_STATUS) == NTAG_PRESENCE_CHK_STATUS) &&
                   rfDiscCmd[3] == 0x01) {
          mNtagControl.mQPOLLMode = NFC_RF_DISC_RESTART;
          sendRfIdleCmd = true;
          NXPLOG_NCIHAL_D("NxpNTag::%s RF Deactivate Sleep to Discovery",
                          __func__);
        }
        break;
    }
  }

  if (sendRfIdleCmd) {
    sendRfDeactivate();
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

NFCSTATUS NxpNTag::phNxpNciHal_HandleNtagCmd(uint16_t dataLen, uint8_t* pData) {
  NFCSTATUS status;
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);

  if (pData[0] == (NCI_MT_CMD | NCI_GID_PROP)) {
    status = NxpNTagInstance.handleNciMessage(dataLen, (uint8_t*)pData);
    if (status == NFCSTATUS_EXTN_FEATURE_SUCCESS)
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  if ((!mNtagControl.mNtagEnableRequest) ||
      ((mNtagControl.mNtagDetectStatus & NTAG_ACTIVATED_STATUS) !=
       NTAG_ACTIVATED_STATUS))
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  vector<uint8_t> rfDiscCmd(pData, pData + dataLen);
  status = processRfDiscCmd(rfDiscCmd);
  if (status == NFCSTATUS_EXTN_FEATURE_SUCCESS)
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

NFCSTATUS NxpNTag::sendNtagPropConfig(bool flag) {
  vector<uint8_t> setPropNtfEnable = {0x20, 0x02, 0x05, 0x01,
                                      0xA1, 0xDA, 0x01, 0x01};
  NFCSTATUS status;
  NXPLOG_NCIHAL_D("NxpNTag::%s Flag: %d", __func__, flag);
  if (!flag) {
    setPropNtfEnable[7] = 0x00;
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

  vector<uint8_t> DISABLE_DISC_CMD = {0x21, 0x06, 0x01, 0x00};
  if (NFCSTATUS_SUCCESS ==
      phNxpHal_EnqueueWrite(&DISABLE_DISC_CMD[0], DISABLE_DISC_CMD.size()))
    return NFCSTATUS_SUCCESS;

  return NFCSTATUS_FAILED;
}

NFCSTATUS NxpNTag::sendRfDiscCmd(uint8_t state) {
  uint8_t NCI_NTAG_PAYLOAD_LEN = 2;
  uint8_t NCI_RF_DISC_PAYLOAD_LEN_INDEX = 2;
  uint8_t NCI_RF_DISC_NUM_OF_CONFIG_INDEX = 3;
  uint8_t NCI_TECH_Q_POLL_VAL = 0x71;
  uint8_t QTAG_ENABLE_OID = 0x01;
  NfcRfState_t rfState;
  vector<uint8_t> RF_DISC_CMD;
  vector<uint8_t> NTAG_RF_DISC_CMD = {0x21, 0x03, 0x03, 0x01, 0x71, 0x01};
  bool sendRfDiscCmdFlag = true;

  if (IDLE != phNxpExtn_NfcGetRfState()) {
    sendRfDeactivate();
    return NFCSTATUS_FAILED;
  }

  switch (state) {
    case NFC_RF_DISC_RESTART:
    case NFC_RF_DISC_START:
      RF_DISC_CMD = mNtagControl.mCurrentDiscCmd;
      break;
    case NFC_RF_DISC_APPEND_QPOLL:
      RF_DISC_CMD = mNtagControl.mCurrentDiscCmd;
      RF_DISC_CMD[NCI_RF_DISC_NUM_OF_CONFIG_INDEX]++;
      RF_DISC_CMD[NCI_RF_DISC_PAYLOAD_LEN_INDEX] += NCI_NTAG_PAYLOAD_LEN;
      RF_DISC_CMD.push_back(NCI_TECH_Q_POLL_VAL);
      RF_DISC_CMD.push_back(QTAG_ENABLE_OID);
      mNtagControl.mNtagDetectStatus &= ~NTAG_ACTIVATED_STATUS;
      if ((mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS) !=
          NTAG_REMOVAL_STATUS)
        QPollTimerStart(NTAG_DETECT_TIMER_VALUE);
      mNtagControl.mNtagDetectStatus |= NTAG_DETECT_TIMER_STATUS;
      break;
    case NFC_RF_DISC_REPLACE_QPOLL:
      RF_DISC_CMD = NTAG_RF_DISC_CMD;
      if (isObserveModeEnabled()) {
        RF_DISC_CMD[NCI_RF_DISC_NUM_OF_CONFIG_INDEX]++;
        RF_DISC_CMD[NCI_RF_DISC_PAYLOAD_LEN_INDEX] += NCI_NTAG_PAYLOAD_LEN;
        RF_DISC_CMD.push_back(0xFF);
        RF_DISC_CMD.push_back(0x01);
      }
      QPollTimerStart(NTAG_DETECT_TIMER_VALUE);
      mNtagControl.mNtagDetectStatus &= ~NTAG_ACTIVATED_STATUS;
      mNtagControl.mNtagDetectStatus |= NTAG_DETECT_TIMER_STATUS;
      break;
    default:
      sendRfDiscCmdFlag = false;
      break;
  }
  if (sendRfDiscCmdFlag) {
    if (NFCSTATUS_SUCCESS ==
        phNxpHal_EnqueueWrite(&RF_DISC_CMD[0], RF_DISC_CMD.size())) {
      return NFCSTATUS_SUCCESS;
    } else {
      QPollTimerStop();
    }
  }
  return NFCSTATUS_FAILED;
}

void NxpNTag::handleTimers() {
  // Stop NTAG detect and presence check timers if running
  if ((mNtagControl.mNtagDetectStatus & NTAG_DETECT_TIMER_STATUS) ==
      NTAG_DETECT_TIMER_STATUS) {
    mNtagControl.mNtagDetectStatus &=
        ~(NTAG_DETECT_TIMER_STATUS | NTAG_REPLACE_QPOLL_STATUS);
    QPollTimerStop();
  }
  if ((mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHECK_TIMER_STATUS) ==
      NTAG_PRESENCE_CHECK_TIMER_STATUS) {
    mNtagControl.mNtagDetectStatus &= ~NTAG_PRESENCE_CHECK_TIMER_STATUS;
    QPollTimerStop();
  }
}

bool NxpNTag::isNtagUidChanged(const uint8_t* uid, uint8_t uidLength) {
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);
  if (mNtagControl.mNtagUid.size() != uidLength ||
      !std::equal(uid, uid + uidLength, mNtagControl.mNtagUid.begin())) {
    NXPLOG_NCIHAL_E("NxpNTag::%s UID is not matched", __func__);
    return false;
  }
  return true;
}

bool NxpNTag::processNtagUid(const std::vector<uint8_t>& rfIntfNtf) {
  uint8_t SUB_GID_OID_LEN_INDEX = 2;
  uint8_t NCI_RF_INTF_ACT_AID_LEN_INDEX = 12;
  uint8_t uidLength = rfIntfNtf[NCI_RF_INTF_ACT_AID_LEN_INDEX];
  const uint8_t* extractedUid = rfIntfNtf.data() + 13;
  uint8_t NTAG_DETECTION_NTF_SUCCESS[] = {
      (NCI_MT_NTF | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      (QTAG_FEATURE_SUB_GID | NTAG_DETECTION_OID), NTAG_STATUS_SUCCESS};
  NXPLOG_NCIHAL_D("NxpNTag::%s Enter", __func__);

  if ((mNtagControl.mNtagDetectStatus & NTAG_REMOVAL_STATUS) ==
      NTAG_REMOVAL_STATUS)
    mNtagControl.mNtagDetectStatus &= ~NTAG_REMOVAL_STATUS;

  mNtagControl.mQPOLLMode = 0x00;
  handleTimers();
  mNtagControl.mNtagDetectStatus |= NTAG_ACTIVATED_STATUS;
  // If UID has changed, update and send the new UID
  if (!isNtagUidChanged(extractedUid, uidLength)) {
    mNtagControl.mNtagUid.assign(extractedUid, extractedUid + uidLength);
    // Create a new notification vector with the UID
    std::vector<uint8_t> NTAGDetectNtfWithUid = {
        NTAG_DETECTION_NTF_SUCCESS,
        NTAG_DETECTION_NTF_SUCCESS + sizeof(NTAG_DETECTION_NTF_SUCCESS)};
    NTAGDetectNtfWithUid[SUB_GID_OID_LEN_INDEX] +=
        rfIntfNtf[NCI_RF_INTF_ACT_AID_LEN_INDEX];
    NTAGDetectNtfWithUid.insert(NTAGDetectNtfWithUid.end(), extractedUid,
                                extractedUid + uidLength);
    // Send the NFC data notification with the UID
    phNxpHal_NfcDataCallback(NTAGDetectNtfWithUid.size(),
                             NTAGDetectNtfWithUid.data());

    return false;
  }
  return true;
}

void NxpNTag::updateNTagRemoveStatus() {
  uint8_t NTAG_MODE_REMOVED_STATUS[] = {
      (NCI_MT_NTF | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL, PAYLOAD_TWO_LEN,
      (QTAG_FEATURE_SUB_GID | NTAG_REMOVED_STATUS_OID), NTAG_STATUS_FAILED};

  // Send removal status and reset necessary flags
  phNxpHal_NfcDataCallback(sizeof(NTAG_MODE_REMOVED_STATUS),
                           NTAG_MODE_REMOVED_STATUS);
  mNtagControl.mNtagUid.clear();
  mNtagControl.mNtagDetectStatus = 0x00;  // Clear NTAG detected status
  mNtagControl.mNtagDetectStatus |= NTAG_REMOVAL_STATUS;
}

void NxpNTag::checkNTagRemoveStatus() {
  // Check if presence check timer is active
  if (mNtagControl.mNtagDetectStatus & NTAG_PRESENCE_CHECK_TIMER_STATUS) {
    // Transition presence check timer to timeout status
    mNtagControl.mNtagDetectStatus &= ~NTAG_PRESENCE_CHECK_TIMER_STATUS;
    mNtagControl.mNtagDetectStatus |= NTAG_PRESENCE_CHECK_TIMEOUT;
  }
  mNtagControl.gNTagTimer.kill();
  // Handle when NTag is not activated and detection timer is active
  if (!(mNtagControl.mNtagDetectStatus & NTAG_ACTIVATED_STATUS) &&
      (mNtagControl.mNtagDetectStatus & NTAG_DETECT_TIMER_STATUS)) {
    NXPLOG_NCIHAL_E("NxpNTag::%s: NTAG removed and clearing the status",
                    __func__);
    if (!mNtagControl.mNtagUid.empty()) {
      // If UID exists, handle check timer
      if (!(mNtagControl.mNtagDetectStatus & NTAG_REPLACE_QPOLL_STATUS)) {
        mNtagControl.mQPOLLMode = NFC_RF_DISC_REPLACE_QPOLL;
        mNtagControl.mNtagDetectStatus |= NTAG_REPLACE_QPOLL_STATUS;
      } else {
        // Send removal status and reset necessary flags
        updateNTagRemoveStatus();
        mNtagControl.mQPOLLMode = NFC_RF_DISC_APPEND_QPOLL;
      }
      NxpNTagInstance.sendRfDeactivate();
    }
  }
}

void NxpNTag::QPollTimerTimeoutCallback(union sigval val) {
  NXPLOG_NCIHAL_D("NxpNTag::%s Timer expired...", __func__);
  NxpNTagInstance.checkNTagRemoveStatus();
}

void NxpNTag::QPollTimerStart(unsigned long sec) {
  NXPLOG_NCIHAL_D("%s Starting %ld milliseconds timer", __func__, sec * 1000);
  if (!mNtagControl.gNTagTimer.set((sec * 1000), NULL,
                                   QPollTimerTimeoutCallback))
    NXPLOG_NCIHAL_E("%s Timer start failed", __func__);
}

void NxpNTag::QPollTimerStop() {
  NXPLOG_NCIHAL_D("NxpNTag::%s  Stopping timer...", __func__);
  mNtagControl.gNTagTimer.kill();
}
