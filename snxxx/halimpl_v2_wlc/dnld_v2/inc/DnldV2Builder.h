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
#include <algorithm>
#include <iterator>
#include <vector>
#include "DnldV2CmdAssembler.h"

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief Firmware download states
 *
 */
typedef enum DnldV2State {
  INIT = 0x00,
  EDL_CERT, /* certificate Verification state */
  WRITE_FIRST,
  NVM_WRITE,
  NVM_WRITE_LAST,
  COMMIT_MODE, /* enter commit mode */
  DOWNLOAD_COMPLETE,
  GET_INFO, /* get Info cmd */
} DnldV2State_t;

#define DnldV2BuilderInstance (DnldV2Builder::getInstance())

/**
 * \ingroup Next_gen_NFC_firmware_download
 * \brief SecureDwnLoadAgent class traverses through different firmware download
 * states. The commands are constructed and responses are parsed.
 *
 */

class SecureDwnLoadAgent {
 public:
  ~SecureDwnLoadAgent();
  void setParams(uint16_t fragmentLen, const uint8_t* fwImgPtr,
                 uint32_t fwImgLen, uint16_t fwVer);
  NFCSTATUS performFWDownload();
  NFCSTATUS getInfo();
  uint8_t getFwRomVersion();                /* should be called after getInfo */
  uint8_t getFwMajorVersion();              /* should be called after getInfo */
  uint8_t getHwVersion();                   /* should be called after getInfo */
  static SecureDwnLoadAgent& getInstance(); /*singleton object*/
  DnldV2State_t mDnldV2State;
  IntervalTimer mDnldV2Timer;
  HcpPacketFrame mPacketFrame;
  phNxpNciHal_Sem_t* getSemHandle();

 private:
  SecureDwnLoadAgent();
  SecureDwnLoadAgent(const SecureDwnLoadAgent&) = delete;
  SecureDwnLoadAgent& operator=(const SecureDwnLoadAgent&) = delete;
  uint16_t mFragmentLen;    /* Max packet data len set by platform   */
  const uint8_t* pFwImgPtr; /* Pointer to the fw file                */
  uint32_t mFwImgLen;       /* Length of the fw file                 */
  uint16_t mMaxFragmentLen; /* max firmware fragment length of pckt  */
  uint32_t mCurrIdx;        /*Starting location of commands to be sent*/
  uint16_t mFwVer;          /* Fw version extracted from the fw file */
  bool fwDnldForm;          /*Should be set to true in degraded fw dnld*/
  NFCSTATUS EDLDownload();
  static void EDLDownloadCallbck(void* pContext,
                                 phTmlNfc_TransactInfo_t* pInfo);
  static void EDLTimerExpired(union sigval val);
  NFCSTATUS sendEdlCmd();
  void printGetInfo();
  NFCSTATUS writeSync();
  getInfoResp mGetInfoResp;
  NFCSTATUS status;
  phNxpNciHal_Sem_t sem_cb_data;
};

/**
 * @brief DnldV2Builder is the class
 * which performs the fw dnld for next gen chips
 */
class DnldV2Builder {
 public:
  static DnldV2Builder& getInstance();
  void setParams(uint16_t fragmentLen, const uint8_t* fwImgPtr,
                 uint32_t fwImgLen, uint16_t fwVer);
  NFCSTATUS doFwDnld();
  NFCSTATUS getInfo();
  void setChipDetected(bool isChipDetected);
  bool getChipDetected();
  uint8_t getFwRomVersion();   /* should be called after getInfo */
  uint8_t getFwMajorVersion(); /* should be called after getInfo */
  uint8_t getHwVersion();      /* should be called after getInfo */
 private:
  bool isChipDetected; /*Set to true in teardown state, used to identify chip
                          type */
};

/** @} */