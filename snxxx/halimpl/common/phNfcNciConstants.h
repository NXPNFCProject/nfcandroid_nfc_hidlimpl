/*
 * Copyright 2024 NXP
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

/* NCI GID index*/
#define NCI_GID_INDEX 0
/* NCI OID index*/
#define NCI_OID_INDEX 1
/* NCI message length index*/
#define NCI_MSG_LEN_INDEX 2
/* NCI message index for feature*/
#define NCI_MSG_INDEX_FOR_FEATURE 3
/* NCI message index feature value*/
#define NCI_MSG_INDEX_FEATURE_VALUE 4
#define NCI_GID_PROP 0x0F
#define NCI_HEADER_MIN_LEN 3  // GID + OID + LENGTH
#define RF_DISC_CMD_NO_OF_CONFIG_INDEX 3
#define RF_DISC_CMD_CONFIG_START_INDEX 4
// RF tech mode and Disc Frequency values
#define RF_DISC_CMD_EACH_CONFIG_LENGTH 2

/* Android Parameters */
#define NCI_ANDROID_POWER_SAVING 0x01
#define NCI_ANDROID_OBSERVER_MODE 0x02
#define NCI_ANDROID_GET_OBSERVER_MODE_STATUS 0x04

/* Android Power Saving Params */
#define NCI_ANDROID_POWER_SAVING_PARAM_SIZE 2
#define NCI_ANDROID_POWER_SAVING_PARAM_DISABLE 0
#define NCI_ANDROID_POWER_SAVING_PARAM_ENABLE 1

#define NCI_RSP_SIZE 2
#define NCI_RSP_OK 0
#define NCI_RSP_FAIL 3

#define NCI_PROP_NTF_GID 0x6F
#define NCI_PROP_LX_NTF_OID 0x36
#define NCI_PROP_NTF_ANDROID_OID 0x0C

#define NCI_RF_DISC_COMMD_GID 0x21
#define NCI_RF_DISC_COMMAND_OID 0x03
#define NFC_A_PASSIVE_LISTEN_MODE 0x80
#define NFC_B_PASSIVE_LISTEN_MODE 0x81
#define NFC_F_PASSIVE_LISTEN_MODE 0x82
#define NFC_ACTIVE_LISTEN_MODE 0x83
#define OBSERVE_MODE 0xFF
#define OBSERVE_MODE_DISCOVERY_CYCLE 0x01

// Observe mode constants
#define L2_EVT_TAG 0x01
#define CMA_EVT_TAG 0x0A
#define CMA_EVT_EXTRA_DATA_TAG 0x07
#define MIN_LEN_NON_CMA_EVT 7
#define MIN_LEN_CMA_EVT 6
#define INDEX_OF_L2_EVT_TYPE 6
#define INDEX_OF_L2_EVT_GAIN 5
#define INDEX_OF_CMA_EVT_TYPE 4
#define INDEX_OF_CMA_EVT_DATA 5
#define INDEX_OF_CMA_DATA 7
#define MIN_LEN_NON_CMA_EVT 7
#define MIN_LEN_CMA_EVT 6
#define MIN_LEN_CMA_EXTRA_DATA_EVT 1
#define L2_EVENT_TRIGGER_TYPE 0x1
#define CMA_EVENT_TRIGGER_TYPE 0x02
#define CMA_DATA_TRIGGER_TYPE 0x0E
// Event types to send upper layer
#define TYPE_RF_FLAG 0x00
#define TYPE_MOD_A 0x01
#define TYPE_MOD_B 0x02
#define TYPE_MOD_F 0x03
#define TYPE_UNKNOWN 0x07
#define EVENT_UNKNOWN 0x00
#define EVENT_MOD_A 0x01
#define EVENT_MOD_B 0x02
#define EVENT_MOD_F 0x03
#define EVENT_RF_ON 0x08
#define EVENT_RF_OFF 0x09
#define REQ_A 0x26
#define WUP_A 0x52
#define TYPE_B_APF 0x05
#define TYPE_F_CMD_LENGH 0x06
#define TYPE_F_ID 0xFF
#define OBSERVE_MODE_OP_CODE 0x03
#define GAIN_FIELD_LENGTH 1
#define OP_CODE_FIELD_LENGTH 1
#define EVENT_TYPE_FIELD_LENGTH 1
#define RF_STATE_FIELD_LENGTH 1
#define NCI_MESSAGE_OFFSET 3
#define LX_TYPE_MASK 0x0F
#define LX_EVENT_MASK 0xF0
#define LX_LENGTH_MASK 0x0F
#define LX_TAG_MASK 0xF0
#define SHORT_FLAG 0x00
