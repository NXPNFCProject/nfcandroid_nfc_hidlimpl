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
#include <stdint.h>
#include <stdio.h>
#include <algorithm>
#include <vector>

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief get Info Response unit structure params
 *
 */
typedef struct getInfoResp {
  uint8_t bootStatus;
  uint8_t romVersion;
  uint8_t protectedPageStatus;
  uint8_t lifecycleVersionScratch[2];
  uint8_t lifecycleVersionProtected[2];
  uint8_t chipVersion[2];
  uint8_t firmwareVersionNxpScratch[2];
  uint8_t firmwareVersionNxpProtected[2];
  uint8_t chipVariant[2];
  uint8_t hwVersion[2];
  uint8_t deviceLifecycle[4];
  uint8_t deviceLifecycleScratch[4];
  uint8_t deviceLifecycleProtected[4];
  uint8_t dieId[16];
  uint8_t dieIdCrc[4];
  uint8_t minLifeCycleCertificateVersionScratch[2];
  uint8_t minLifecycleCertificateVersionProtected[2];
  uint8_t minDownloadCertificateVersionScratch[2];
  uint8_t minDownloadCertificateVersionProtected[2];
  uint8_t hashAppNxpStatus;
  uint8_t hashAppCustStatus;
  uint8_t hashAppNxp[32];
  uint8_t hashAppCust[32];
  uint8_t trimmings[80];
  uint8_t rfu1;
  uint8_t modelId;
  uint8_t rfu2[6];
} getInfoResp;

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Base class to hold command, response and notification of firmware
 * download packet
 *
 */
class Message {
 public:
  Message();
  virtual bool getIsChunk() = 0;
  virtual uint16_t getLen() = 0;
  virtual uint8_t getType() = 0;
  virtual uint8_t getGroup() = 0;
  virtual uint8_t getOpcode() = 0;  // opcode
  virtual std::vector<uint8_t> getPayload() = 0;
  virtual ~Message() = 0;

 protected:
  bool mIsChunk;
  uint16_t mLen;
  uint8_t mType;
  uint8_t mGroup;
  uint8_t mOperation;  // opcode
  std::vector<uint8_t> mPayload;
  uint16_t mCRC16;
  uint8_t mPadding;
  uint8_t* mRawDnldFrame;
  uint8_t mRawDnldFrameLen;
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Class to hold command of firmware download packet
 *
 */
class Command : private Message {
 public:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief Empty construtor of command class.
   */
  Command();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function updates the command parameters to be used for packet
   * construction
   *
   * \param[in]       isChunk   - if payload is sent in chunks
   * \param[in]       type      - type of command
   * \param[in]       group     - command group
   * \param[in]       operation - opcode of the command
   * \param[in]       payload   - payload to be formed inside the packet
   * \param[out]      void
   *
   * \retval void
   */
  void setParams(bool isChunk, uint8_t type, uint8_t group, uint8_t operation,
                 std::vector<uint8_t>& payload, uint16_t CRC16 = 0,
                 uint8_t padding = 0, uint8_t* pRawCmd = nullptr,
                 uint8_t rawCmdLen = 0);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the packet
   *
   * \param[in]       command   - Empty vector
   * \param[out]      command   - Packet to be sent would be populated in
   * command
   *
   * \retval void
   */
  void getSerializedCommand(std::vector<uint8_t>& command);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the packet
   *
   * \param[in]       command   - Empty vector
   * \param[in]       isChunk   - if payload is sent in chunks
   * \param[in]       buff      - additional data to be appended onto the packet
   * \param[in]       buff_len  - buff length
   * \param[out]      command   - Packet to be sent would be populated in
   * command
   *
   * \retval void
   */
  void getSerializedCommand(std::vector<uint8_t>& command, bool isChunk,
                            const uint8_t* pBuff, uint16_t buff_len);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the packet
   *
   * \param[in]       command   - Empty vector
   * \param[in]       buff      - additional data to be appended onto the packet
   * \param[in]       buff_len  - buff length
   * \param[out]      command   - Packet to be sent would be populated in
   * command
   *
   * \retval void
   */
  void getSerializedCommand(std::vector<uint8_t>& command, const uint8_t* pBuff,
                            uint16_t buff_len);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns if the present command formulated is chunked
   * into multiple packets
   *
   * \retval bool
   */
  bool getIsChunk() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the length of command packet
   *
   * \retval uint16_t
   */
  uint16_t getLen() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the type of command packet
   *
   * \retval uint8_t
   */
  uint8_t getType() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the group of command packet
   *
   * \retval uint8_t
   */
  uint8_t getGroup() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the opcode of command packet
   *
   * \retval uint8_t
   */
  uint8_t getOpcode() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the payload of command packet
   *
   * \retval vector<uint8_t>
   */
  std::vector<uint8_t> getPayload() override;
  ~Command();

 private:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns the CRC16 of command packet
   *
   * \retval uint16_t
   */
  uint16_t calculateCRC();
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Class to hold response of firmware download packet
 *
 */
class Response : private Message {
 public:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief Empty construtor of command class.
   */
  Response();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the response received by NFC and maps with each
   * parameter of response
   *
   * \param[in]       response - vector that contains response received by NFC
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  bool parseResponse(std::vector<uint8_t>& response);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns status after the parsing of response is done.
   *            Note:
   *                 To get appropriate status, parseResponse should be called
   * before calling this function
   * \retval status value from response
   */
  uint8_t getStatus();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns if the response is chunked
   *            Note:
   *                 To get appropriate status, parseResponse should be called
   * before calling this function
   * \retval true  - chunked response
   * \retval false - no chunked response
   */
  bool getIsChunk() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns length of payload
   *            Note:
   *                 To get appropriate status, parseResponse should be called
   * before calling this function
   * \retval length of payload
   */
  uint16_t getLen() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns type of firmware download response
   *            Note:
   *                 To get appropriate status, parseResponse should be called
   * before calling this function
   * \retval type of response
   */
  uint8_t getType() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns group of firmware download response
   *            Note:
   *                 To get appropriate status, parseResponse should be called
   * before calling this function
   * \retval group type of payload
   */
  uint8_t getGroup() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns opcode of firmware download response
   *            Note:
   *                 To get appropriate status, parseResponse should be called
   * before calling this function
   * \retval opcode of payload
   */
  uint8_t getOpcode() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns payload of firmware download response
   *            Note:
   *                 To get appropriate status, parseResponse should be called
   * before calling this function
   * \retval payload
   */
  std::vector<uint8_t> getPayload() override;
  ~Response();

 private:
  uint8_t mStatus;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function clears all the variables
   *            Note:
   *                 Use before parsing a new response
   */
  void clearVar();
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Class to hold notification of firmware download packet
 *
 */
class Notification : private Message {
 public:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief Empty construtor of notification class.
   */
  Notification();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the notification received by NFC and maps with
   * each parameter of notification
   *
   * \param[in]       notification - vector that contains notification received
   * by NFC
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  bool parseNotification(std::vector<uint8_t>& notification);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns status after the parsing of notification is
   * done. Note: To get appropriate status, parseNotification should be called
   * before calling this function
   * \retval status value from notification
   */
  uint8_t getStatus();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns if the notification is chunked
   *            Note:
   *                 To get appropriate status, parseNotification should be
   * called before calling this function
   * \retval true  - chunked notification
   * \retval false - no chunked notification
   */
  bool getIsChunk() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns length of payload
   *            Note:
   *                 To get appropriate status, parseNotification should be
   * called before calling this function
   * \retval length of payload
   */
  uint16_t getLen() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns type of firmware download notification
   *            Note:
   *                 To get appropriate status, parseNotification should be
   * called before calling this function
   * \retval type of notification
   */
  uint8_t getType() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns group of firmware download notification
   *            Note:
   *                 To get appropriate status, parseNotification should be
   * called before calling this function
   * \retval group type of payload
   */
  uint8_t getGroup() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns opcode of firmware download notification
   *            Note:
   *                 To get appropriate status, parseNotification should be
   * called before calling this function
   * \retval opcode of payload
   */
  uint8_t getOpcode() override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function returns payload of firmware download notification
   *            Note:
   *                 To get appropriate status, parseNotification should be
   * called before calling this function
   * \retval payload
   */
  std::vector<uint8_t> getPayload() override;
  ~Notification();

 private:
  uint8_t mStatus;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function clears all the variables
   *            Note:
   *                 Use before parsing a new notification
   */
  void clearVar();
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Base class to hold different types of groups
 *        like generic, protocol, etc.. of firmware download packet
 *
 */
class Group {
 public:
  virtual void serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                                uint8_t type, uint8_t group, uint8_t operation,
                                std::vector<uint8_t>& payload) = 0;
  virtual bool parseResponse(std::vector<uint8_t>& response);
  virtual bool parseNotification(std::vector<uint8_t>& notification) = 0;
  virtual ~Group() = default;
  Command command;
  Response response;
  Notification notification;
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Generic commands class for Get_Info, Enter_Download_Mode,
 * Enter_Commit_mode
 *
 */
class Generic : public Group {
 public:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief Empty construtor of Generic class.
   */
  Generic();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the command packet based on
   *        the inputs provided
   *
   * \param[in]       isChunk   - if payload is sent in chunks then set to true
   * \param[in]       type      - type of command
   * \param[in]       group     - command group
   * \param[in]       operation - opcode of the command
   * \param[in]       payload   - payload to be formed inside the packet
   * \param[out]      command
   *
   * \retval void
   */
  void serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                        uint8_t type, uint8_t group, uint8_t operation,
                        std::vector<uint8_t>& payload) override;
  /* returns true if successfull, else false if there's an CRC error*/

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the response received by NFC
   *
   * \param[in]       response - vector that contains response received by NFC
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  bool parseResponse(std::vector<uint8_t>& response) override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the notification received by NFC
   *
   * \param[in]       notification - vector that contains notification received
   * by NFC
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  bool parseNotification(std::vector<uint8_t>& notification) override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function gives out the get info response received from firwmare
   *
   * \param[in]        mGetInfoResp - empty structure of getInfoResp
   * \param[out]       mGetInfoResp - get info response
   *
   * \retval void
   */
  void getInfo(void* mGetInfoResp);
  ~Generic();

 private:
  getInfoResp mGetInfoResp;
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Protocol commands class
 *
 */
class Protocol : public Group {
 public:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief Empty construtor of Protocol class.
   */
  Protocol();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the command packet based on
   *        the inputs provided
   *
   * \param[in]       isChunk   - if payload is sent in chunks then set to true
   * \param[in]       type      - type of command
   * \param[in]       group     - command group
   * \param[in]       operation - opcode of the command
   * \param[in]       payload   - payload to be formed inside the packet
   * \param[out]      command
   *
   * \retval void
   */
  void serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                        uint8_t type, uint8_t group, uint8_t operation,
                        std::vector<uint8_t>& payload) override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the notification received by NFC
   *
   * \param[in]       notification - vector that contains notification received
   * by NFC
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  bool parseNotification(std::vector<uint8_t>& notification) override;
  ~Protocol();
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Generic commands class for Get_Info, Enter_Download_Mode,
 * Enter_Commit_mode
 *
 */
class EDL : public Group {
 public:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief Empty construtor of EDL class.
   */
  EDL();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the command packet based on
   *        the inputs provided
   *
   * \param[in]       isChunk   - if payload is sent in chunks then set to true
   * \param[in]       type      - type of command
   * \param[in]       group     - command group
   * \param[in]       operation - opcode of the command
   * \param[in]       payload   - payload to be formed inside the packet
   * \param[out]      command
   *
   * \retval void
   */
  void serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                        uint8_t type, uint8_t group, uint8_t operation,
                        std::vector<uint8_t>& payload) override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the notification received by NFC
   *
   * \param[in]       notification - vector that contains notification received
   * by NFC
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  bool parseNotification(std::vector<uint8_t>& notification) override;
  ~EDL();
};

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Raw commands class for for the purpose of sending raw data from binary
 * file
 *
 */
class Raw : public Group {
 public:
  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief Empty construtor of Raw class.
   */
  Raw();

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the command packet based on
   *        the inputs provided
   *
   * \param[in]       isChunk   - if payload is sent in chunks then set to true
   * \param[in]       type      - type of command
   * \param[in]       group     - command group
   * \param[in]       operation - opcode of the command
   * \param[in]       payload   - payload to be formed inside the packet
   * \param[out]      command
   *
   * \retval void
   */
  void serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                        uint8_t type, uint8_t group, uint8_t operation,
                        std::vector<uint8_t>& payload) override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the command packet based on
   *        the raw data provided
   *
   * \param[in]       isChunk      - if payload is sent in chunks then set to
   * true
   * \param[in]       buff         - pointer to raw data
   * \param[in]       buff_len     - raw data length
   * \param[out]      command
   *
   * \retval void
   */
  void serializeMessage(std::vector<uint8_t>& command, bool isChunk,
                        const uint8_t* pBuff, uint16_t buff_len);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function formulates the command packet based on
   *        the raw data provided
   *
   * \param[in]       buff         - pointer to raw data
   * \param[in]       buff_len     - raw data length
   * \param[out]      command
   *
   * \retval void
   */
  void serializeMessage(std::vector<uint8_t>& command, const uint8_t* pBuff,
                        uint16_t buff_len);

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the response received by NFC
   *
   * \param[in]       response - vector that contains response received by NFC
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  bool parseResponse(std::vector<uint8_t>& response) override;

  /**
   * \ingroup Next_gen_NFC_firmware_download
   * \brief This function parses the notification received by NFC
   *
   * \param[in]       notification - vector that contains notification received
   * by NFC
   *
   * \retval true  - Parsing is successfull
   * \retval false - CRC check failed
   */
  bool parseNotification(std::vector<uint8_t>& notification) override;
  ~Raw();
};
/** @} */