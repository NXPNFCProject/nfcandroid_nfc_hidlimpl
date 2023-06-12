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

#include "phNxpNciHal_ULPDet.h"
#include <phNxpLog.h>
#include <phTmlNfc.h>
#include <vector>
#include "phNfcCommon.h"
#include "phNxpNciHal_IoctlOperations.h"
#include "phNxpNciHal_PowerTrackerIface.h"
#include "phNxpNciHal_extOperations.h"

extern phNxpNciHal_Control_t nxpncihal_ctrl;
extern NFCSTATUS phNxpNciHal_ext_send_sram_config_to_flash();
extern PowerTrackerHandle gPowerTrackerHandle;

/*******************************************************************************
**
** Function         phNxpNciHal_isULPDetSupported()
**
** Description      this function is to check ULPDet feature is supported or not
**
** Returns          true or false
*******************************************************************************/
bool phNxpNciHal_isULPDetSupported() {
  unsigned long num = 0;
  if ((GetNxpNumValue(NAME_NXP_DEFAULT_ULPDET_MODE, &num, sizeof(num)))) {
    if ((uint8_t)num == ENABLE_ULPDET_USING_API) {
      NXPLOG_NCIHAL_E("%s: NxpNci isULPDetSupported true", __func__);
      return true;
    }
  }
  NXPLOG_NCIHAL_E("%s: NxpNci isULPDetSupported false", __func__);
  return false;
}

/*******************************************************************************
**
** Function         phNxpNciHal_isULPDetDeviceOffSupported()
**
** Description      this function is to check ULPDet Device Off supported or not
**
** Returns          true or false
*******************************************************************************/
bool phNxpNciHal_isULPDetDeviceOffSupported() {
  unsigned long num = 0;
  if ((GetNxpNumValue(NAME_NXP_DEFAULT_ULPDET_MODE, &num, sizeof(num)))) {
    if ((uint8_t)num == ENABLE_ULPDET_ON_DEVICE_OFF) {
      NXPLOG_NCIHAL_E("%s: NxpNci ULPDET phone off enabled", __func__);
      return true;
    }
  }
  NXPLOG_NCIHAL_E("%s: NxpNci ULPDET phone off not supported", __func__);
  return false;
}

/*******************************************************************************
**
** Function         phNxpNciHal_getUlpdetGPIOConfig()
**
** Description      this function gets the ULPDET get gpio configuration
**
** Returns          true or false
*******************************************************************************/
uint8_t phNxpNciHal_getUlpdetGPIOConfig() {
  unsigned long num = 0;
  bool isFound = GetNxpNumValue(NAME_NXP_ULPDET_GPIO_CFG, &num, sizeof(num));
  if (!isFound) num = 0;
  return num;
}

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
void phNxpNciHal_setULPDetFlag(bool flag) {
  nxpncihal_ctrl.isUlpdetModeEnabled = flag;
  if (gPowerTrackerHandle.stateChange != NULL) {
    RefreshNfccPowerState state = (flag) ? ULPDET_ON : ULPDET_OFF;
    gPowerTrackerHandle.stateChange(state);
  }
}

/*******************************************************************************
**
** Function         phNxpNciHal_getULPDetFlag()
**
** Description      this function get the ULPDet state, true if it is enabled
**                  false if it is disabled
**
** Returns          true or false
*******************************************************************************/
bool phNxpNciHal_getULPDetFlag() { return nxpncihal_ctrl.isUlpdetModeEnabled; }

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
NFCSTATUS phNxpNciHal_propConfULPDetMode(bool bEnable) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  NXPLOG_NCIHAL_E("%s flag %d", __func__, bEnable);
  if (!(phNxpNciHal_isULPDetSupported() ||
        phNxpNciHal_isULPDetDeviceOffSupported()))
    return false;

  vector<uint8_t> cmd_coreULPDET = {0x2F, 0x00, 0x01, 0x03};
  vector<uint8_t> getConfig_A015 = {0x20, 0x03, 0x03, 0x01, 0xA0, 0x15};
  vector<uint8_t> setConfig_A015 = {0x20, 0x02, 0x05, 0x01,
                                    0xA0, 0x15, 0x01, 0x01};
  vector<uint8_t> getConfig_A10F = {0x20, 0x03, 0x03, 0x01, 0xA1, 0x0F};
  vector<uint8_t> setConfig_A10F = {0x20, 0x02, 0x06, 0x01, 0xA1,
                                    0x0F, 0x02, 0x00, 0x00};

  do {
    if (bEnable) {
      status = phNxpNciHal_setULPDetConfig(true);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("ulpdet configs set: Failed");
        break;
      }

      status =
          phNxpNciHal_send_ext_cmd(cmd_coreULPDET.size(), &cmd_coreULPDET[0]);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("coreULPDET: Failed");
        break;
      }

      nxpncihal_ctrl.halStatus = HAL_STATUS_CLOSE;
      status = phNxpNciHal_ext_send_sram_config_to_flash();
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Updation of the SRAM contents failed");
        break;
      }
      (void)phTmlNfc_IoCtl(phTmlNfc_e_PullVenLow);
    } else {
      status = phNxpNciHal_setULPDetConfig(false);
      /* reset the flag upon exit ulpdet mode */
      phNxpNciHal_setULPDetFlag(false);
    }
  } while (false);
  NXPLOG_NCIHAL_E("%s: exit. status = %d", __func__, status);

  return status;
}

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
NFCSTATUS phNxpNciHal_setULPDetConfig(bool flag) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  NXPLOG_NCIHAL_E("%s flag", __func__);

  vector<uint8_t> getConfig_A015 = {0x20, 0x03, 0x03, 0x01, 0xA0, 0x15};
  vector<uint8_t> setConfig_A015 = {0x20, 0x02, 0x05, 0x01,
                                    0xA0, 0x15, 0x01, 0x03};
  vector<uint8_t> getConfig_A10F = {0x20, 0x03, 0x03, 0x01, 0xA1, 0x0F};
  vector<uint8_t> setConfig_A10F = {0x20, 0x02, 0x06, 0x01, 0xA1,
                                    0x0F, 0x02, 0x01, 0x01};
  uint8_t setVal_A015 = STANDBY_STATE;
  if (flag) {
    if (phNxpNciHal_isULPDetDeviceOffSupported()) {
      setVal_A015 = AUTONOMOUS_ULPDET_MODE;
    } else if (phNxpNciHal_isULPDetSupported()) {
      setVal_A015 = AUTONOMOUS_MODE;
    }
  }
  uint8_t setVal_A10F_firstByte = flag ? 0x01 : 0x00;

  do {
    status =
        phNxpNciHal_send_ext_cmd(getConfig_A015.size(), &getConfig_A015[0]);
    if ((status == NFCSTATUS_SUCCESS) &&
        (nxpncihal_ctrl.p_rx_data[8] != setVal_A015)) {
      setConfig_A015[7] = setVal_A015;
      status =
          phNxpNciHal_send_ext_cmd(setConfig_A015.size(), &setConfig_A015[0]);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Set Config : Failed");
      }
    }

    status =
        phNxpNciHal_send_ext_cmd(getConfig_A10F.size(), &getConfig_A10F[0]);
    if ((status == NFCSTATUS_SUCCESS) &&
        (nxpncihal_ctrl.p_rx_data[8] != 0x01)) {
      setConfig_A10F[7] = setVal_A10F_firstByte;
      setConfig_A10F[8] = phNxpNciHal_getUlpdetGPIOConfig();
      status =
          phNxpNciHal_send_ext_cmd(setConfig_A10F.size(), &setConfig_A10F[0]);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Set Config: Failed");
      }
    }
  } while (false);
  NXPLOG_NCIHAL_E("%s: exit. status = %d", __func__, status);

  return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_checkAndDisableULPDetConfigs()
**
** Description      this function disables the power mode when the ULPDet
*feature
**                  is disabled
**
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/
NFCSTATUS phNxpNciHal_checkAndDisableULPDetConfigs() {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  vector<uint8_t> getConfig_A10F = {0x20, 0x03, 0x03, 0x01, 0xA1, 0x0F};
  vector<uint8_t> setConfig_A10F = {0x20, 0x02, 0x06, 0x01, 0xA1,
                                    0x0F, 0x02, 0x00, 0x00};
  status = phNxpNciHal_send_ext_cmd(getConfig_A10F.size(), &getConfig_A10F[0]);
  if ((status == NFCSTATUS_SUCCESS) && (nxpncihal_ctrl.p_rx_data[8] != 0x00)) {
    status =
        phNxpNciHal_send_ext_cmd(setConfig_A10F.size(), &setConfig_A10F[0]);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("Set Config: Failed");
    }
  }
  return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_enableUlpdetOrAutonomousMode()
**
** Description      this function enables ULPDET mode or Autonomous modes based
**                  on the configurations
**
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/
NFCSTATUS phNxpNciHal_enableUlpdetOrAutonomousMode() {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  bool isULPDetRequiredForShutdown =
      IS_CHIP_TYPE_GE(sn220u) && phNxpNciHal_isULPDetDeviceOffSupported();
  bool isULPDetEnabledForApis =
      IS_CHIP_TYPE_GE(sn220u) && phNxpNciHal_isULPDetSupported();

  if (isULPDetRequiredForShutdown) {
    status = phNxpNciHal_setULPDetConfig(true);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("NxpNci ULPdet configs set: Failed");
    }
  } else if (isULPDetEnabledForApis) {
    status = phNxpNciHal_propConfULPDetMode(false);
  } else {
    status = phNxpNciHal_setAutonomousMode();
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("Set Autonomous enable: Failed");
    }
    if (IS_CHIP_TYPE_GE(sn220u)) phNxpNciHal_checkAndDisableULPDetConfigs();
  }

  return status;
}