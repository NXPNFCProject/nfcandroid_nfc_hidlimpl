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

#include "BroadcastFrameHandler.h"

std::mutex BroadcastFrameHandler::sBroadcastFrameHandlerMutex;
std::unique_ptr<BroadcastFrameHandler>
    BroadcastFrameHandler::sBroadcastFrameHandler = nullptr;

BroadcastFrameHandler::BroadcastFrameHandler() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "BroadcastFrameHandler::%s Enter ",
                 __func__);
}

BroadcastFrameHandler::~BroadcastFrameHandler() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "BroadcastFrameHandler::%s Enter ",
                 __func__);
}

BroadcastFrameHandler* BroadcastFrameHandler::getInstance() {
  std::lock_guard<std::mutex> lock(sBroadcastFrameHandlerMutex);
  if (!sBroadcastFrameHandler) {
    sBroadcastFrameHandler =
        std::unique_ptr<BroadcastFrameHandler>(new BroadcastFrameHandler());
  }
  return sBroadcastFrameHandler.get();
}

void BroadcastFrameHandler::phNxpNciHal_configureBroadcastFrameHandler() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Enter ", __func__);
  uint8_t BroadcastFrameHandlerConfig[BROADCAST_FRAME_CONFIG_SIZE] = {0};
  ;
  const long bufflen = BROADCAST_FRAME_CONFIG_SIZE;
  long retlen = 0;
  const bool isFound =
      PlatformAbstractionLayer::getInstance()->palGetNxpByteArrayValue(
          NAME_NXP_BROADCAST_FRAME_CONFIG,
          reinterpret_cast<char*>(BroadcastFrameHandlerConfig), bufflen,
          &retlen);
  uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
  uint16_t rsp_len = 0;

  if (isFound) {
    NFCSTATUS status =
        PlatformAbstractionLayer::getInstance()->palNfcSendExtCmd(
            retlen, BroadcastFrameHandlerConfig, &rsp_len, rsp);

    if (status == NFCSTATUS_SUCCESS || rsp_len != 4 || rsp[3] != 0x00) {
      NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Set command success",
                     __func__);
    } else {
      NXPLOG_EXTNS_E(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Set command fail", __func__);
    }
  } else {
    NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Config %s not found ",
                   __func__, NAME_NXP_BROADCAST_FRAME_CONFIG);
  }

  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s Exit ", __func__);
}

NFCSTATUS BroadcastFrameHandler::handleVendorNciRspNtf(uint16_t dataLen,
                                                       uint8_t* pData) {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "BroadcastFrameHandler:%s Enter",
                 __func__);

  std::vector<uint8_t> converted;

  if (pData == nullptr || dataLen < MIN_HEADER_LEN)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  if (pData[0] != BROADCAST_ACTION_NTF_GID ||
      pData[1] != BROADCAST_ACTION_NTF_OID)
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  // Convert 6116 Notification to
  uint8_t payloadLen = pData[NCI_MSG_LEN_INDEX];

  if (payloadLen > (UINT8_MAX - 2)) return NFCSTATUS_EXTN_FEATURE_FAILURE;

  // Sanity check
  if (dataLen < (MIN_HEADER_LEN + payloadLen))
    return NFCSTATUS_EXTN_FEATURE_FAILURE;

  // Reserve required space (optional but efficient)
  converted.reserve(MIN_HEADER_LEN + PAYLOAD_TWO_LEN + payloadLen);

  // Header
  converted.push_back(NCI_PROP_NTF_VAL);
  converted.push_back(NCI_ROW_PROP_OID_VAL);

  // Length = 2 bytes (flags) + payload
  uint8_t newLen = 2 + payloadLen;
  converted.push_back(newLen);

  // Bit fields
  converted.push_back(BROADCAST_ACTION_NTF_SUB_GID);  // 0001b
  converted.push_back(BROADCAST_ACTION_NTF_SUB_OID);  // 0011b

  // Payload copy

  converted.insert(converted.end(), pData + MIN_HEADER_LEN,
                   pData + MIN_HEADER_LEN + payloadLen);

  PlatformAbstractionLayer::getInstance()->palSendNfcDataCallback(
      converted.size(), &converted[0]);

  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}
