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

/**
 * \addtogroup Next_gen_NFC_firmware_download
 *
 * @{ */
#pragma once
#include <memory>
#include <vector>
#include "DnldV2Cmd.h"
#include "DnldV2Opcodes.h"
#include "IntervalTimer.h"
#include "phNxpNciHal_utils.h"

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Class for read and write of firmware download packets
 *
 */
class DnldV2Transport {
 public:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function sends the command packet to driver for i2c write
   *
   * \param[in]       pBuffer            - pointer for command
   * \param[in]       wLength            - length of pBuffer
   *
   * \retval NFCSTATUS_SUCCESS           - if command is processed successfully
   * \retval NFCSTATUS_INVALID_PARAMETER - at least one parameter is
   *                                                invalid
   * \retval NFCSTATUS_BUSY              - write request is already in progress
   */
  NFCSTATUS hdllHcpWrite(std::vector<uint8_t>& data);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function reads the response/notification packet
   *
   * \param[in]      data                - vector to store the
   * response/notification
   * \param[in]      pTmlReadComplete    - callback to be invoked when the read
   * is completed by the driver
   * \param[in]      pContext            - pointer to the parameters passed for
   * the callback
   *
   * \retval NFCSTATUS_PENDING           - command is yet to be processed
   * \retval NFCSTATUS_INVALID_PARAMETER - at least one parameter is
   *                                                invalid
   * \retval NFCSTATUS_BUSY              - write request is already in progress
   */
  NFCSTATUS hdllHcpRead(std::vector<uint8_t>& data,
                        pphTmlNfc_TransactCompletionCb_t pTmlReadComplete,
                        void* pContext);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function performs ioctl call to NFC
   *
   * \param[in]       eControlCode       - ioctl code
   *
   * \retval NFCSTATUS_SUCCESS           - ioctl command completed successfully
   * \retval NFCSTATUS_FAILED            - ioctl command request failed
   */
  NFCSTATUS hdllIoctlOperation(phTmlNfc_ControlCode_t eControlCode);
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Main class to be called during firmware download
 *        for constructing commands and parsing responses/notifcations
 *
 */
class HcpPacketFrame {
 public:
  HcpPacketFrame();
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function updates the command parameters to be used for packet
   * construction
   *
   * \param[in]       mIsChunk           - if payload is sent in chunks
   * \param[in]       mCommandType       - type of command
   * \param[in]       mGroupID           - command group
   * \param[in]       mOperationId       - opcode of the command
   * \param[in]       mPayload           - payload to be formed inside the
   * packet
   *
   * \retval NFCSTATUS_SUCCESS           - command constructed successfully
   * \retval NFCSTATUS_FAILED            - when wrong HCP group identified
   */
  NFCSTATUS frameHdllMessage(bool mIsChunk, uint8_t mCommandType,
                             uint8_t mGroupID, uint8_t mOperationId,
                             std::vector<uint8_t> mPayload);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function updates the command parameters to be used for packet
   * construction, used when command read from binary file needs to be chunked
   *
   * \param[in]       mIsChunk           - if payload is sent in chunks
   * \param[in]       buff               - pointer to buffer data
   * \param[in]       buff_len           - length of buffer data
   *
   * \retval NFCSTATUS_SUCCESS           - command constructed successfully
   */
  NFCSTATUS frameHdllMessage(bool mIsChunk, const uint8_t* pBuff,
                             uint16_t buff_len);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function updates the command parameters to be used for packet
   * construction, used when the command read from the binary file would be sent
   * directly
   *
   * \param[in]       buff               - pointer to buffer data
   * \param[in]       buff_len           - length of buffer data
   *
   * \retval NFCSTATUS_SUCCESS           - command constructed successfully
   */
  NFCSTATUS frameHdllMessage(const uint8_t* pBuff, uint16_t buff_len);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function sets the output data received by callback
   *
   *
   */
  void setOutData(uint8_t* pBuff, uint16_t buff_len);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the response/notification received by NFC
   *
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  NFCSTATUS parseHdllResponse();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns true if the packet is chunked into multiple
   * packets
   *
   * \retval bool
   */
  bool getIsChunk();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the type of packet
   *
   * \retval uint8_t
   */
  uint8_t getType();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the group of packet
   *
   * \retval uint8_t
   */
  uint8_t getGroup();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the opcode of packet
   *
   * \retval uint8_t
   */
  uint8_t getOpcode();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the payload of packet
   *
   * \retval vector<uint8_t>
   */
  std::vector<uint8_t> getPayload();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function gives out the get info response received from firwmare
   *
   * \param[in]      mGetInfoResp        - empty structure of getInfoResp
   * \param[out]     mGetInfoResp        - get info response
   *
   * \retval void
   */
  void doGetInfo(getInfoResp* mGetInfoResp);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function updates the command parameters to be used for packet
   * construction
   *
   * \param[in]       mIsChunk           - if payload is sent in chunks
   * \param[in]       mCommandType       - type of command
   * \param[in]       mGroupID           - command group
   * \param[in]       mOperationId       - opcode of the command
   * \param[in]       mPayload           - payload to be formed inside the
   * packet
   *
   * \retval void
   */
  void setParms(bool isChunk, uint8_t commandType, uint8_t groupID,
                uint8_t operationId, std::vector<uint8_t> payload);
  NFCSTATUS write();
  NFCSTATUS read(pphTmlNfc_TransactCompletionCb_t pTmlReadComplete,
                 void* pContext);

 private:
  DnldV2Transport mDnldV2Transport;
  bool mIsChunk;
  uint8_t mCommandType;
  uint8_t mGroupID;
  uint8_t mOperationId;
  std::vector<uint8_t> mPayload;
  Generic groupGeneric;
  Protocol groupProtocol;
  EDL groupEDL;
  Raw groupRaw;
  std::vector<uint8_t> mOutData;
};