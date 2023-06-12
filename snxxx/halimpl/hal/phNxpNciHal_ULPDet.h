/*
 * Copyright 2022-2023 NXP
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

#include <phNxpNciHal_ext.h>

#define ENABLE_ULPDET_USING_API 0x01
#define ENABLE_ULPDET_ON_DEVICE_OFF 0x02

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
** Function         phNxpNciHal_isULPDetDeviceOffSupported()
**
** Description      this function is to check ULPDet Device Off supported or not
**
** Returns          true or false
*******************************************************************************/
bool phNxpNciHal_isULPDetDeviceOffSupported();

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

/*******************************************************************************
**
** Function         phNxpNciHal_setULPDetConfig()
**
** Description      this function set's ULPDet config which is required to
**                  enable ULPDet mode
**
** flag             true to update configurations to enable ULPDET, false
**                  to update configurations to disable ULPDET
**
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/
NFCSTATUS phNxpNciHal_setULPDetConfig(bool flag);

/*******************************************************************************
**
** Function         phNxpNciHal_checkAndDisableULPDetConfigs()
**
** Description      this function disables the power mode when the ULPDet
**                  feature is disabled
**
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/
NFCSTATUS phNxpNciHal_checkAndDisableULPDetConfigs();

/*******************************************************************************
**
** Function         phNxpNciHal_getUlpdetGPIOConfig()
**
** Description      this function gets the ULPDET get gpio configuration
**
** Returns          true or false
*******************************************************************************/
uint8_t phNxpNciHal_getUlpdetGPIOConfig();

/*******************************************************************************
**
** Function         phNxpNciHal_enableUlpdetOrAutonomousMode()
**
** Description      this function enables ULPDET mode or Autonomous modes based
**                  on the configurations
**
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/
NFCSTATUS phNxpNciHal_enableUlpdetOrAutonomousMode();