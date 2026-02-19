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

#include "WlcExtensionController.h"
#include "WlcExtensionConstants.h"
#include "PlatformAbstractionLayer.h"
#include <NciStateMonitor.h>
#include <phNxpLog.h>
WlcExtensionController *WlcExtensionController::sWlcExtensionController;

WlcExtensionController::WlcExtensionController() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s: enter", __func__);
  mWlcHalState = {0};
  mCurrentHandlerType = {0};
  mCurrentHandlerState = HandlerState::STOPPED;
}

WlcExtensionController::~WlcExtensionController() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s: enter", __func__);
  mHandlers.clear();
  mIEventHandler = nullptr;
  mDefaultEventHandler = nullptr;
  NciStateMonitor::finalize();
}

WlcExtensionController *WlcExtensionController::getInstance() {
  if (sWlcExtensionController == nullptr) {
    sWlcExtensionController = new WlcExtensionController();
  }
  return sWlcExtensionController;
}

std::shared_ptr<IEventHandler> WlcExtensionController::getEventHandler() {
  return mIEventHandler;
}

void WlcExtensionController::addEventHandler(
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

void WlcExtensionController::revertToDefaultHandler() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mIEventHandler = mDefaultEventHandler;
}

// TODO:Add state diagram for usage for switchEventHandler
void WlcExtensionController::switchEventHandler(HandlerType handlerType) {
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

void WlcExtensionController::init() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
}

WLCSTATUS WlcExtensionController::handleVendorNciMessage(uint16_t dataLen,
                                                         const uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter dataLen:%d", __func__,
                 dataLen);
  auto subGid =
      HandlerType((pData[SUB_GID_OID_INDEX] & SUB_GID_MASK) >> NCI_SHIFT_BY_4);
  uint8_t subOid = (pData[SUB_GID_OID_INDEX] & SUB_OID_MASK);
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s subGid:%d subOid:%d", __func__,
                 static_cast<int>(subGid), subOid);

  auto it = mHandlers.find(subGid);
  if (it != mHandlers.end()) {
    std::shared_ptr<IEventHandler> eventHandler = it->second;
    return eventHandler->handleVendorNciMessage(dataLen, pData);
  } else {
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Handler not found", __func__);
    return WLCSTATUS_EXTN_FEATURE_FAILURE;
  }
}

WLCSTATUS WlcExtensionController::handleVendorNciRspNtf(uint16_t dataLen,
                                                        uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN,
                 "WlcExtensionController::%s Enter dataLen:%d", __func__,
                 dataLen);
  WLCSTATUS status = NciStateMonitor::getInstance()->handleVendorNciRspNtf(dataLen, pData);
  if (status == WLCSTATUS_EXTN_FEATURE_SUCCESS) {
    return status;
  }
  return mIEventHandler->handleVendorNciRspNtf(dataLen, pData);
}

WLCSTATUS WlcExtensionController::onHandleHalEvent(uint8_t event) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter event:%d", __func__,
                 event);
  WLCSTATUS status = NciStateMonitor::getInstance()->handleHalEvent(event);
  if (status == WLCSTATUS_EXTN_FEATURE_SUCCESS) {
    return status;
  }
  return mIEventHandler->handleHalEvent(event);
}

void WlcExtensionController::onWriteCompletion(uint8_t status) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter status:%d", __func__,
                 status);
  mIEventHandler->onWriteComplete(status);
}

void WlcExtensionController::updateWlcHalState(int state) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter state:%d", __func__,
                 state);
  auto it = mWlcHalStateMap.find(state);
  if (it != mWlcHalStateMap.end()) {
    mWlcHalState = it->second;
  } else {
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Invalid state", __func__);
  }
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter mWlcHalState:%d", __func__,
                 static_cast<int>(mWlcHalState));
  // TODO: WLC state handling to be done
}

void WlcExtensionController::halControlGranted() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter ", __func__);
  mIEventHandler->halControlGranted();
}

void WlcExtensionController::halRequestControlTimedout() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mIEventHandler->onHalRequestControlTimeout();
}

void WlcExtensionController::writeRspTimedout() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mIEventHandler->onWriteRspTimeout();
}

WLCSTATUS WlcExtensionController::processExtnWrite(uint16_t *dataLen,
                                                   uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "WlcExtensionController %s Enter",
                 __func__);
  if (mIEventHandler->processExtnWrite(dataLen, pData) == WLCSTATUS_EXTN_FEATURE_SUCCESS) {
    return WLCSTATUS_EXTN_FEATURE_SUCCESS;
  }
  return NciStateMonitor::getInstance()->processNciCmd(*dataLen, pData);;
}
