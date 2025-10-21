/*
 * Copyright 2025 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "NxpNfcExtension.h"

#include <ObserveMode.h>
#include <phNxpLog.h>

#include "phNxpAutoCard.h"
#include "phNxpNTag.h"

void phNxpNfcExtn_deInit() {
  AutoCard::finalize();
  NxpNTag::finalize();
}

NFCSTATUS phNxpNfcExtn_HandleNciMsg(uint16_t* dataLen, const uint8_t* pData) {
  NXPLOG_NCIHAL_D("%s Enter dataLen:%d", __func__, *dataLen);

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      AutoCard::getInstance()->handleVendorNciMessage(*dataLen,
                                                      (uint8_t*)pData))
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  return NxpNTag::getInstance()->handleVendorNciMessage(*dataLen,
                                                        (uint8_t*)pData);
}

NFCSTATUS phNxpNfcExtn_HandleNciRspNtf(uint16_t* dataLen,
                                       const uint8_t* pData) {
  NXPLOG_NCIHAL_D("%s Enter dataLen:%d", __func__, *dataLen);

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      handleObserveModeRfStateRspNtf(*dataLen, (uint8_t*)pData)) {
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      AutoCard::getInstance()->handleVendorNciRspNtf(*dataLen, (uint8_t*)pData))
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  return NxpNTag::getInstance()->handleVendorNciRspNtf(*dataLen,
                                                       (uint8_t*)pData);
}

void phNxpNfcExtn_core_initialized() {
  NxpNTag::getInstance()->phNxpNciHal_disableNtagNtfConfig();
  AutoCard::getInstance()->phNxpNciHal_getAutoCardConfig();
}
