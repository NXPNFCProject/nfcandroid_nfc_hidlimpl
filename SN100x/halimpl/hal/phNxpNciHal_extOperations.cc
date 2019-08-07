/*
 * Copyright (C) 2019 NXP Semiconductors
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

#include "phNxpNciHal_extOperations.h"
#include "phNfcCommon.h"
#include <phNxpLog.h>
#include <phNxpNciHal_ext.h>

nxp_nfc_config_ext_t config_ext;
/******************************************************************************
 * Function         phNxpNciHal_updateAutonomousPwrState
 *
 * Description      This function can be used to update autonomous pwr state.
 *                  num: value to check  switch off bit is set or not.
 *
 * Returns          uint8_t
 *
 ******************************************************************************/
uint8_t phNxpNciHal_updateAutonomousPwrState(uint8_t num) {
  if ((config_ext.autonomous_mode == true) &&
      ((num & SWITCH_OFF_MASK) == SWITCH_OFF_MASK)) {
    num = (num | AUTONOMOUS_SCREEN_OFF_LOCK_MASK);
  }
  return num;
}
/******************************************************************************
 * Function         phNxpNciHal_setAutonomousMode
 *
 * Description      This function can be used to set NFCC in autonomous mode
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
 *                  or NFCSTATUS_FEATURE_NOT_SUPPORTED
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setAutonomousMode() {

  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  uint8_t autonomous_mode_value = 0x01;
  if (config_ext.autonomous_mode == true)
    autonomous_mode_value = 0x02;

  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  mEEPROM_info.buffer = (uint8_t *)&autonomous_mode_value;
  mEEPROM_info.bufflen = sizeof(autonomous_mode_value);
  mEEPROM_info.request_type = EEPROM_AUTONOMOUS_MODE;

  return request_EEPROM(&mEEPROM_info);
}
/******************************************************************************
 * Function         phNxpNciHal_setGuardTimer
 *
 * Description      This function can be used to set nfcc Guard timer
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setGuardTimer() {

  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};

  if (config_ext.autonomous_mode != true)
    config_ext.guard_timer_value = 0x00;

  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  mEEPROM_info.buffer = &config_ext.guard_timer_value;
  mEEPROM_info.bufflen = sizeof(config_ext.guard_timer_value);
  mEEPROM_info.request_type = EEPROM_GUARD_TIMER;

  return request_EEPROM(&mEEPROM_info);
}
