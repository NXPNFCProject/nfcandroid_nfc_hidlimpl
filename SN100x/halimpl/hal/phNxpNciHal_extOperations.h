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

#pragma once

#include "phNfcStatus.h"

#define AUTONOMOUS_SCREEN_OFF_LOCK_MASK 0x20
#define SWITCH_OFF_MASK 0x02
typedef struct {
  uint8_t autonomous_mode;
  uint8_t guard_timer_value;
} nxp_nfc_config_ext_t;
extern nxp_nfc_config_ext_t config_ext;

/******************************************************************************
 * Function         phNxpNciHal_updateAutonomousPwrState
 *
 * Description      This function can be used to update autonomous pwr state.
 *                  num: value to check  switch off bit is set or not.
 *
 * Returns          uint8_t
 *
 ******************************************************************************/
uint8_t phNxpNciHal_updateAutonomousPwrState(uint8_t num);
/******************************************************************************
 * Function         phNxpNciHal_setAutonomousMode
 *
 * Description      This function can be used to set NFCC in autonomous mode
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setAutonomousMode();

/******************************************************************************
 * Function         phNxpNciHal_setGuardTimer
 *
 * Description      This function can be used to set Guard timer
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setGuardTimer();
