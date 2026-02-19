/*
 * Copyright 2025-2026 NXP
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

#include "DnldV2CmdAssembler.h"

NFCSTATUS DnldV2Transport::hdllHcpWrite(std::vector<uint8_t>& data) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  status = phTmlNfc_Write(data.data(), (uint16_t)data.size());
  return status;
}

NFCSTATUS DnldV2Transport::hdllHcpRead(
    std::vector<uint8_t>& data,
    pphTmlNfc_TransactCompletionCb_t pTmlReadComplete, void* pContext) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  status = phTmlNfc_Read(data.data(), (uint16_t)data.capacity(),
                         pTmlReadComplete, pContext);
  return status;
}

NFCSTATUS DnldV2Transport::hdllIoctlOperation(
    phTmlNfc_ControlCode_t eControlCode) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  status = phTmlNfc_IoCtl(eControlCode);
  return status;
}

HcpPacketFrame::HcpPacketFrame() {
  mIsChunk = false;
  mCommandType = 0;
  mGroupID = 0;
  mOperationId = 0;
  mOutData.resize(DNLDV2_PCKT_LEN);
  std::fill(mOutData.begin(), mOutData.end(), 0);
}

NFCSTATUS HcpPacketFrame::frameHdllMessage(bool mIsChunk, uint8_t mCommandType,
                                           uint8_t mGroupID,
                                           uint8_t mOperationId,
                                           std::vector<uint8_t> mPayload) {
  mOutData.resize(DNLDV2_PCKT_LEN);
  NFCSTATUS status = NFCSTATUS_FAILED;
  setParms(mIsChunk, mCommandType, mGroupID, mOperationId, mPayload);
  switch (mGroupID) {
    case SecureDwnldHcpGroup_t::HCP_PROTOCOL:
      groupProtocol.serializeMessage(mOutData, mIsChunk, mCommandType, mGroupID,
                                     mOperationId, mPayload);
      status = NFCSTATUS_SUCCESS;
      break;

    case SecureDwnldHcpGroup_t::HCP_GENERIC:
      groupGeneric.serializeMessage(mOutData, mIsChunk, mCommandType, mGroupID,
                                    mOperationId, mPayload);
      status = NFCSTATUS_SUCCESS;
      break;

    case SecureDwnldHcpGroup_t::HCP_EDL:
      groupEDL.serializeMessage(mOutData, mIsChunk, mCommandType, mGroupID,
                                mOperationId, mPayload);
      status = NFCSTATUS_SUCCESS;
      break;

    default:
      status = NFCSTATUS_FAILED;
      break;
  }

  return status;
}

NFCSTATUS HcpPacketFrame::frameHdllMessage(bool mIsChunk, const uint8_t* pBuff,
                                           uint16_t buff_len) {
  mOutData.resize(DNLDV2_PCKT_LEN);
  NFCSTATUS status = NFCSTATUS_FAILED;
  groupRaw.serializeMessage(mOutData, mIsChunk, pBuff, buff_len);
  status = NFCSTATUS_SUCCESS;
  return status;
}

NFCSTATUS HcpPacketFrame::frameHdllMessage(const uint8_t* pBuff,
                                           uint16_t buff_len) {
  mOutData.resize(DNLDV2_PCKT_LEN);
  NFCSTATUS status = NFCSTATUS_FAILED;
  groupRaw.serializeMessage(mOutData, pBuff, buff_len);
  status = NFCSTATUS_SUCCESS;
  return status;
}

NFCSTATUS HcpPacketFrame::parseHdllResponse() {
  NFCSTATUS status = NFCSTATUS_FAILED;
  /* Check for type of response received from the MSB of 3rd byte*/

  uint8_t status_val =
      (mOutData.at(DNLDV2_HEADER_CRC_LEN) & EDL_THIRD_MSB_OFFSET) >>
      EDL_HCP_GROUP_OFFSET;

  switch (status_val) {
    case SecureDwnldHcpType::HCP_RESPONSE:
      switch (mOutData.at(DNLDV2_HEADER_CRC_LEN) & EDL_EIGHT_LSB_OFFSET) {
        case SecureDwnldHcpGroup::HCP_PROTOCOL:
          if (groupProtocol.parseResponse(mOutData)) {
            setParms(groupProtocol.response.getIsChunk(),
                     groupProtocol.response.getType(),
                     groupProtocol.response.getGroup(),
                     groupProtocol.response.getOpcode(),
                     groupProtocol.response.getPayload());
            status = NFCSTATUS_SUCCESS;
          } else {
            NXPLOG_NCIHAL_E(
                "%s Protocol group parse response failed due to status fail in "
                "notification / CRC error\n",
                __func__);
            status = NFCSTATUS_FAILED;
          }
          break;

        case SecureDwnldHcpGroup::HCP_GENERIC:
          if (groupGeneric.parseResponse(mOutData)) {
            setParms(groupGeneric.response.getIsChunk(),
                     groupGeneric.response.getType(),
                     groupGeneric.response.getGroup(),
                     groupGeneric.response.getOpcode(),
                     groupGeneric.response.getPayload());
            status = NFCSTATUS_SUCCESS;
          } else {
            NXPLOG_NCIHAL_E(
                "%s Generic group parse response failed due to status fail in "
                "notification / CRC error\n",
                __func__);
            status = NFCSTATUS_FAILED;
          }
          break;

        case SecureDwnldHcpGroup::HCP_EDL:
          if (groupEDL.parseResponse(mOutData)) {
            setParms(groupEDL.response.getIsChunk(),
                     groupEDL.response.getType(), groupEDL.response.getGroup(),
                     groupEDL.response.getOpcode(),
                     groupEDL.response.getPayload());
            status = NFCSTATUS_SUCCESS;
          } else {
            NXPLOG_NCIHAL_E(
                "%s EDL group parse response failed due to status fail in "
                "notification / CRC error\n",
                __func__);
            status = NFCSTATUS_FAILED;
          }
          break;

        default:
          break;
      }
      break;

    case SecureDwnldHcpType::HCP_NOTIFICATION:
      switch (mOutData.at(DNLDV2_HEADER_CRC_LEN) & EDL_EIGHT_LSB_OFFSET) {
        case SecureDwnldHcpGroup::HCP_PROTOCOL:
          if (groupProtocol.parseNotification(mOutData)) {
            setParms(groupProtocol.notification.getIsChunk(),
                     groupProtocol.notification.getType(),
                     groupProtocol.notification.getGroup(),
                     groupProtocol.notification.getOpcode(),
                     groupProtocol.notification.getPayload());
            status = NFCSTATUS_SUCCESS;
          } else {
            NXPLOG_NCIHAL_E(
                "%s Protocol group parse Notification failed due to status "
                "fail in notification / CRC error\n",
                __func__);
            status = NFCSTATUS_FAILED;
          }
          break;

        case SecureDwnldHcpGroup::HCP_GENERIC:
          if (groupGeneric.parseNotification(mOutData)) {
            setParms(groupGeneric.notification.getIsChunk(),
                     groupGeneric.notification.getType(),
                     groupGeneric.notification.getGroup(),
                     groupGeneric.notification.getOpcode(),
                     groupGeneric.notification.getPayload());
            status = NFCSTATUS_SUCCESS;
          } else {
            NXPLOG_NCIHAL_E(
                "%s Generic group parse Notification failed due to status fail "
                "in notification / CRC error\n",
                __func__);
            status = NFCSTATUS_FAILED;
          }
          break;

        case SecureDwnldHcpGroup::HCP_EDL:
          if (groupEDL.parseNotification(mOutData)) {
            setParms(groupEDL.notification.getIsChunk(),
                     groupEDL.notification.getType(),
                     groupEDL.notification.getGroup(),
                     groupEDL.notification.getOpcode(),
                     groupEDL.notification.getPayload());
            status = NFCSTATUS_SUCCESS;
          } else {
            NXPLOG_NCIHAL_E(
                "%s EDL group parse Notification failed due to status fail in "
                "notification / CRC error\n",
                __func__);
            status = NFCSTATUS_FAILED;
          }
          break;

        default:
          break;
      }
      break;

    default:
      NXPLOG_NCIHAL_E("Unexpected response error %s", __func__);
      break;
  }
  return status;
}

bool HcpPacketFrame::getIsChunk() { return mIsChunk; }

uint8_t HcpPacketFrame::getType() { return mCommandType; }

uint8_t HcpPacketFrame::getGroup() { return mGroupID; }

uint8_t HcpPacketFrame::getOpcode() { return mOperationId; }

std::vector<uint8_t> HcpPacketFrame::getPayload() { return mPayload; }

void HcpPacketFrame::setParms(bool isChunk, uint8_t commandType,
                              uint8_t groupID, uint8_t operationId,
                              std::vector<uint8_t> payload) {
  mIsChunk = isChunk;
  mCommandType = commandType;
  mGroupID = groupID;
  mOperationId = operationId;
  mPayload = payload;
}

void HcpPacketFrame::doGetInfo(getInfoResp* mGetInfoResp) {
  groupGeneric.getInfo(mGetInfoResp);
}

NFCSTATUS HcpPacketFrame::write() {
  NFCSTATUS status = NFCSTATUS_OK;
  status = mDnldV2Transport.hdllHcpWrite(mOutData);
  return status;
}

NFCSTATUS HcpPacketFrame::read(
    pphTmlNfc_TransactCompletionCb_t pTmlReadComplete, void* pContext) {
  std::fill(mOutData.begin(), mOutData.end(), 0);
  return mDnldV2Transport.hdllHcpRead(mOutData, pTmlReadComplete, pContext);
}

void HcpPacketFrame::setOutData(uint8_t* pBuff, uint16_t buff_len) {
  mOutData.resize(buff_len);
  std::fill(mOutData.begin(), mOutData.end(), 0);
  mOutData.assign(pBuff, pBuff + buff_len);
}
