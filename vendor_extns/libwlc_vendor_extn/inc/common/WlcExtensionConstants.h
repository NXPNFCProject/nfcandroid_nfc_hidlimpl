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

#ifndef WLC_EXTENSION_CONSTANTS_H
#define WLC_EXTENSION_CONSTANTS_H

#include <cstdint>

/** \addtogroup WLC_EXTENSION_CONSTANTS_INTERFACE
 *  @brief  interface for wlc extension specific constants and error codes.
 *  @{
 */

#define WLC_NXP_GEN_EXT_VERSION_MAJ (0x04) /* MW Major Version */
#define WLC_NXP_GEN_EXT_VERSION_MIN (0x00) /* MW Minor Version */

/**
 * @brief returns UN_SUPPORTED_FEATURE code to framework, if HAL is not
 *        supporting the vendor specific message
 *
 */
constexpr uint8_t UN_SUPPORTED_FEATURE = -1;

/**
 * @brief returns SUPPPORTED_FEATURE code to framework, if HAL
 *        supports the vendor specific message
 *
 */
constexpr uint8_t SUPPPORTED_FEATURE = 0;

/**
 * @brief returns ERROR code to framework, if HAL
 *        does not able to process the request because mPOS on going
 *
 */
constexpr uint8_t ERROR = 1;

/**
 * @brief HAL gives this error code when write
 *        response times out
 *
 */
constexpr uint8_t WRITE_RSP_TIMED_OUT = 2;
/**
 * @brief NCI 2.0 Response Status Codes
 *
 */
constexpr uint8_t RESPONSE_STATUS_OK = 0x00;
constexpr uint8_t RESPONSE_STATUS_FAILED = 0x03;
constexpr uint8_t RESPONSE_STATUS_OPERATION_NOT_SUPPORTED = 0x0B;

constexpr uint8_t EXT_WLC_STATUS_REJECTED = 0x01;
constexpr uint8_t EXT_WLC_STATUS_INDEX = 0x03;
constexpr uint8_t EXT_WLC_STATUS_OK = 0x00;
constexpr uint8_t EXT_WLC_STATUS_SEMANTIC_ERROR = 0x06;

constexpr uint8_t NCI_MT_CMD = 0x20;
constexpr uint8_t NCI_MT_RSP = 0x40;
constexpr uint8_t NCI_MT_NTF = 0x60;
constexpr uint8_t NCI_OID_MASK = 0x3F;
constexpr uint8_t NCI_MSG_CORE_RESET = 0x00;

constexpr uint8_t MT_CMD = 0x00;
constexpr uint8_t MT_RSP = 0x01;
constexpr uint8_t MT_NTF = 0x02;
constexpr uint8_t MIN_HED_LEN = 0x03;
constexpr uint16_t NCI_MAX_DATA_LEN = 300;

constexpr uint8_t NCI_GID_CORE = 0x00;
constexpr uint8_t NCI_GID_RF_MANAGE = 0x01;
constexpr uint8_t NCI_GID_EE_MANAGE = 0x02;

constexpr uint8_t DEFAULT_RESP = 0xFF;
constexpr uint8_t ZERO_PAYLOAD = 0x00;
constexpr uint8_t PAYLOAD_TWO_LEN = 0x02;

constexpr uint8_t SUB_GID_OID_INDEX = 3;
constexpr uint8_t SUB_GID_OID_ACTION_INDEX = 4;
constexpr uint8_t SUB_GID_MASK = 0xF0;
constexpr uint8_t SUB_OID_MASK = 0x0F;
constexpr uint8_t NCI_PROP_ACTION_INDEX = 0x04;
constexpr uint8_t SUB_GID_OID_RSP_STATUS_INDEX = 0x04;

constexpr uint8_t NCI_GID_INDEX = 0;
constexpr uint8_t NCI_OID_INDEX = 1;
constexpr uint8_t EXT_NCI_GID_MASK = 0x0F;
constexpr uint8_t EXT_NCI_OID_MASK = 0xFF;

constexpr uint8_t NCI_PROP_CMD_VAL = 0x2F;
constexpr uint8_t NCI_PROP_RSP_VAL = 0x4F;
constexpr uint8_t NCI_PROP_NTF_VAL = 0x6F;
constexpr uint8_t SEND_SRAM_CMD_TIMEOUT_SEC = 2;

constexpr uint8_t NCI_ROW_PROP_OID_VAL = 0x70;
constexpr uint8_t NCI_WLC_PROP_OID_VAL = 0x71;
constexpr uint8_t NCI_OEM_PROP_OID_VAL = 0x3E;
constexpr uint8_t RF_REGISTER_SUB_GIDOID = 0x72;
constexpr uint8_t NCI_PROP_NTF_SYSTEM_GENERIC_INFO_OID = 0x0F;

constexpr uint8_t NCI_UN_RECOVERABLE_ERR = 0x01;
constexpr uint8_t NCI_HAL_CONTROL_BUSY = 0x02;
constexpr uint8_t NCI_HAL_CONTROL_NOT_RELEASED = 0x03;
constexpr uint8_t NCI_UN_SUPPORTED_FEATURE = 0x04;

constexpr uint8_t NCI_WLC_PROP_SUB_GID_VAL          = 0x02;
constexpr uint8_t NCI_WLC_SEND_RAW_CMD_SUB_OID      = 0x00;
constexpr uint8_t NCI_WLC_STATUS_NTF_SUB_OID        = 0x01;
constexpr uint8_t NCI_WLC_OBS_DATA_PKT_CMD_SUB_OID  = 0x02;
constexpr uint8_t NCI_WLC_RF_INTF_ACT_NTF_SUB_OID   = 0x03;
constexpr uint8_t NCI_WLC_RF_INTF_DEACT_NTF_SUB_OID = 0x04;

constexpr uint8_t NCI_PAYLOAD_LEN_INDEX = 2;
constexpr uint8_t MIN_PCK_MSG_LEN = 0x03;
constexpr uint8_t NCI_TECH_A_POLL_VAL = 0x00;
constexpr uint8_t NCI_TECH_Q_POLL_VAL = 0x71;
constexpr uint8_t NCI_RF_INTF_ACT_TECH_TYPE_INDEX = 6;
constexpr uint8_t NCI_MSG_TYPE_INDEX = 0;
constexpr uint8_t NCI_OID_TYPE_INDEX = 1;
constexpr uint8_t NCI_SHIFT_BY_4 = 4;
constexpr uint8_t NCI_HEADER_LEN = 0x03;
constexpr uint8_t NCI_SHIFT_BY_8 = 8;
constexpr uint8_t NCI_MSG_TYPE_CMD = 0x01;
constexpr uint8_t NCI_MSG_TYPE_DATA = 0x00;
constexpr uint8_t NCI_RF_INTF_VAL_INDEX = 0x04;
constexpr uint8_t NCI_RF_INTF_WLC_AUTON_VAL = 0x07;

constexpr uint8_t NCI_CORE_SET_CFG_OID_VAL = 0x02;

constexpr uint8_t DISABLE_LOG = 0x00;
constexpr uint8_t ENABLE_LOG = 0x01;

constexpr uint16_t NCI_DISC_REQ_NTF_GID_OID = 0x610A;
constexpr uint16_t NCI_SET_CONFIG_RSP_GID_OID = 0x4002;
constexpr uint16_t NCI_DISC_MAP_RSP_GID_OID = 0x4100;
constexpr uint16_t NCI_RF_DISC_STOP_RSP_GID_OID = 0x4106;
constexpr uint16_t NCI_RF_DISC_CMD_GID_OID = 0x2103;
constexpr uint16_t NCI_RF_SET_CONFIG_CMD_GID_OID = 0x2002;
constexpr uint16_t NCI_RF_DISC_RSP_GID_OID = 0x4103;
constexpr uint16_t NCI_RF_INTF_ACTD_NTF_GID_OID = 0x6105;
constexpr uint16_t NCI_RF_DEACTD_NTF_GID_OID = 0x6106;
constexpr uint16_t NCI_RF_DEACTD_RSP_GID_OID = 0x4106;
constexpr uint16_t NCI_GENERIC_ERR_NTF_GID_OID = 0x6007;
constexpr uint16_t NCI_DATA_PKT_RSP_GID_OID = 0x0100;
constexpr uint16_t NCI_EE_DISC_NTF_GID_OID = 0x6200;
constexpr uint16_t NCI_SCREEN_SUB_STATE_CMD_GID_OID = 0x2009;
constexpr uint16_t NCI_SCREEN_SUB_STATE_RSP_GID_OID = 0x4009;
constexpr uint16_t NCI_RF_DISC_MAP_CMD_GID_OID = 0x2100;
constexpr uint16_t NCI_RF_DISC_STOP_CMD_GID_OID = 0x2106;
constexpr uint16_t NCI_EE_PWR_LINK_CMD_GID_OID = 0x2203;
constexpr uint16_t NCI_EE_PWR_LINK_RSP_GID_OID = 0x4203;
constexpr uint16_t NCI_RF_PLL_UNLOCK_NTF_GID_OID = 0x6121;
constexpr uint16_t NCI_RF_DISC_NTF_GID_OID = 0x6103;
constexpr uint16_t NCI_CORE_RESET_NTF_GID_OID = 0x6000;
constexpr uint16_t NCI_PROP_WLC_STATUS_NTF_GID_OID = 0x6127;
constexpr uint16_t NCI_ROW_WLC_STATUS_NTF_GID_OID = 0x6113;
constexpr uint16_t NCI_CORE_INTERFACE_ERROR_NTF_GID_OID = 0x6008;
constexpr uint16_t NCI_DATA_PKT_GID_OID = 0x0000;


constexpr uint16_t NCI_FW_MAJOR_VER_INDEX = 0x0A;
constexpr uint16_t NCI_FW_MINOR_VER_INDEX = 0x0B;
constexpr uint16_t NCI_FW_PATCH_VER_INDEX = 0x0C;
constexpr uint16_t NCI_MIN_CORE_RESET_NTF_LEN = 0x0D;

constexpr uint8_t NCI_CORE_RESET_OID = 0x00;
constexpr uint8_t NCI_CORE_INIT_OID = 0x01;
constexpr uint8_t NCI_EE_MODE_SET_OID = 0x01;
constexpr uint8_t NCI_EE_STATUS_OID = 0x02;
constexpr uint8_t NCI_POWER_LINK_OID = 0x03;
constexpr uint8_t NCI_RF_DISCOVERY_OID = 0x03;
constexpr uint8_t NCI_RF_INTF_ACT_OID = 0x05;
constexpr uint8_t NCI_RF_DEACTIVATE_OID = 0x06;
constexpr uint8_t NCI_CORE_GENERIC_ERROR_OID = 0x07;
constexpr uint8_t NCI_EE_ACTION_OID = 0x09;
constexpr uint8_t NCI_CORE_SET_POWER_SUB_STATE_OID = 0x09;
/* Core generic error for TAG collision detected */
constexpr uint8_t NCI_TAG_COLLISION_DETECTED = 0xE4;

constexpr uint8_t NCI_RF_DISC_NTF_PROTOCOL_INDEX = 4;
constexpr uint8_t NCI_RF_ID_INDEX = 3;

/**
 * @brief MAINLINE related constants
 *
 */
constexpr uint8_t MAINLINE_MIN_CMD_LEN = 0x04;

constexpr uint8_t NCI_CMD_TIMEOUT_IN_SEC = 2;
static const int NXP_EXTNS_WRITE_RSP_TIMEOUT_IN_MS = 2000;
static const int NXP_EXTNS_HAL_REQUEST_CTRL_TIMEOUT_IN_MS = 1000;
/**
 * @brief WLC FORUM LISTEN TECH
 *
 */
constexpr uint8_t NCI_TYPE_A_LISTEN = 0x80;
constexpr uint8_t NCI_TYPE_B_LISTEN = 0x81;
constexpr uint8_t NCI_TYPE_F_LISTEN = 0x82;

constexpr uint8_t EXT_WLC_HAL_OPEN_CPLT_EVT = 0;
constexpr uint8_t EXT_WLC_HAL_CLOSE_CPLT_EVT = 1;
constexpr uint8_t EXT_HAL_DEF_EVT_STATUS = 0;
constexpr uint8_t HAL_WLC_FW_UPDATE_STATUS_EVT = 0x0A;
constexpr uint8_t HAL_EVT_STATUS_INVALID = 0xFF;
constexpr uint8_t STATUS_FW_DL_REQUEST = 0x04;
constexpr uint8_t HAL_WLC_ERROR_EVT = 0x06;

extern uint8_t EXT_WLC_ADAPTATION_INIT;
extern uint8_t EXT_WLC_PRE_DISCOVER;

constexpr uint8_t NCI_EE_MODE_SET_ACTIVATE_VAL = 0x01;
constexpr uint8_t NCI_EE_MODE_SET_DEACTIVATE_VAL = 0x00;
constexpr uint8_t NCI_CORE_SET_CONF_CON_DISCOVERY_PARAM = 0x02;

/**
 * @brief Defines the state of the Handler
 *
 */
enum class HandlerState {
  /**
   * @brief indicates state has been started
   */
  STARTED,
  /**
   * @brief indicates state has been stopped
   */
  STOPPED
};

/**
 * @brief Defines the state of WLC HAL module
 *
 */
enum class WlcHalState {
  /**
   * @brief indicates Wlc HAL is closed
   */
  CLOSE,
  /**
   * @brief indicates Wlc HAL is opened fully
   */
  OPEN,
  /**
   * @brief indicates Wlc HAL is partially opened.
   */
  MIN_OPEN
};

/** @}*/
#endif // WLC_EXTENSION_CONSTANTS_H
