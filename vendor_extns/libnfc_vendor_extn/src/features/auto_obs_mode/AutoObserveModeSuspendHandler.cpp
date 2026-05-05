/******************************************************************************
 *
 *  Copyright 2026 NXP
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

#include "AutoObserveModeSuspendHandler.h"

std::unique_ptr<AutoObserveModeSuspendHandler>
    AutoObserveModeSuspendHandler::sAutoObserveModeSuspendHandler = nullptr;

AutoObserveModeSuspendHandler::AutoObserveModeSuspendHandler() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN,
                 "AutoObserveModeSuspendHandler::%s Enter ", __func__);
}

AutoObserveModeSuspendHandler::~AutoObserveModeSuspendHandler() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN,
                 "AutoObserveModeSuspendHandler::%s Enter ", __func__);
}

AutoObserveModeSuspendHandler* AutoObserveModeSuspendHandler::getInstance() {
  if (!sAutoObserveModeSuspendHandler) {
    sAutoObserveModeSuspendHandler =
        std::unique_ptr<AutoObserveModeSuspendHandler>(
            new AutoObserveModeSuspendHandler());
  }
  return sAutoObserveModeSuspendHandler.get();
}

void AutoObserveModeSuspendHandler::
    phNxpNciHal_configureAutoObserveModeSuspendHandler() {
  uint8_t autoObserveModeConfig[AUTO_OBSERVE_MODE_CONFIG_SIZE];
  const long bufflen = AUTO_OBSERVE_MODE_CONFIG_SIZE;
  long retlen = 0;
  const bool isFound =
      PlatformAbstractionLayer::getInstance()->palGetNxpByteArrayValue(
          NAME_NXP_AUTO_OBSERVE_MODE_SUSPEND_CONFIG,
          reinterpret_cast<char*>(autoObserveModeConfig), bufflen, &retlen);
  if (!isFound) {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN, "%s %s config not found", __func__,
                   NAME_NXP_AUTO_OBSERVE_MODE_SUSPEND_CONFIG);
    return;
  }
  if (retlen < 3) {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN,
                   "%s %s config has lessthan 3 bytes", __func__,
                   NAME_NXP_AUTO_OBSERVE_MODE_SUSPEND_CONFIG);
    return;
  }

  uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
  uint16_t rsp_len = 0;

  std::vector<uint8_t> setStrAutoSelection = {
      NCI_PROP_CMD_VAL,
      AUTO_OBSERVE_MODE_OID,
      AUTO_OBSERVE_MODE_CMD_PAYLOAD_LENGTH,
      AUTO_OBSERVE_MODE_SET_OP_ID,
      autoObserveModeConfig[0],
      autoObserveModeConfig[1],
      autoObserveModeConfig[2]};
  NFCSTATUS status = PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
      setStrAutoSelection.size(), setStrAutoSelection.data(), &rsp_len, rsp);

  if (status == NFCSTATUS_SUCCESS) {
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Set command success",
                   __func__);
  } else {
    NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Set command fail", __func__);
  }
}
