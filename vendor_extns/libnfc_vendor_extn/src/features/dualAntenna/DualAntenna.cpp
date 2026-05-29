
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

#include "DualAntenna.h"

using std::vector;

DualAntenna::DualAntenna() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mDualAntennaContext.mDualAntennaFeature = false;
  mDualAntennaContext.mDualAntennaRequest = false;
  mDualAntennaContext.mDualAntennaSetting = false;
  mDualAntennaContext.mAppendQpoll = false;
  mDualAntennaContext.mAntennaFeature = 0x00;
  mDualAntennaContext.mAntOneConfig = 0x00;
  mDualAntennaContext.mAntTwoConfig = 0x00;
  mDualAntennaContext.mConfigReaderMode = 0x01;
}

DualAntenna::~DualAntenna() {}

std::unique_ptr<DualAntenna> DualAntenna::mDualAntenna = nullptr;

DualAntenna *DualAntenna::getInstance() {
  if (!mDualAntenna) {
    mDualAntenna = std::unique_ptr<DualAntenna>(new DualAntenna());
  }
  return mDualAntenna.get();
}

NFCSTATUS DualAntenna::handleVendorNciMessage(uint16_t dataLen,
                                              uint8_t *pData) {
  NFCSTATUS status = NFCSTATUS_EXTN_FEATURE_FAILURE;
  uint8_t num = 0;

  if ((mDualAntennaContext.mDualAntennaFeature == true) &&
      ((dataLen == CON_DISC_PARAM_LENGTH) && pData[NCI_GID_INDEX] == 0x20 &&
       pData[NCI_OID_INDEX] == 0x02 && pData[NCI_MSG_LEN_INDEX] == 0x04 &&
       pData[NCI_MSG_INDEX_FEATURE_VALUE] == 0x02)) {
    if (pData[READER_MODE_CONFIG_INDEX] == 0x01) {
      return sendConDiscParamCmd();
    }
  }

  if ((mDualAntennaContext.mDualAntennaFeature == true) &&
    (pData[NCI_GID_INDEX] == 0x21 && pData[NCI_OID_INDEX] == 0x03)) {
    if (NFCSTATUS_SUCCESS == sendRfDiscCmd()) {
      NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN,
                     "DualAntenna::%s handled Vendor Nci Message", __func__);
      return NFCSTATUS_EXTN_FEATURE_SUCCESS;
    } else {
      return NFCSTATUS_EXTN_FEATURE_FAILURE;
    }
  }

  if ((dataLen < CMD_MIN_DATA_LENGTH) ||
      (pData[NCI_OID_INDEX] != NCI_ROW_PROP_OID_VAL) ||
      ((pData[NCI_MSG_INDEX_FOR_FEATURE] & DUAL_ANTENNA_SUB_GID_OID) !=
       DUAL_ANTENNA_SUB_GID_OID) ||
      (mDualAntennaContext.mDualAntennaFeature != true)) {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  switch (pData[DUAL_ANTENNA_SUB_GID_OID_INDEX]) {
  case DUAL_ANTENNA_IS_SUPPORTED: {
    uint8_t num = 0;
    status = ((!(PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
                 NAME_NXP_DUAL_ANTENNA_SUPPORTED, &num, sizeof(num)))))
                 ? NFCSTATUS_FAILED
                 : NFCSTATUS_SUCCESS;
    break;
  }

  case DUAL_ANTENNA_SET_DISCOVERY: {
    mDualAntennaContext.mAntOneConfig =
        pData[DUAL_ANTENNA_ANTENNA_ONE_CONF_INDEX];
    mDualAntennaContext.mAntTwoConfig =
        pData[DUAL_ANTENNA_ANTENNA_TWO_CONF_INDEX];
    mDualAntennaContext.mDualAntennaRequest = true;
    mDualAntennaContext.mAntennaFeature = DUAL_ANTENNA_SET_DISCOVERY;
    if ((mDualAntennaContext.mAntOneConfig == DUAL_ANTENNA_NFC_PASSIVE_ABFKQ ||
         mDualAntennaContext.mAntOneConfig == DUAL_ANTENNA_NFC_PASSIVE_Q) ||
        (mDualAntennaContext.mAntTwoConfig == DUAL_ANTENNA_NFC_PASSIVE_ABFKQ ||
         mDualAntennaContext.mAntTwoConfig == DUAL_ANTENNA_NFC_PASSIVE_Q)) {
      mDualAntennaContext.mAppendQpoll = true;
    } else {
      mDualAntennaContext.mAppendQpoll = false;
    }
    status = sendRfDeactivate(pData);
    break;
  }

  case DUAL_ANTENNA_SET_READER_MODE: {
    if (PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
        NAME_NXP_DUAL_ANTENNA_FOLDABLE, &num, sizeof(num))) {
      if (num != 0x00 && (num | DUAL_ANTENNA_FOLDABLE) == DUAL_ANTENNA_FOLDABLE) {
        mDualAntennaContext.mDualAntennaRequest = true;
        mDualAntennaContext.mConfigReaderMode =
          pData[DUAL_ANTENNA_PHONE_STATE_INDEX];
        mDualAntennaContext.mAntennaFeature = DUAL_ANTENNA_SET_READER_MODE;
        status = sendRfDeactivate(pData);
      }
    } else {
      status = NFCSTATUS_FAILED;
    }
    break;
  }

  case DUAL_ANTENNA_GET_DISCOVERY_TECH: {

    uint8_t GET_DISCOVERY_TECH_STATUS_RSP[] = {
    (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL,
    DUAL_ANTENNA_TWO_GET_DISC_PAYLOAD_LEN, pData[DUAL_ANTENNA_SUB_GID_OID_INDEX],
     mDualAntennaContext.mAntOneConfig, mDualAntennaContext.mAntTwoConfig};

    PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
      sizeof(GET_DISCOVERY_TECH_STATUS_RSP), GET_DISCOVERY_TECH_STATUS_RSP);
    NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "DualAntenna::%s handled Vendor Nci Message", __func__);
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  case DUAL_ANTENNA_GET_READER_MODE: {

    uint8_t GET_READER_MODE_STATUS_RSP[] = {
    (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL,
    DUAL_ANTENNA_TWO_PAYLOAD_LEN, pData[DUAL_ANTENNA_SUB_GID_OID_INDEX],
     mDualAntennaContext.mConfigReaderMode};

    PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
      sizeof(GET_READER_MODE_STATUS_RSP), GET_READER_MODE_STATUS_RSP);
    NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "DualAntenna::%s handled Vendor Nci Message", __func__);
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  default: {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }
  }
  uint8_t DUAL_ANTENNA_STATUS_RSP[] = {
      (NCI_MT_RSP | NCI_GID_PROP), NCI_ROW_PROP_OID_VAL,
      DUAL_ANTENNA_TWO_PAYLOAD_LEN, pData[DUAL_ANTENNA_SUB_GID_OID_INDEX],
      (status == NFCSTATUS_SUCCESS) ? RESPONSE_STATUS_OK
                                    : RESPONSE_STATUS_FAILED};

  PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
      sizeof(DUAL_ANTENNA_STATUS_RSP), DUAL_ANTENNA_STATUS_RSP);
  if (status != NFCSTATUS_SUCCESS)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN,
                 "DualAntenna::%s handled Vendor Nci Message", __func__);
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

bool DualAntenna::isValidDualAntennaNtf(uint16_t dataLen, uint8_t* pData) {
  if (pData == nullptr) {
    return false;
  }

  constexpr uint8_t NCI_GENERIC_INFO_NTF_GID = 0x6F;
  constexpr uint8_t NCI_GENERIC_INFO_NTF_OID = 0x0F;
  constexpr uint8_t NCI_GENERIC_INFO_NTF_MSG_LEN = 0x04;
  constexpr uint8_t NCI_GENERIC_INFO_NTF_FEATURE_ID = 0x01;
  constexpr uint8_t NCI_GENERIC_INFO_NTF_FEATURE_VALUE = 0x90;
  constexpr uint8_t NCI_GENERIC_INFO_NTF_LEN = 0x07;

  if ((mDualAntennaContext.mDualAntennaFeature != true) ||
      (dataLen != NCI_GENERIC_INFO_NTF_LEN) ||
      (pData[NCI_GID_INDEX] != NCI_GENERIC_INFO_NTF_GID) ||
      (pData[NCI_OID_INDEX] != NCI_GENERIC_INFO_NTF_OID) ||
      (pData[NCI_MSG_LEN_INDEX] != NCI_GENERIC_INFO_NTF_MSG_LEN) ||
      (pData[NCI_MSG_INDEX_FOR_FEATURE] != NCI_GENERIC_INFO_NTF_FEATURE_ID) ||
      (pData[NCI_MSG_INDEX_FEATURE_VALUE] !=
       NCI_GENERIC_INFO_NTF_FEATURE_VALUE))
    return false;

  std::vector<uint8_t> NCI_GENERIC_INFO_NTF;
  NCI_GENERIC_INFO_NTF.insert(NCI_GENERIC_INFO_NTF.end(), pData,
                              pData + dataLen);
  std::vector<uint8_t> antennaDetectNtf;
  antennaDetectNtf.push_back(NCI_MT_RSP | NCI_GID_PROP);
  antennaDetectNtf.push_back(NCI_ROW_PROP_OID_VAL);
  antennaDetectNtf.push_back(DUAL_ANTENNA_TWO_PAYLOAD_LEN);
  antennaDetectNtf.push_back(DUAL_ANTENNA_SET_DISCOVERY);
  antennaDetectNtf.insert(antennaDetectNtf.end(), NCI_GENERIC_INFO_NTF.begin(),
                          NCI_GENERIC_INFO_NTF.end());
  PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
      antennaDetectNtf.size(), antennaDetectNtf.data());
  return true;
}

NFCSTATUS DualAntenna::handleVendorNciRspNtf(uint16_t dataLen, uint8_t *pData) {
  NFCSTATUS status = NFCSTATUS_FAILED;

  if (isValidDualAntennaNtf(dataLen, pData)) {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  if ((dataLen < CMD_MIN_DATA_LENGTH) ||
      (mDualAntennaContext.mDualAntennaRequest != true) ||
      ((pData[NCI_GID_INDEX] & NCI_MT_MASK) != NCI_MT_RSP)) {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }

  switch (pData[NCI_OID_INDEX] & NCI_OID_MASK) {
  case NCI_MSG_RF_DEACTIVATE: {
    switch (mDualAntennaContext.mAntennaFeature) {
    case DUAL_ANTENNA_ENABLE:
    case DUAL_ANTENNA_DISABLE: {
      status =
          applyDualAntennaSettings(mDualAntennaContext.mDualAntennaSetting);
      break;
    }
    case DUAL_ANTENNA_SET_DISCOVERY: {
      status = setDiscoveryTechnology_DualAntenna();
      break;
    }
    case DUAL_ANTENNA_SET_READER_MODE: {
      status = setDualAntennaPollMode();
      break;
    }
    default: {
      return NFCSTATUS_EXTN_FEATURE_FAILURE;
    }
    }
    if (status != NFCSTATUS_SUCCESS)
      return NFCSTATUS_EXTN_FEATURE_FAILURE;
    break;
  }
  case NCI_MSG_RF_DISCOVER: {
    mDualAntennaContext.mDualAntennaRequest = false;
    mDualAntennaContext.mDualAntennaSetting = false;
    mDualAntennaContext.mAntennaFeature = 0x00;
    NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "DualAntenna::%s handled NciRspNtf", __func__);
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }
  case NCI_MSG_CORE_SET_CONFIG: {
    status = sendRfDiscCmd();
    if (status != NFCSTATUS_SUCCESS)
      return NFCSTATUS_EXTN_FEATURE_FAILURE;
    break;
  }
  default: {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }
  }

  NXPLOG_EXTNS_I(NXPLOG_ITEM_NXP_GEN_EXTN, "DualAntenna::%s handled NciRspNtf",
                 __func__);
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

NFCSTATUS DualAntenna::applyDualAntennaSettings(bool feature) {
  vector<uint8_t> cmd =
      feature ? vector<uint8_t>{0x20, 0x02, 0x09, 0x01, 0xA1, 0xE0,
                                0x05, 0x02, 0x07, 0x07, 0x00, 0x00}
              : vector<uint8_t>{0x20, 0x02, 0x09, 0x01, 0xA1, 0XE0,
                                0x05, 0x00, 0x00, 0x00, 0x00, 0x00};

  return PlatformAbstractionLayer::getInstance()->palenQueueWrite(cmd.data(),
                                                                  cmd.size());
}

NFCSTATUS DualAntenna::setDiscoveryTechnology_DualAntenna() {
  constexpr uint8_t CONFIGURE_ONE_ANTENNA = 0x01;
  constexpr uint8_t DISABLE_DUAL_ANTENNA = 0x00;
  constexpr uint8_t ANT_CONFIG_INDEX = 7;
  constexpr uint8_t ANT_ONE_CONFIG_INDEX = 8;
  constexpr uint8_t ANT_TWO_CONFIG_INDEX = 9;
  vector<uint8_t> cmd_dual_antenna_set_discovery = {
      0x20, 0x02, 0x09, 0x01, 0xA1, 0XE0, 0x05, 0x02, 0x07, 0x07, 0x00, 0x00};

  if (mDualAntennaContext.mAntOneConfig == 0x00 &&
      mDualAntennaContext.mAntTwoConfig == 0x00) {
    cmd_dual_antenna_set_discovery[ANT_CONFIG_INDEX] =
        DISABLE_DUAL_ANTENNA; // Disable dual antenna feature
  } else if (mDualAntennaContext.mAntTwoConfig == 0x00) {
    cmd_dual_antenna_set_discovery[ANT_CONFIG_INDEX] =
        CONFIGURE_ONE_ANTENNA; // changing one antenna configuration
  }
  cmd_dual_antenna_set_discovery[ANT_ONE_CONFIG_INDEX] =
      mDualAntennaContext.mAntOneConfig;
  cmd_dual_antenna_set_discovery[ANT_TWO_CONFIG_INDEX] =
      mDualAntennaContext.mAntTwoConfig;
  return PlatformAbstractionLayer::getInstance()->palenQueueWrite(
      cmd_dual_antenna_set_discovery.data(),
      cmd_dual_antenna_set_discovery.size());
}

bool DualAntenna::isDualAntennaSupported() {
  NFCSTATUS status = NFCSTATUS_FAILED;
  uint8_t num = 0;
  if (!PlatformAbstractionLayer::getInstance()->palGetNxpNumValue(
          NAME_NXP_DUAL_ANTENNA_SUPPORTED, &num, sizeof(num)))
    return false;
  if (num == DUAL_ANTENNA_FEATURE_SUPPORTED) {
    mDualAntennaContext.mDualAntennaFeature = true;
  } else {
    mDualAntennaContext.mDualAntennaFeature = false;
  }
  mDualAntennaContext.mDualAntennaSetting =
      (num == DUAL_ANTENNA_FEATURE_SUPPORTED);
  mDualAntennaContext.mAntennaFeature = (num == DUAL_ANTENNA_FEATURE_SUPPORTED)
                                            ? DUAL_ANTENNA_ENABLE
                                            : DUAL_ANTENNA_DISABLE;
  if (NfcRfState::IDLE == RfStateMonitor::getInstance()->getNfcRfState()) {
    status = applyDualAntennaSettings(mDualAntennaContext.mDualAntennaSetting);
  } else {
    vector<uint8_t> DISABLE_DISC_CMD = {0x21, 0x06, 0x01, 0x00};
    status = PlatformAbstractionLayer::getInstance()->palenQueueWrite(
        DISABLE_DISC_CMD.data(), DISABLE_DISC_CMD.size());
  }
  if (NFCSTATUS_SUCCESS == status)
    return true;

  return false;
}

NFCSTATUS DualAntenna::setDualAntennaPollMode() {
  vector<uint8_t> cmd_dual_antenna_set_polling = {0x20, 0x02, 0x04, 0x01,
                                                  0x02, 0X01, 0x01};
  cmd_dual_antenna_set_polling[READER_MODE_CONFIG_INDEX] =
      mDualAntennaContext.mConfigReaderMode;
  return PlatformAbstractionLayer::getInstance()->palenQueueWrite(
      cmd_dual_antenna_set_polling.data(),
      cmd_dual_antenna_set_polling.size());
}

NFCSTATUS DualAntenna::sendRfDeactivate(const uint8_t *pData) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  if (NfcRfState::IDLE == RfStateMonitor::getInstance()->getNfcRfState()) {
    switch (mDualAntennaContext.mAntennaFeature) {
    case DUAL_ANTENNA_SET_DISCOVERY: {
      status = setDiscoveryTechnology_DualAntenna();
      break;
    }
    case DUAL_ANTENNA_SET_READER_MODE: {
      status = setDualAntennaPollMode();
      break;
    }
    default: {
      return NFCSTATUS_FAILED;
    }
    }
    return status;
  }
  vector<uint8_t> DISABLE_DISC_CMD = {0x21, 0x06, 0x01, 0x00};
  return PlatformAbstractionLayer::getInstance()->palenQueueWrite(
      DISABLE_DISC_CMD.data(), DISABLE_DISC_CMD.size());
}

vector<uint8_t> removeListenModes(std::vector<uint8_t> RF_DISC_CMD) {

  constexpr uint8_t NCI_RF_DISC_PAYLOAD_LEN_INDEX = 2;
  constexpr uint8_t NCI_RF_DISC_NUM_OF_CONFIG_INDEX = 3;
  constexpr uint8_t NCI_TYPE_A_LISTEN  = 0x80;
  constexpr uint8_t NCI_TYPE_B_LISTEN  = 0x81;
  constexpr uint8_t NCI_TYPE_F_LISTEN  = 0x82;

  uint8_t removedConfigs = 0;
  for (auto it = RF_DISC_CMD.begin(); it < RF_DISC_CMD.end() - 1;) {
    uint8_t tech = *it;
    bool isListenMode = (tech == NCI_TYPE_A_LISTEN ||
                         tech == NCI_TYPE_B_LISTEN ||
                         tech == NCI_TYPE_F_LISTEN);

    if (isListenMode) {
      it = RF_DISC_CMD.erase(it, it + 2);  // remove pair
      removedConfigs++;
    } else {
      it += 2;  // move to next entry
    }
  }

  // Update number of configurations
  RF_DISC_CMD[NCI_RF_DISC_NUM_OF_CONFIG_INDEX] -= removedConfigs;
  RF_DISC_CMD[NCI_RF_DISC_PAYLOAD_LEN_INDEX] = RF_DISC_CMD.size() - 3;

  return RF_DISC_CMD;  // return modified vector
}

NFCSTATUS DualAntenna::sendRfDiscCmd() {
  constexpr uint8_t NCI_RF_PAYLOAD_LEN = 2;
  constexpr uint8_t NCI_QTAG_PAYLOAD_LEN = 2;
  constexpr uint8_t NCI_RF_DISC_PAYLOAD_LEN_INDEX = 2;
  constexpr uint8_t NCI_RF_DISC_NUM_OF_CONFIG_INDEX = 3;
  constexpr uint8_t NCI_TECH_Q_POLL_VAL = 0x71;
  constexpr uint8_t QTAG_ENABLE_OID = 0x01;
  vector<uint8_t> RF_DISC_CMD_ONLY_QTAG = {0x21, 0x03, 0x03, 0x01, 0x71, 0x01};
  vector<uint8_t> RF_DISC_CMD =
      PlatformAbstractionLayer::getInstance()->palGetDiscoveryCommand();
  if (mDualAntennaContext.mAntOneConfig != 0x00 &&
      mDualAntennaContext.mAntTwoConfig != 0x00) {
    if ((mDualAntennaContext.mAntOneConfig &
         mDualAntennaContext.mAntTwoConfig) == DUAL_ANTENNA_NFC_PASSIVE_Q) {
      RF_DISC_CMD = RF_DISC_CMD_ONLY_QTAG;
      goto send_command;
    }
  } else {
    if ((mDualAntennaContext.mAntOneConfig |
         mDualAntennaContext.mAntTwoConfig) == DUAL_ANTENNA_NFC_PASSIVE_Q) {
      RF_DISC_CMD = RF_DISC_CMD_ONLY_QTAG;
      goto send_command;
    }
  }
  if (mDualAntennaContext.mAppendQpoll) {
    RF_DISC_CMD[NCI_RF_DISC_NUM_OF_CONFIG_INDEX]++;
    RF_DISC_CMD[NCI_RF_DISC_PAYLOAD_LEN_INDEX] += NCI_QTAG_PAYLOAD_LEN;
    RF_DISC_CMD.push_back(NCI_TECH_Q_POLL_VAL);
    RF_DISC_CMD.push_back(QTAG_ENABLE_OID);
  }

send_command:
  if (PlatformAbstractionLayer::getInstance()->palGetObserveModeStatus()) {
    RF_DISC_CMD[NCI_RF_DISC_NUM_OF_CONFIG_INDEX]++;
    RF_DISC_CMD[NCI_RF_DISC_PAYLOAD_LEN_INDEX] += NCI_RF_PAYLOAD_LEN;
    RF_DISC_CMD.push_back(0xFF);
    RF_DISC_CMD.push_back(0x01);
    RF_DISC_CMD = removeListenModes(RF_DISC_CMD);
  }
  return PlatformAbstractionLayer::getInstance()->palenQueueWrite(
      RF_DISC_CMD.data(), RF_DISC_CMD.size());
}

NFCSTATUS DualAntenna::sendConDiscParamCmd() {
  NFCSTATUS status = NFCSTATUS_FAILED;
  vector<uint8_t> cmd_con_disc_param = {0x20, 0x02, 0x04, 0x01,
                                        0x02, 0X01, 0x01};
  cmd_con_disc_param[READER_MODE_CONFIG_INDEX] =
      mDualAntennaContext.mConfigReaderMode;
  status = PlatformAbstractionLayer::getInstance()->palenQueueWrite(
      cmd_con_disc_param.data(), cmd_con_disc_param.size());
  if (status != NFCSTATUS_SUCCESS)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  NXPLOG_EXTNS_I(
      NXPLOG_ITEM_NXP_GEN_EXTN,
      "DualAntenna::handleVendorNciMessage handled Vendor Nci Message");
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}
