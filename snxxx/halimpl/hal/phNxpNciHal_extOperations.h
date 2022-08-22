/*
 * Copyright 2019-2022 NXP
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

#include <phNxpNciHal_ext.h>

#include "phNfcStatus.h"

#define AUTONOMOUS_SCREEN_OFF_LOCK_MASK 0x20
#define SWITCH_OFF_MASK 0x02
#define NCI_GET_CONFI_MIN_LEN 0x04
#define NXP_MAX_RETRY_COUNT 0x03
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

/*****************************************************************************
 * Function         phNxpNciHal_send_get_cfg
 *
 * Description      This function is called to get the configurations from
 * EEPROM
 *
 * Params           cmd_get_cfg, Buffer to GET command
 *                  cmd_len,     Length of the command
 * Returns          SUCCESS/FAILURE
 *
 *
 *****************************************************************************/
NFCSTATUS phNxpNciHal_send_get_cfg(const uint8_t* cmd_get_cfg, long cmd_len);

/*****************************************************************************
 * Function         phNxpNciHal_configure_merge_sak
 *
 * Description      This function is called to apply iso_dep sak merge settings
 *                  as per the config option NAME_NXP_ISO_DEP_MERGE_SAK
 *
 * Params           None

 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
 *
 *****************************************************************************/
NFCSTATUS phNxpNciHal_configure_merge_sak();
/******************************************************************************
 * Function         phNxpNciHal_setSrdtimeout
 *
 * Description      This function can be used to set srd SRD Timeout.
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS or
 *                  NFCSTATUS_FEATURE_NOT_SUPPORTED
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setSrdtimeout();
/******************************************************************************
 * Function         phNxpNciHal_set_uicc_hci_params
 *
 * Description      This will update value of uicc session status to store flag
 *                  to eeprom
 *
 * Parameters       value - this value will be updated to eeprom flag.
 *
 * Returns          status of the write
 *
 ******************************************************************************/
NFCSTATUS
phNxpNciHal_set_uicc_hci_params(std::vector<uint8_t>& ptr, uint8_t bufflen,
                                phNxpNci_EEPROM_request_type_t uiccType);
/******************************************************************************
 * Function         phNxpNciHal_get_uicc_hci_params
 *
 * Description      This will read the value of fw download status flag
 *                  from eeprom
 *
 * Parameters       value - this parameter will be updated with the flag
 *                  value from eeprom.
 *
 * Returns          status of the read
 *
 ******************************************************************************/
NFCSTATUS
phNxpNciHal_get_uicc_hci_params(std::vector<uint8_t>& ptr, uint8_t bufflen,
                                phNxpNci_EEPROM_request_type_t uiccType);

/******************************************************************************
 * Function         phNxpNciHal_setExtendedFieldMode
 *
 * Description      This function can be used to set nfcc extended field mode
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS or
 *                  NFCSTATUS_FEATURE_NOT_SUPPORTED
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setExtendedFieldMode();

/*******************************************************************************
**
** Function         phNxpNciHal_configGPIOControl()
**
** Description      Helper function to configure GPIO control
**
** Parameters       gpioControl - Byte array with first two bytes are used to
**                  configure gpio for specific functionality (ex:ULPDET,
**                  GPIO LEVEL ...) and 3rd byte indicates the level of GPIO
**                  to be set.
**                  len        - Len of byte array
**
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/
NFCSTATUS phNxpNciHal_configGPIOControl(uint8_t gpioControl[], uint8_t len);

/*******************************************************************************
**
** Function         phNxpNciHal_isULPDetSupported()
**
** Description      this function is to check ULPDet feature is supported or not
**
** Returns          true or false
*******************************************************************************/
bool phNxpNciHal_isULPDetSupported();

/*******************************************************************************
**
** Function         phNxpNciHal_setULPDetFlag()
**
** Description      this function is called by Framework API to set ULPDet mode
**                  enable/disable
**
** Parameters       flag - true to enable ULPDet, false to disable
**
** Returns          true or false
*******************************************************************************/
void phNxpNciHal_setULPDetFlag(bool flag);

/*******************************************************************************
**
** Function         phNxpNciHal_getULPDetFlag()
**
** Description      this function get the ULPDet state, true if it is enabled
**                  false if it is disabled
**
** Returns          true or false
*******************************************************************************/
bool phNxpNciHal_getULPDetFlag();

/*******************************************************************************
**
** Function         phNxpNciHal_propConfULPDetMode()
**
** Description      this function applies the configurations to enable/disable
**                  ULPDet Mode
**
** Parameters       bEnable - true to enable, false to disable
**
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/
NFCSTATUS phNxpNciHal_propConfULPDetMode(bool bEnable);