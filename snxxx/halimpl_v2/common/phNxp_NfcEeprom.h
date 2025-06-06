/*
 * Copyright 2025 NXP
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

#ifndef PHNXP_NFCEEPROM_H
#define PHNXP_NFCEEPROM_H

#include <stdint.h>

#define GET_EEPROM_DATA (1U)
#define SET_EEPROM_DATA (2U)

typedef enum {
  EEPROM_RF_CFG,
  EEPROM_FW_DWNLD,
  EEPROM_WIREDMODE_RESUME_TIMEOUT,
  EEPROM_ESE_SVDD_POWER,
  EEPROM_ESE_POWER_EXT_PMU,
  EEPROM_PROP_ROUTING,
  EEPROM_ESE_SESSION_ID,
  EEPROM_SWP1_INTF,
  EEPROM_SWP1A_INTF,
  EEPROM_SWP2_INTF,
  EEPROM_FLASH_UPDATE,
  EEPROM_AUTH_CMD_TIMEOUT,
  EEPROM_GUARD_TIMER,
  EEPROM_T4T_NFCEE_ENABLE,
  EEPROM_AUTONOMOUS_MODE,
  EEPROM_CE_PHONE_OFF_CFG,
  EEPROM_ENABLE_VEN_CFG,
  EEPROM_ISODEP_MERGE_SAK,
  EEPROM_SRD_TIMEOUT,
  EEPROM_UICC1_SESSION_ID,
  EEPROM_UICC2_SESSION_ID,
  EEPROM_CE_ACT_NTF,
  EEPROM_UICC_HCI_CE_STATE,
  EEPROM_EXT_FIELD_DETECT_MODE,
  EEPROM_CONF_GPIO_CTRL,
  EEPROM_SET_GPIO_VALUE,
  EEPROM_POWER_TRACKER_ENABLE,
  EEPROM_VDDPA,
  EEPROM_INTERPOLATED_RSSI_8AM,
} phNxpNci_EEPROM_request_type_t;

typedef struct phNxpNci_EEPROM_info {
  uint8_t request_mode;
  phNxpNci_EEPROM_request_type_t request_type;
  uint8_t update_mode;
  uint8_t* buffer;
  uint8_t bufflen;
} phNxpNci_EEPROM_info_t;

NFCSTATUS request_EEPROM(phNxpNci_EEPROM_info_t* mEEPROM_info);

#endif //PHNXP_NFCEEPROM_H