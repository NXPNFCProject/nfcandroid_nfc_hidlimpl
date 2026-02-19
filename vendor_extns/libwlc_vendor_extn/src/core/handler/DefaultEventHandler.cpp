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

#include "DefaultEventHandler.h"
#include "WlcExtensionConstants.h"
#include "WlcExtensionController.h"
#include "PlatformAbstractionLayer.h"
#include <phNxpLog.h>

WLCSTATUS DefaultEventHandler::handleVendorNciMessage(uint16_t dataLen,
                                                      const uint8_t *pData) {
  NXPLOG_EXTNS_D(
      NXPLOG_ITEM_NXP_GEN_EXTN,
      "DefaultEventHandler::%s Enter dataLen:%d, EventHandlerType:%d, "
      "EventHandlerState:%d",
      __func__, dataLen,
      static_cast<int>(
          WlcExtensionController::getInstance()->getEventHandlerType()),
      static_cast<int>(
          WlcExtensionController::getInstance()->getEventHandlerState()));
  return WLCSTATUS_EXTN_FEATURE_FAILURE;
}

WLCSTATUS DefaultEventHandler::handleVendorNciRspNtf(uint16_t dataLen,
                                                     uint8_t *pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN,
                 "DefaultEventHandler::%s Enter dataLen:%d", __func__, dataLen);
  // call false, if it is not to be processed by Wlc Extension library
  return WLCSTATUS_EXTN_FEATURE_FAILURE;
}
