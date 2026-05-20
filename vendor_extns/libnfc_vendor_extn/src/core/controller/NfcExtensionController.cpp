/**
 *
 *  Copyright 2024-2026 NXP
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

#include "NfcExtensionController.h"
#include <NciStateMonitor.h>
#include <phNxpLog.h>
#include "BroadcastFrameHandler.h"
#include "DualAntenna.h"
#include "NfcExtensionConstants.h"
#include "PlatformAbstractionLayer.h"
#include "Srd.h"
#include "phNxpAutoCard.h"
#include "phNxpNTag.h"

std::unique_ptr<NfcExtensionController>
    NfcExtensionController::sNfcExtensionController = nullptr;

NfcExtensionController::NfcExtensionController() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s: enter", __func__);
  mNfcHalState = {0};
  mCurrentHandlerType = {0};
  mCurrentHandlerState = HandlerState::STOPPED;
}

NfcExtensionController::~NfcExtensionController() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s: enter", __func__);
  mHandlers.clear();
  mIEventHandler = nullptr;
  mDefaultEventHandler = nullptr;
}

NfcExtensionController *NfcExtensionController::getInstance() {
  if (!sNfcExtensionController) {
    sNfcExtensionController =
        std::unique_ptr<NfcExtensionController>(new NfcExtensionController());
  }
  return sNfcExtensionController.get();
}

std::shared_ptr<IEventHandler> NfcExtensionController::getEventHandler() {
  return mIEventHandler;
}

void NfcExtensionController::addEventHandler(
    HandlerType handlerType, std::shared_ptr<IEventHandler> eventHandler) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  if (HandlerType::DEFAULT == handlerType) {
    mDefaultEventHandler = eventHandler;
    mIEventHandler = eventHandler;
    mHandlers[handlerType] = eventHandler;
  } else {
    mHandlers[handlerType] = std::move(eventHandler);
  }
}

void NfcExtensionController::revertToDefaultHandler() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mIEventHandler = mDefaultEventHandler;
}

// TODO:Add state diagram for usage for switchEventHandler
void NfcExtensionController::switchEventHandler(HandlerType handlerType) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter handlerType:%d", __func__,
                 static_cast<int>(handlerType));
  auto it = mHandlers.find(handlerType);
  if (it != mHandlers.end()) {
    /* When incoming handler is different than current handler then
       current handler will be notified about feature end and
       handleVendorNciMessage will be received only to incoming handler after
       this switch */
    if (handlerType != mCurrentHandlerType) {
      if (mIEventHandler) {
        mCurrentHandlerState = HandlerState::STOPPED;
        mIEventHandler->onFeatureEnd();
      }
    }
    mIEventHandler = it->second;
    if (mIEventHandler) {
      mIEventHandler->onFeatureStart();
      mCurrentHandlerType = handlerType;
      mCurrentHandlerState = HandlerState::STARTED;
    }
  } else {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Handler not found", __func__);
  }
}

void NfcExtensionController::init() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  Srd::getInstance()->Initialize();
}

NFCSTATUS NfcExtensionController::handleVendorNciMessage(uint16_t dataLen,
                                                         const uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter dataLen:%d", __func__,
                 dataLen);
  auto subGid =
      HandlerType((pData[SUB_GID_OID_INDEX] & SUB_GID_MASK) >> NCI_SHIFT_BY_4);
  const uint8_t subOid = (pData[SUB_GID_OID_INDEX] & SUB_OID_MASK);
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s subGid:%d subOid:%d", __func__,
                 static_cast<int>(subGid), subOid);

  auto it = mHandlers.find(subGid);
  if (it != mHandlers.end()) {
    const std::shared_ptr<IEventHandler> eventHandler = it->second;
    return eventHandler->handleVendorNciMessage(dataLen, pData);
  } else {
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Handler not found", __func__);
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }
}

NFCSTATUS NfcExtensionController::handleVendorNciRspNtf(uint16_t dataLen,
                                                        uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN,
                 "NfcExtensionController::%s Enter dataLen:%d", __func__,
                 dataLen);
  NFCSTATUS status = NciStateMonitor::getInstance()->handleVendorNciRspNtf(dataLen, pData);
  if (status == NFCSTATUS_EXTN_FEATURE_SUCCESS) {
    return status;
  }

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      BroadcastFrameHandler::getInstance()->handleVendorNciRspNtf(
          dataLen, const_cast<uint8_t*>(pData)))
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      AutoCard::getInstance()->handleVendorNciRspNtf(dataLen, pData))
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      DualAntenna::getInstance()->handleVendorNciRspNtf(dataLen,
                                                        (uint8_t *)pData))
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      NxpNTag::getInstance()->handleVendorNciRspNtf(
          dataLen, const_cast<uint8_t *>(pData)))
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      processSeNtfToUpdateMifareSupport(dataLen, const_cast<uint8_t*>(pData)))
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  return mIEventHandler->handleVendorNciRspNtf(dataLen, pData);
}

NFCSTATUS NfcExtensionController::onHandleHalEvent(uint8_t event) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter event:%d", __func__,
                 event);
  NFCSTATUS status = NciStateMonitor::getInstance()->handleHalEvent(event);
  if (status == NFCSTATUS_EXTN_FEATURE_SUCCESS) {
    return status;
  }
  return mIEventHandler->handleHalEvent(event);
}

void NfcExtensionController::onWriteCompletion(uint8_t status) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter status:%d", __func__,
                 status);
  mIEventHandler->onWriteComplete(status);
}

void NfcExtensionController::updateNfcHalState(int state) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter state:%d", __func__,
                 state);
  auto it = mNfcHalStateMap.find(state);
  if (it != mNfcHalStateMap.end()) {
    mNfcHalState = it->second;
  } else {
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Invalid state", __func__);
  }
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter mNfcHalState:%d", __func__,
                 static_cast<int>(mNfcHalState));
  // TODO: NFC state handling to be done
}

void NfcExtensionController::halControlGranted() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter ", __func__);
  mIEventHandler->halControlGranted();
}

void NfcExtensionController::halRequestControlTimedout() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mIEventHandler->onHalRequestControlTimeout();
}

void NfcExtensionController::writeRspTimedout() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mIEventHandler->onWriteRspTimeout();
}

NFCSTATUS NfcExtensionController::processExtnWrite(uint16_t *dataLen,
                                                   uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "NfcExtensionController %s Enter",
                 __func__);
  if (mIEventHandler->processExtnWrite(dataLen, pData) == NFCSTATUS_EXTN_FEATURE_SUCCESS) {
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }
  return NciStateMonitor::getInstance()->processNciCmd(*dataLen, pData);
}

NFCSTATUS NfcExtensionController::processSeNtfToUpdateMifareSupport(
    uint16_t dataLen, uint8_t* pData) {
  if (pData == nullptr || dataLen < NFCC_EE_DIS_NTF_LEN)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  if (isValidSeDiscNtf(dataLen, pData) &&
      isSupportedSeId(pData[NCI_SE_ID_INDEX]) &&
      pData[NFCC_EE_TECH_INDEX] == NCI_TYPE_A_LISTEN &&
      pData[NFCC_EE_PROTOCOL_INDEX] == NFCC_EE_ISO_DEP_PROTO) {
    constexpr uint8_t NFCEE_ADD_REMOVE_INDEX = 4;

    if (pData[NFCEE_ADD_REMOVE_INDEX] == 0x00) {
      std::vector<uint8_t> getSeSakStatus = {0x20, 0x03, 0x03,
                                             0x01, 0xA0, 0xF0};
      if (pData[NCI_SE_ID_INDEX] == NCI_ROUTE_UICC1_ID)
        getSeSakStatus[5] = CONFIG_PARAM_UICC1_VAL;

      PlatformAbstractionLayer::getInstance()->palenQueueWrite(
          getSeSakStatus.data(), getSeSakStatus.size());

      return NFCSTATUS_EXTN_FEATURE_FAILURE;
    }
    std::vector<uint8_t> nfcEeMifareRemoveNtf = {0x61, 0x0A, 0x06, 0x01, 0x00,
                                                 0x03, 0xC0, 0x80, 0x80};

    nfcEeMifareRemoveNtf[NFCEE_ADD_REMOVE_INDEX] =
        pData[NFCEE_ADD_REMOVE_INDEX];

    PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(dataLen,
                                                                    pData);
    PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
        nfcEeMifareRemoveNtf.size(), nfcEeMifareRemoveNtf.data());

    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  if (isValidGetCfgRsp(dataLen, pData) &&
      (pData[NCI_SE_ID_INDEX] == CONFIG_PARAM_ESE_VAL ||
       pData[NCI_SE_ID_INDEX] == CONFIG_PARAM_UICC1_VAL)) {
    constexpr uint8_t MIFARE_SUPPORT_STATUS_INDEX = 21;
    constexpr uint8_t NFCC_EE_MIFARE_PROTOCOL_SUPPORT = 0x08;

    std::vector<uint8_t> nfcEeMifareAddNtf = {0x61, 0x0A, 0x06, 0x01, 0x00,
                                              0x03, 0xC0, 0x80, 0x80};

    if (pData[NCI_SE_ID_INDEX] == CONFIG_PARAM_UICC1_VAL)
      nfcEeMifareAddNtf[NCI_SE_ID_INDEX] = NCI_ROUTE_UICC1_ID;

    if (pData[MIFARE_SUPPORT_STATUS_INDEX] & NFCC_EE_MIFARE_PROTOCOL_SUPPORT) {
      NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN,
                     "%s Sending SE add ntf for Mifare", __func__);

      PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
          nfcEeMifareAddNtf.size(), nfcEeMifareAddNtf.data());
    }
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }
  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}
