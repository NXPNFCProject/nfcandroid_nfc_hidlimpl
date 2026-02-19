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

#include "DnldV2Cmd.h"
#include <log/log.h>
#include "DnldV2Opcodes.h"
#include "phDnldNfc_Utils.h"
#include "phNxpLog.h"

static bool verifyCRC(uint8_t* pBuff, uint16_t buffLen, uint16_t CRC16) {
  return CRC16 == phDnldNfc_CalcCrc16(pBuff, buffLen);
}

/* Implementation of message */
Message::Message() {
  mIsChunk = false;
  mLen = UINT16_MAX;
  mType = UINT8_MAX;
  mGroup = UINT8_MAX;
  mOperation = UINT8_MAX;
  mPayload = {};
  mCRC16 = UINT16_MAX;
  mPadding = UINT8_MAX;
  mRawDnldFrame = nullptr;
  mRawDnldFrameLen = UINT8_MAX;
}

Message::~Message(){};

/* Implementation of command */
Command::Command() {}

bool Command::getIsChunk() { return mIsChunk; }

uint16_t Command::getLen() { return mLen; }

uint8_t Command::getType() { return mType; }

uint8_t Command::getGroup() { return mGroup; }

uint8_t Command::getOpcode() { return mOperation; }

std::vector<uint8_t> Command::getPayload() { return mPayload; }

void Command::setParams(bool isChunk, uint8_t type, uint8_t group,
                        uint8_t operation, std::vector<uint8_t>& payload,
                        uint16_t CRC16, uint8_t padding, uint8_t* pRawCmd,
                        uint8_t rawCmdLen) {
  mIsChunk = isChunk;
  mType = type;
  mGroup = group;
  mOperation = operation;
  mPayload = payload;
  mCRC16 = CRC16;
  mPadding = padding;
  mRawDnldFrame = pRawCmd;
  mRawDnldFrameLen = rawCmdLen;
  mLen = 0;
}

/* need to fix len :- right now len is of uint8_t -> make it to 13bits*/
void Command::getSerializedCommand(std::vector<uint8_t>& command) {
  uint16_t HDLL = 0; /* RFU(2 bits) + Chunk(1 bit) + Pckt len(13 bits)*/

  /* Construct the command in reverse order to calculate the HCP and payload
   * length */

  if (mPayload.size() != 0) {
    HDLL += mPayload.size();
    std::reverse_copy(mPayload.begin(), mPayload.end(), command.begin());
  }

  if (mRawDnldFrame != nullptr) {
    HDLL += mRawDnldFrameLen;
    std::reverse_copy(mRawDnldFrame, mRawDnldFrame + mRawDnldFrameLen,
                      command.begin());
  }
  /* append HCP operation */
  command.push_back(mOperation);
  HDLL += EDL_FIRST_OFFSET;

  /* append HCP TYPE (2 bits) | HCP GROUP (6 bits)*/
  command.push_back(mGroup | (mType << EDL_HCP_GROUP_OFFSET));
  HDLL += EDL_FIRST_OFFSET;

  /* append HDLL */
  HDLL |= (mIsChunk ? 1 : 0) << EDL_CHUNK_OFFSET;
  command.push_back(HDLL & EDL_PCKT_BYTE);
  command.push_back(HDLL >> EDL_SHIFT_BYTE);

  /* reverse the array to get the actual command */
  std::reverse(command.begin(), command.end());
  /* remove the last dummy data, +2 is for adding the header bytes*/
  command.erase(command.begin() + HDLL + DNLDV2_HEADER_CRC_LEN, command.end());
  mCRC16 = phDnldNfc_CalcCrc16(command.data(), command.size());

  /* append the CRC */
  command.push_back(mCRC16 >> EDL_SHIFT_BYTE);
  command.push_back(mCRC16 & EDL_PCKT_BYTE);
}

/* do serialize for raw commands*/
void Command::getSerializedCommand(std::vector<uint8_t>& command, bool isChunk,
                                   const uint8_t* pBuff, uint16_t buff_len) {
  uint16_t HDLL = 0;
  if (pBuff != nullptr) {
    NXPLOG_NCIHAL_D("%s forming command by copying pBuff data passed",
                    __func__);
    HDLL += buff_len;
    std::reverse_copy(pBuff, pBuff + buff_len, command.begin());
  }
  command.erase(command.begin() + HDLL, command.end());
  // HDLL += 2; /* 2 bytes of CRC */ -- not required
  HDLL |= (isChunk ? 1 : 0) << EDL_CHUNK_OFFSET;
  command.push_back(HDLL & EDL_PCKT_BYTE);
  command.push_back(HDLL >> EDL_SHIFT_BYTE);
  /* reverse the array to get the actual command */
  std::reverse(command.begin(), command.end());
  mCRC16 = phDnldNfc_CalcCrc16(command.data(), command.size());

  /* append the CRC */
  command.push_back(mCRC16 >> EDL_SHIFT_BYTE);
  command.push_back(mCRC16 & EDL_PCKT_BYTE);
}

void Command::getSerializedCommand(std::vector<uint8_t>& command,
                                   const uint8_t* pBuff, uint16_t buff_len) {
  uint16_t HDLL = 0;

  if (pBuff != nullptr) {
    HDLL += buff_len;
    std::copy(pBuff, pBuff + buff_len, command.begin());
  }
  /*remove the last dummy data, +2 is for adding the header bytes*/
  command.erase(command.begin() + HDLL, command.end());
  mCRC16 = phDnldNfc_CalcCrc16(command.data(), command.size());

  /* append the CRC */
  command.push_back(mCRC16 >> EDL_SHIFT_BYTE);
  command.push_back(mCRC16 & EDL_PCKT_BYTE);
}

Command::~Command(){};

/* Implementation of response */
Response::Response() { clearVar(); }

bool Response::parseResponse(std::vector<uint8_t>& response) {
  clearVar();
  // getting the third bit
  mIsChunk = (response.at(DNLDV2_PCKT_START_IDX) & EDL_THIRD_LSB_OFFSET) >>
             EDL_CHUNK_OFFSET;
  // get the last 5 bits of len;
  mLen |= response.at(DNLDV2_PCKT_START_IDX) & EDL_FIVE_LSB;

  mLen <<= EDL_CHUNK_OFFSET;
  // adding the remaining length
  mLen |= response.at(EDL_FIRST_OFFSET);
  // getting the 2 MSbs
  mType |= (response.at(DNLDV2_HEADER_CRC_LEN) & EDL_THIRD_MSB_OFFSET) >>
           EDL_HCP_GROUP_OFFSET;
  // getting the 6 LSbs
  mGroup |= response.at(DNLDV2_HEADER_CRC_LEN) & EDL_EIGHT_LSB_OFFSET;

  mOperation |= response.at(DNLDV2_HDLL_OPCODE);

  mStatus |= response.at(EDL_FOURTH_OFFSET);

  mCRC16 |= response.at(DNLDV2_HEADER_CRC_LEN + mLen);  // getting MSb of CRC16

  mCRC16 <<= EDL_SHIFT_BYTE;
  mCRC16 |= response.at(mLen);

  mCRC16 |= response.at(DNLDV2_HEADER_CRC_LEN + mLen + EDL_FIRST_OFFSET);
  // Group (1 byte) + Operation (1 byte) + Status(1 byte) + CRC16 (2 bytes) = 5
  // bytes
  if (!verifyCRC(response.data(), DNLDV2_HEADER_CRC_LEN + mLen, mCRC16)) {
    return false;
  }

  if (mLen > DNLDV2_PCKT_START_IDX) {
    mPayload = std::vector<uint8_t>(
        &response.data()[EDL_CHUNK_OFFSET],
        &response[DNLDV2_HEADER_CRC_LEN + mLen - EDL_FIRST_OFFSET]);
  }
  return true;
}

bool Response::getIsChunk() { return mIsChunk; }

uint16_t Response::getLen() { return mLen; }

uint8_t Response::getType() { return mType; }

uint8_t Response::getGroup() { return mGroup; }

uint8_t Response::getOpcode() { return mOperation; }

uint8_t Response::getStatus() { return mStatus; }

std::vector<uint8_t> Response::getPayload() { return mPayload; }

void Response::clearVar() {
  mIsChunk = 0;
  mLen = 0;
  mType = 0;
  mGroup = 0;
  mOperation = 0;
  mPayload = {};
  mCRC16 = 0;
  mPadding = 0;
  mRawDnldFrame = nullptr;
  mRawDnldFrameLen = 0;
  mStatus = 0x00;
}

Response::~Response(){};

/* Implementation of notification */
Notification::Notification() { clearVar(); }

bool Notification::parseNotification(std::vector<uint8_t>& notification) {
  clearVar();
  //  getting the third bit
  mIsChunk = (notification.at(DNLDV2_PCKT_START_IDX) & EDL_THIRD_LSB_OFFSET) >>
             EDL_CHUNK_OFFSET;
  //  get the last 5 bits of len;
  mLen |= notification.at(DNLDV2_PCKT_START_IDX) & EDL_FIVE_LSB;
  mLen <<= EDL_CHUNK_OFFSET;
  //  adding the remaining length
  mLen |= notification.at(EDL_FIRST_OFFSET);
  //  getting the 2 MSbs
  mType |= (notification.at(DNLDV2_HEADER_CRC_LEN) & EDL_THIRD_MSB_OFFSET) >>
           EDL_HCP_GROUP_OFFSET;
  //  getting the 6 LSbs
  mGroup |= notification.at(DNLDV2_HEADER_CRC_LEN) & EDL_EIGHT_LSB_OFFSET;
  mOperation |= notification.at(DNLDV2_HDLL_OPCODE);
  mStatus |= notification.at(EDL_FOURTH_OFFSET);
  //  getting MSb of CRC16
  mCRC16 |= notification.at(DNLDV2_HEADER_CRC_LEN + mLen);
  mCRC16 <<= EDL_SHIFT_BYTE;
  mCRC16 |= notification.at(DNLDV2_HEADER_CRC_LEN + mLen + EDL_FIRST_OFFSET);

  //  Group (1 byte) + Operation (1 byte) + Status(1 byte) + CRC16 (2 bytes) = 5
  //  bytes
  if (!verifyCRC(notification.data(), DNLDV2_HEADER_CRC_LEN + mLen, mCRC16)) {
    return false;
  }

  mPayload = std::vector<uint8_t>(
      &notification.data()[EDL_CHUNK_OFFSET],
      &notification.data()[DNLDV2_HEADER_CRC_LEN + mLen - EDL_FIRST_OFFSET]);
  return true;
}

bool Notification::getIsChunk() { return mIsChunk; }

uint16_t Notification::getLen() { return mLen; }

uint8_t Notification::getType() { return mType; }

uint8_t Notification::getGroup() { return mGroup; }

uint8_t Notification::getOpcode() { return mOperation; }

uint8_t Notification::getStatus() { return mStatus; }

std::vector<uint8_t> Notification::getPayload() { return mPayload; }

void Notification::clearVar() {
  mIsChunk = 0;
  mLen = 0;
  mType = 0;
  mGroup = 0;
  mOperation = 0;
  mPayload = {};
  mCRC16 = 0;
  mPadding = 0;
  mRawDnldFrame = nullptr;
  mRawDnldFrameLen = 0;
  mStatus = 0x00;
}

Notification::~Notification(){};
/* Implementation of generic group */
Generic::Generic() {
  memset(&mGetInfoResp, 0, sizeof(getInfoResp));
  command = Command();
  response = Response();
  notification = Notification();
}

bool Group::parseResponse(std::vector<uint8_t>& response) {
  bool status = false;
  if (this->response.parseResponse(response)) {
    if (this->response.getStatus() == NFCSTATUS_SUCCESS)
      status = true;
    else {
      NXPLOG_NCIHAL_D("Found status error while parsing response = %d",
                      this->response.getStatus());
    }
  } else {
    NXPLOG_NCIHAL_D("%s CRC check failed", __func__);
    status = false;
  }

  return status;
};

void Generic::serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                               uint8_t type, uint8_t group, uint8_t operation,
                               std::vector<uint8_t>& payload) {
  this->command.setParams(isChunk, type, group, operation, payload);
  this->command.getSerializedCommand(command);
}

bool Generic::parseResponse(std::vector<uint8_t>& response) {
  bool status = false;
  if (this->response.parseResponse(response)) {
    if (this->response.getStatus() == NFCSTATUS_SUCCESS) {
      if (this->response.getGroup() == SecureDwnldHcpGroup_t::HCP_GENERIC &&
          this->response.getOpcode() ==
              SecureDwnldHcpGroupGeneric_t::GROUP_GENERIC_GET_INFO) {
        memcpy(&(mGetInfoResp), this->response.getPayload().data(),
               sizeof(mGetInfoResp));
      }
      status = true;
    } else {
      NXPLOG_NCIHAL_E("Found status error while parsing response = %d",
                      this->response.getStatus());
    }
  } else {
    NXPLOG_NCIHAL_E("%s CRC check failed", __func__);
    status = false;
  }
  return status;  // parsing failed due to CRC mismatch
}

bool Generic::parseNotification(std::vector<uint8_t>& notification) {
  bool status = false;
  if (this->notification.parseNotification(notification)) {
    if (this->notification.getStatus() == NFCSTATUS_SUCCESS)
      status = true;
    else {
      NXPLOG_NCIHAL_E("Found status error while parsing notification = %d",
                      this->notification.getStatus());
    }
  } else {
    NXPLOG_NCIHAL_E("%s CRC check failed", __func__);
    status = false;
  }
  return status;
}

void Generic::getInfo(void* mGetInfoResp) {
  memcpy(mGetInfoResp, &(this->mGetInfoResp), sizeof(getInfoResp));
}

Generic::~Generic(){};
/* Implementation of Protocol group */
Protocol::Protocol() {
  command = Command();
  response = Response();
  notification = Notification();
}

void Protocol::serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                                uint8_t type, uint8_t group, uint8_t operation,
                                std::vector<uint8_t>& payload) {
  this->command.setParams(isChunk, type, group, operation, payload);
  this->command.getSerializedCommand(command);
}

bool Protocol::parseNotification(std::vector<uint8_t>& notification) {
  bool status = false;
  if (this->notification.parseNotification(notification)) {
    if (this->notification.getStatus() == NFCSTATUS_SUCCESS)
      status = true;
    else {
      NXPLOG_NCIHAL_E("Found status error while parsing notification = %d",
                      this->notification.getStatus());
    }
  } else {
    NXPLOG_NCIHAL_E("%s CRC check failed", __func__);
    status = false;
  }
  return status;
}

Protocol::~Protocol(){};

/* Implementation of EDL group */
EDL::EDL() {
  command = Command();
  response = Response();
  notification = Notification();
}

void EDL::serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                           uint8_t type, uint8_t group, uint8_t operation,
                           std::vector<uint8_t>& payload) {
  this->command.setParams(isChunk, type, group, operation, payload);
  this->command.getSerializedCommand(command);
}

bool EDL::parseNotification(std::vector<uint8_t>& notification) {
  return this->notification.parseNotification(notification) &&
         this->notification.getStatus() == 0x00;
}

EDL::~EDL(){};

/* Implementation of Raw group to process raw data sent from binary file */
Raw::Raw() {
  command = Command();
  response = Response();
  notification = Notification();
}

void Raw::serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                           uint8_t type, uint8_t group, uint8_t operation,
                           std::vector<uint8_t>& payload) {
  /* Implmented just to override */
}

void Raw::serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                           const uint8_t* pBuff, uint16_t buff_len) {
  this->command.getSerializedCommand(command, isChunk, pBuff, buff_len);
}

void Raw::serializeMessage(std::vector<uint8_t>& command, const uint8_t* pBuff,
                           uint16_t buff_len) {
  this->command.getSerializedCommand(command, pBuff, buff_len);
}

bool Raw::parseResponse(std::vector<uint8_t>& response) {
  bool status = false;
  if (this->response.parseResponse(response)) {
    if (this->response.getStatus() == NFCSTATUS_SUCCESS)
      status = true;
    else {
      NXPLOG_NCIHAL_E("Found status error while parsing response = %d",
                      this->response.getStatus());
    }
  } else {
    NXPLOG_NCIHAL_E("%s CRC check failed", __func__);
    status = false;
  }

  return status;
}

bool Raw::parseNotification(std::vector<uint8_t>& notification) {
  bool status = false;
  if (this->notification.parseNotification(notification)) {
    if (this->notification.getStatus() == NFCSTATUS_SUCCESS)
      status = true;
    else {
      NXPLOG_NCIHAL_E("Found status error while parsing notification = %d",
                      this->notification.getStatus());
    }
  } else {
    NXPLOG_NCIHAL_E("%s CRC check failed", __func__);
    status = false;
  }
  return status;
}

Raw::~Raw(){};