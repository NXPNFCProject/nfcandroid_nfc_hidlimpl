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
#include <NxpNfcThreadMutex.h>
#include <phNfcNciConstants.h>
#include <phNfcStatus.h>
#include <phNfcTypes.h>
#include <phNxpNciHal_ext.h>

#include "IntervalTimer.h"
#include "NciDef.h"
#include "NfcExtension.h"
#include "phNxpNciHal.h"

#define NTAG_DETECT_TIMER_VALUE 3
#define NTAG_NTF_ENABLE_STATE 0x01
#define NTAG_PRESENCE_CHECK_DEFAULT_CONF_VAL 13
#define DEFAULT_NTAG_SUPPORT_MIN_FW_VER 0x02204A

enum class NTagSetSubState : uint8_t {
  /* Initial state, no operation in progress */
  NTAG_SET_SUB_STATE_IDLE,
  NTAG_SET_SUB_STATE_WAIT_FOR_RF_IDLE_RSP,
  NTAG_SET_SUB_STATE_WAIT_FOR_PROP_CMD_RSP,
  NTAG_SET_SUB_STATE_WAIT_FOR_RF_DISC_RSP,
};

enum class NTagState : uint8_t {
  /* Initial state, no operation in progress */
  NTAG_STATE_IDLE,
  /* NTag state enabled */
  NTAG_STATE_ENABLE,
  /* NTag state disabled */
  NTAG_STATE_DISABLE,
  /* RF discovery for NTag */
  NTAG_STATE_RF_DISCOVERY,
  /* RF deactivation for NTag */
  NTAG_STATE_RF_DEACTIVATE_IDLE,
  /* NTag detected */
  NTAG_STATE_SAME_UID_DETECTED,
  /* NTag presence check is on going */
  NTAG_STATE_PRESENCE_CHECK,
  /* Max State */
  NTAG_STATE_MAX,
};

enum class NTagEvent : uint8_t {
  ACTION_NTAG_ENABLE_REQUEST,
  ACTION_NTAG_DISABLE_REQUEST,
  ACTION_NTAG_PROP_NTF_SET_STATUS,
  ACTION_NTAG_RF_INTF_ACTIVATED,
  ACTION_NTAG_RF_DEACTIVATE_IDLE,
  ACTION_NTAG_RF_DISCOVERY,
  ACTION_NTAG_RF_LOAD_CHANGE_NTF,
  ACTION_NTAG_PRESENCE_CHECK,
  ACTION_NTAG_UID_MATCHED,
  ACTION_NTAG_REMOVAL_DETECTED
};

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
  /* Ntag feature enabled status */
  bool mNtagEnableRequest;
  /* Ntag ntf enable/disable command need to be send */
  bool isNTagNtfCmdReq;
  /* Ntag ntf enable/disable status */
  bool isNTagNtfEnabled;
  /* Timer used for tracking Detect/Removal Ntag status. */
  IntervalTimer mNTagTimer;
  /* Response status for RF Idle/Discovery commands */
  NFCSTATUS mCmdRspStatus;
};

class NxpNTag {
 public:
  /**
   * @brief Get the singleton instance of NTAG.
   * @return NxpNTag* Pointer to the instance.
   */
  static NxpNTag* getInstance();

  /**
   * @brief Releases all the resources
   * @return None
   *
   */
  static inline void finalize() {
    if (sNxpNTag != nullptr) {
      delete (sNxpNTag);
      sNxpNTag = nullptr;
    }
  }

  /**
   * @brief Process NCI response/notification for ntag.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleVendorNciRspNtf(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief handle ntag command.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleVendorNciMessage(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief Check and update the NTag prop ntf configurations.
   * @param None
   * @return None
   */
  static void phNxpNciHal_disableNtagNtfConfig();

 private:
  static NxpNTag* sNxpNTag;
  NtagControl mNtagControl;
  NTagState mNTagState;
  NTagSetSubState mNTagSetSubState;
  NfcHalThreadCondVar mNTagDiscRspCv;
  bool mWaitingforDiscRsp = false;

  constexpr static uint8_t NTAG_STATUS_SUCCESS = 0x00;
  constexpr static uint8_t NTAG_STATUS_FAILED = 0x01;

  constexpr static uint8_t NFC_RF_DISC_START = 0x01;
  constexpr static uint8_t NFC_RF_DISC_REPLACE_QPOLL = 0x02;
  constexpr static uint8_t NFC_RF_DISC_RESTART = 0x04;

  constexpr static uint8_t NTAG_ACTIVATED_STATUS = 0x01;
  constexpr static uint8_t NTAG_PRESENCE_CHK_STATUS = 0x02;
  constexpr static uint8_t NTAG_READ_COMPLETE = 0x04;
  constexpr static uint8_t NTAG_REMOVAL_STATUS = 0x08;

  constexpr static uint8_t NTAG_DETECT_TIMER_STATUS = 0x10;
  constexpr static uint8_t NTAG_PRESENCE_CHECK_TIMER_STATUS = 0x20;
  constexpr static uint8_t NTAG_PRESENCE_CHECK_TIMEOUT = 0x40;

  constexpr static uint8_t NTAG_DETECTION_OID = 0x03;
  constexpr static uint8_t NTAG_ENABLE_PROP_OID = 0x04;
  constexpr static uint8_t NTAG_DISABLE_PROP_OID = 0x05;
  constexpr static uint8_t NTAG_REMOVED_STATUS_OID = 0x06;
  constexpr static uint8_t NTAG_MODE_ENABLED_STATUS_OID = 0x07;

  constexpr static uint8_t PAYLOAD_TWO_LEN = 0x02;
  constexpr static uint8_t QTAG_FEATURE_SUB_GID = 0x30;
  constexpr static uint8_t QTAG_ENABLE_OID = 0x01;
  constexpr static uint8_t NCI_TECH_Q_POLL_VAL = 0x71;

  /**
   * @brief Update the current state of the NTag.
   * @param state New state to update.

   */
  void updateState(NTagState state);

  /**
   * @brief Get the current state of the NTag.
   * @param None

   */
  NTagState getState() const { return mNTagState; }

  /**
   * @brief Process NTag event based on the current state.
   * @param NTag event.
   * @return returns NFCSTATUS_SUCCESS, if it is processed successfully
   *  else returns NFCSTATUS_FAILED.
   */
  NFCSTATUS processNTagEvent(NTagEvent event);

  /**
   * @brief Process NTag event based on the current sub state.
   * @param Current NTag event.
   * @return returns NFCSTATUS_SUCCESS, if it is processed successfully
   *  else returns NFCSTATUS_FAILED.
   */
  NFCSTATUS processNTagSetSubState(NTagEvent event);

  /**
   * @brief Process NCI message for Ntag enable, disable and get enable status.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS setNTagMode(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief handle NTag RF proprietary load change notification.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleNTagPropNtf(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief send RF deactivate to idle command.
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS sendRfDeactivate();

  /**
   * @brief send RF discovery command..
   * @param pollMode RF discovery with/without QPoll
   * @return returns NFCSTATUS_SUCCESS, on RF discovery commands is success
   *  else NFCSTATUS_FAILURE.
   */
  NFCSTATUS sendRfDiscCmd(uint8_t pollMode);

  /**
   * @brief Send Prop ntf enable/disable command .
   * @param flag True/False for enable/disable notification.
   * @return returns NFCSTATUS_SUCCESS, on command response  is success
   *  else NFCSTATUS_FAILURE.
   */
  NFCSTATUS sendNTagPropConfig(bool flag);

  /**
   * @brief validate UID value received from NFCC and stored UID value.
   * @param uid value received from NFCC
   * @return returns true if UID is matched and read completed with stored
   *  UID value else false.
   */
  bool isNTagReadComplete(const std::vector<uint8_t>& uid);

  /**
   * @brief Process NTag detected notification.
   * @param None
   * @return None
   */
  void processNTagDetectNtf(const std::vector<uint8_t>& uid);

  /**
   * @brief Update NTag removed status.
   * @param None
   * @return None
   */
  void updateNTagRemoveStatus();

  /**
   * @brief Process RF interface activated notification
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleRfIntfActivated(uint8_t* pData, uint16_t dataLen);
  /**
   * @brief Check and update the Ntag status.
   * @param None
   * @return None
   */
  void checkNTagRemoveStatus();

  /**
   * @brief Process RF discovery/idle command. If Ntag is not detected
   *        then appending the Qpoll and send to NFCC.
   * @param rfDiscCmd RF discovery command received from NFC service.
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS processRfDiscCmd(vector<uint8_t>& rfDiscCmd);

  /**
   * @brief Process NTag NCi responses
   * @param pData Pointer to the data
   * @param dataLen Length of the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleNTagNciRsp(uint8_t* pData, uint16_t dataLen);

  /**
   * @brief Process RF interface activated notification
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleNTagNciNtf(uint8_t* pData, uint16_t dataLen);

  /**
   * @brief Process NTag presence check response
   * @return None
   */
  void handleNTagPresenceCheckRsp();

  /**
   * @brief Process NTag presence check response
   * @param pData Pointer to the data
   * @return None
   */
  void handleNTagPresenceCheckNtf(uint8_t* pData);

  /**
   * @brief reset NTag active timer and update the status
   * @return None
   */
  void stopNTagTimerInternal();

  /**
   * @brief wait for discovery response after sending RF Idle.
   * if RF Idle sent and still discovery rsp not received then
   * default RF discovery command will be updated.
   * @return None
   */
  NFCSTATUS waitForRfDiscRsp(NFCSTATUS status);

  /**
   * @brief check the chipset is supporting Ntag feature or not.
   * @return True/False
   */
  static bool isNtagSupported();

  static void QPollTimerTimeoutCallback(union sigval val);
  bool QPollTimerStart(unsigned long sec);
  void QPollTimerStop();
  void clearNTagFlags();
  NxpNTag();
  ~NxpNTag();
};
