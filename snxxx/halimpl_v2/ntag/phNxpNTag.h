/******************************************************************************
 *
 *  Copyright 2025 NXP
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
#pragma once

/*include files*/
#include <phNfcNciConstants.h>
#include <phNfcStatus.h>
#include <phNfcTypes.h>
#include <phNxpNciHal_ext.h>
#include "IntervalTimer.h"
#include "NciDef.h"
#include "NfcExtension.h"
#include "phNxpNciHal.h"

#define NxpNTagInstance (NxpNTag::getInstance())

#define NTAG_DETECT_TIMER_VALUE 3
#define NTAG_NTF_ENABLE_STATE 0x01
#define NTAG_PRESENCE_CHECK_DEFAULT_CONF_VAL 13

struct NtagControl {
  /* Ntag UID value */
  std::vector<uint8_t> mNtagUid;
  /* Default discovery configuration */
  std::vector<uint8_t> mCurrentDiscCmd;
  /* stores Qpoll mode */
  uint8_t mQPOLLMode;

  /**
   * @brief maintains ntag status.
   * Bit-0 NTag activated state.
   * Bit-1 NTag presence check.
   * Bit-2 NTag read complete.
   * Bit-3 NTag removal status.
   * Bit-4 NTag replace QPoll status.
   * Bit-5 Detect timer status.
   * Bit-6 Presence check timer status.
   * Bit-7 Presence check timer timeout status.
   */
  uint8_t mNtagDetectStatus;
  /* Ntag fature enabled status */
  bool mNtagEnableRequest;
  /* Ntag ntf enable/disable command need to be send */
  bool isNTagNtfCmdReq;
  /* Ntag ntf enable/disable status */
  bool isNTagNtfEnabled;
  /* Timer used for tracking Detect/Removal Ntag status. */
  IntervalTimer gNTagTimer;
};

class NxpNTag {
 public:
  /**
   * @brief Get the singleton instance of NTAG.
   * @return NxpNTag* Pointer to the instance.
   */
  static NxpNTag& getInstance();

  /**
   * @brief Process NCI response/notification for ntag.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS phNxpNciHal_HandleNtagRspNtf(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief handle ntag command.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS phNxpNciHal_HandleNtagCmd(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief Check and update the NTag prop ntf configurtins.
   * @param None
   * @return None
   */
  void phNxpNciHal_disableNtagNtfConfig();

 private:
  NtagControl mNtagControl;

  constexpr static uint8_t NTAG_STATUS_SUCCESS = 0x00;
  constexpr static uint8_t NTAG_STATUS_FAILED = 0x01;

  constexpr static uint8_t NFC_RF_DISC_START = 0x01;
  constexpr static uint8_t NFC_RF_DISC_APPEND_QPOLL = 0x02;
  constexpr static uint8_t NFC_RF_DISC_REPLACE_QPOLL = 0x04;
  constexpr static uint8_t NFC_RF_DISC_RESTART = 0x08;

  constexpr static uint8_t NTAG_ACTIVATED_STATUS = 0x01;
  constexpr static uint8_t NTAG_PRESENCE_CHK_STATUS = 0x02;
  constexpr static uint8_t NTAG_READ_COMPLETE = 0x04;
  constexpr static uint8_t NTAG_REMOVAL_STATUS = 0x08;

  constexpr static uint8_t NTAG_REPLACE_QPOLL_STATUS = 0x10;
  constexpr static uint8_t NTAG_DETECT_TIMER_STATUS = 0x20;
  constexpr static uint8_t NTAG_PRESENCE_CHECK_TIMER_STATUS = 0x40;
  constexpr static uint8_t NTAG_PRESENCE_CHECK_TIMEOUT = 0x80;

  constexpr static uint8_t NTAG_DETECTION_OID = 0x03;
  constexpr static uint8_t NTAG_ENABLE_PROP_OID = 0x04;
  constexpr static uint8_t NTAG_DISABLE_PROP_OID = 0x05;
  constexpr static uint8_t NTAG_REMOVED_STATUS_OID = 0x06;
  constexpr static uint8_t NTAG_MODE_ENABLED_STATUS_OID = 0x07;

  constexpr static uint8_t PAYLOAD_TWO_LEN = 0x02;
  constexpr static uint8_t QTAG_FEATURE_SUB_GID = 0x30;

  constexpr static uint8_t NCI_TECH_A_POLL_VAL = 0x00;
  constexpr static uint8_t NCI_TECH_Q_POLL_VAL = 0x71;
  constexpr static uint8_t NTAG_LOAD_CHANGE_VAL = 0xA9;
  constexpr static uint8_t QTAG_ENABLE_OID = 0x01;

  /**
   * @brief Process NCI message for Ntag.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleNciMessage(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief send RF deactivate to idle command.
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS sendRfDeactivate();

  /**
   * @brief send RF discovery command..
   * @param state RF discovery with/without QPoll
   * @return returns NFCSTATUS_SUCCESS, on RF discovery commands is success
   *  else NFCSTATUS_FAILURE.
   */
  NFCSTATUS sendRfDiscCmd(uint8_t state);

  /**
   * @brief Send Prop ntf enbale/disable command .
   * @param flag True/False for enable/disable notification.
   * @return returns NFCSTATUS_SUCCESS, on command response  is success
   *  else NFCSTATUS_FAILURE.
   */
  NFCSTATUS sendNtagPropConfig(bool flag);

  /**
   * @brief validate UID value received from NFCC and stored UID value.
   * @param uid value received from NFCC
   * @param uidLength length of the UID value.
   * @return returns true if UID and length is matched with stored
   *  UID value else Flase.
   */
  bool isNtagUidChanged(const uint8_t* uid, uint8_t uidLength);

  /**
   * @brief Update NTag removed status.
   * @param None
   * @return None
   */
  void updateNTagRemoveStatus();

  /**
   * @brief Process RF Interface activated notification for UID.
   * @param rfIntfNtf RF Interface activated notification
   * @return returns true if the processed UID is matched with stored
   * UID value else Flase and stores UID value.
   */
  bool processNtagUid(const std::vector<uint8_t>& rfIntfNtf);

  /**
   * @brief Check and update the Ntag status.
   * @param None
   * @return None
   */
  void checkNTagRemoveStatus();

  /**
   * @brief Process RF discovery/idle command. If Ntag is not detected
   *        then appending the Qpoll and send to NFCC.
   * @param rfDiscCmd RF discovery command receoved from NFC service.
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internaly otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS processRfDiscCmd(vector<uint8_t>& rfDiscCmd);

  void handleTimers();
  static void QPollTimerTimeoutCallback(union sigval val);
  void QPollTimerStart(unsigned long millisecs);
  void QPollTimerStop();
  NxpNTag();
  ~NxpNTag();
};
