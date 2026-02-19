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

#include "WlcExtensionWriter.h"
#include "WlcExtensionConstants.h"
#include "WlcExtensionController.h"
#include "PlatformAbstractionLayer.h"
#include <phNxpLog.h>

WlcExtensionWriter *WlcExtensionWriter::sWlcExtensionWriter = nullptr;

WlcExtensionWriter::WlcExtensionWriter() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
}

WlcExtensionWriter::~WlcExtensionWriter() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
}

WlcExtensionWriter *WlcExtensionWriter::getInstance() {
  if (sWlcExtensionWriter == nullptr) {
    sWlcExtensionWriter = new WlcExtensionWriter();
  }
  return sWlcExtensionWriter;
}

void WlcExtensionWriter::onhalControlGrant() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  stopHalCtrlTimer();
}

void WlcExtensionWriter::stopHalCtrlTimer() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  mHalCtrlTimer.kill(&mHalCtrlTimerId);
}

static void halRequestControlTimeoutCbk(union sigval val) {
  UNUSED_PROP(val);
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter ", __func__);
  WlcExtensionController::getInstance()->halRequestControlTimedout();
  return;
}

void WlcExtensionWriter::releaseHALcontrol() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  stopHalCtrlTimer();
  PlatformAbstractionLayer::getInstance()->palReleaseHALcontrol();
}

void WlcExtensionWriter::requestHALcontrol() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter ", __func__);
  if (mHalCtrlTimer.set(NXP_EXTNS_HAL_REQUEST_CTRL_TIMEOUT_IN_MS, NULL,
                        halRequestControlTimeoutCbk, &mHalCtrlTimerId)) {
    PlatformAbstractionLayer::getInstance()->palRequestHALcontrol();
  } else {
    WlcExtensionController::getInstance()
        ->getCurrentEventHandler()
        ->notifyGenericErrEvt(NCI_UN_RECOVERABLE_ERR);
    WlcExtensionController::getInstance()->switchEventHandler(
        HandlerType::DEFAULT);
  }
}

void WlcExtensionWriter::writeRspTimeoutCbk(union sigval val) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter", __func__);
  WlcExtensionWriter *writer = static_cast<WlcExtensionWriter *>(val.sival_ptr);
  if (writer) {
    writer->mWriteRspTimer.kill(&writer->mWriteRspTimerId);
  }
  WlcExtensionWriter &mExtWriter = *WlcExtensionWriter::getInstance();
  if (mExtWriter.lastNciCmd.size() >= 3) {
    PlatformAbstractionLayer::getInstance()->setVendorParam("nfc.cmd_timeout",
                                                            "");
    mExtWriter.write(mExtWriter.lastNciCmd.data(),
                     mExtWriter.lastNciCmd.size(),
                     NXP_EXTNS_WRITE_RSP_TIMEOUT_IN_MS);
    mExtWriter.lastNciCmd.clear();
  } else {
    WlcExtensionController::getInstance()->writeRspTimedout();
  }
  return;
}

WLCSTATUS WlcExtensionWriter::write(const uint8_t *pBuffer, uint16_t wLength,
                                    int timeout) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter wLength:%d", __func__,
                 wLength);
  lastNciCmd.clear();
  lastNciCmd.assign(pBuffer, pBuffer + wLength);
  if (mWriteRspTimer.set(timeout, NULL, writeRspTimeoutCbk, &mWriteRspTimerId)) {
    cmdData.clear();
    cmdData.assign(pBuffer, pBuffer + wLength);
    return PlatformAbstractionLayer::getInstance()->palenQueueWrite(pBuffer, wLength);
  } else {
    WlcExtensionController::getInstance()
        ->getCurrentEventHandler()
        ->notifyGenericErrEvt(NCI_UN_RECOVERABLE_ERR);
    WlcExtensionController::getInstance()->switchEventHandler(
        HandlerType::DEFAULT);
    return WLCSTATUS_INSUFFICIENT_RESOURCES;
  }
}

void WlcExtensionWriter::onWriteComplete(uint8_t status) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter status = 0x%x", __func__,
                 status);
}

void WlcExtensionWriter::stopWriteRspTimer(const uint8_t *pRspBuffer,
                                           uint16_t rspLength) {
  if (cmdData.size() >= NCI_PAYLOAD_LEN_INDEX &&
      rspLength >= NCI_PAYLOAD_LEN_INDEX) {
    uint8_t cmdGid = (cmdData[NCI_GID_INDEX] & EXT_NCI_GID_MASK);
    uint8_t cmdOid = (cmdData[NCI_OID_INDEX] & EXT_NCI_OID_MASK);
    uint8_t rspGid = (pRspBuffer[NCI_GID_INDEX] & EXT_NCI_GID_MASK);
    uint8_t rspOid = (pRspBuffer[NCI_OID_INDEX] & EXT_NCI_OID_MASK);

    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s Enter cmdGid:%d, cmdOid:%d, rspGid:%d, rspOid:%d,",
                   __func__, cmdGid, cmdOid, rspGid, rspOid);
    if (cmdGid == rspGid && cmdOid == rspOid && mWriteRspTimerId != 0)
      mWriteRspTimer.kill(&mWriteRspTimerId);
  }
}
