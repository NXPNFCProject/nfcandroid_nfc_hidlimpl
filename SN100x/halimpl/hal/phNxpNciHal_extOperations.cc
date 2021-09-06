/*
 * Copyright 2019-2021 NXP
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
#include <phTmlNfc.h>
#include <phTmlNfc_i2c.h>
#include <phNxpNciHal.h>

#include "phNxpNciHal_extOperations.h"
#include "phNfcCommon.h"
#include "phNxpNciHal_IoctlOperations.h"
#include <phNxpLog.h>
#include <phNxpNciHal_ext.h>

#define NCI_HEADER_SIZE 3
#define NCI_SE_CMD_LEN  4

nxp_nfc_config_ext_t config_ext;

extern phNxpNciHal_Control_t nxpncihal_ctrl;
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
  NFCSTATUS status = NFCSTATUS_FEATURE_NOT_SUPPORTED;

  if (nfcFL.chipType >= sn100u) {
    if (config_ext.autonomous_mode != true)
      config_ext.guard_timer_value = 0x00;

    mEEPROM_info.request_mode = SET_EEPROM_DATA;
    mEEPROM_info.buffer = &config_ext.guard_timer_value;
    mEEPROM_info.bufflen = sizeof(config_ext.guard_timer_value);
    mEEPROM_info.request_type = EEPROM_GUARD_TIMER;

    status = request_EEPROM(&mEEPROM_info);
  }
  return status;
}

/******************************************************************************
 * Function         get_system_property_se_type
 *
 * Description      This will read NFCEE status from system properties
 *                  and returns status.
 *
 * Returns          NFCEE enabled(0x01)/disabled(0x00)
 *
 ******************************************************************************/
static int8_t get_system_property_se_type(uint8_t se_type)
{
  int8_t retVal = -1;
  char valueStr[PROPERTY_VALUE_MAX] = {0};
  if (se_type >= NUM_SE_TYPES)
    return retVal;
  int len = 0;
  switch(se_type) {
    case SE_TYPE_ESE:
      len = property_get("nfc.product.support.ese", valueStr, "");
      break;
    case SE_TYPE_UICC:
      len = property_get("nfc.product.support.uicc", valueStr, "");
      break;
    case SE_TYPE_UICC2:
      len = property_get("nfc.product.support.uicc2", valueStr, "");
      break;
  }
  if(strlen(valueStr) == 0 || len <= 0) {
    return retVal;
  }
  retVal = atoi(valueStr);
  return retVal;
}

/******************************************************************************
 * Function         phNxpNciHal_read_and_update_se_state
 *
 * Description      This will read NFCEE status from system properties
 *                  and update to NFCC to enable/disable.
 *
 * Returns          none
 *
 ******************************************************************************/
void phNxpNciHal_read_and_update_se_state()
{
  NFCSTATUS status = NFCSTATUS_FAILED;
  int16_t i = 0;
  int8_t  val = -1;
  int16_t num_se = 0;
  uint8_t retry_cnt = 0;
  int8_t values[NUM_SE_TYPES];

  for (i = 0; i < NUM_SE_TYPES; i++) {
    val = get_system_property_se_type(i);
    switch(i) {
      case SE_TYPE_ESE:
        NXPLOG_NCIHAL_D("Get property : SUPPORT_ESE %d", val);
        values[SE_TYPE_ESE] = val;
        if(val != -1) {
          num_se++;
        }
        break;
      case SE_TYPE_UICC:
        NXPLOG_NCIHAL_D("Get property : SUPPORT_UICC %d", val);
        values[SE_TYPE_UICC] = val;
        if(val != -1) {
          num_se++;
        }
        break;
      case SE_TYPE_UICC2:
        values[SE_TYPE_UICC2] = val;
        if(val != -1) {
          num_se++;
        }
        NXPLOG_NCIHAL_D("Get property : SUPPORT_UICC2 %d", val);
        break;
    }
  }
  if(num_se < 1) {
    return;
  }
  uint8_t set_cfg_cmd[NCI_HEADER_SIZE + 1 + (num_se * NCI_SE_CMD_LEN)]; // 1 for Number of Argument
  uint8_t *index = &set_cfg_cmd[0];
  *index++ = NCI_MT_CMD;
  *index++ = NXP_CORE_SET_CONFIG_CMD;
  *index++ = (num_se * NCI_SE_CMD_LEN) + 1;
  *index++ = num_se;
  for (i = 0; i < NUM_SE_TYPES; i++) {
    switch(i) {
      case SE_TYPE_ESE:
        if(values[SE_TYPE_ESE] != -1) {
          *index++ = 0xA0;
          *index++ = 0xED;
          *index++ = 0x01;
          *index++ = values[SE_TYPE_ESE];
        }
        break;
      case SE_TYPE_UICC:
        if(values[SE_TYPE_UICC] != -1) {
          *index++ = 0xA0;
          *index++ = 0xEC;
          *index++ = 0x01;
          *index++ = values[SE_TYPE_UICC];
        }
        break;
      case SE_TYPE_UICC2:
        if(values[SE_TYPE_UICC2] != -1) {
          *index++ = 0xA0;
          *index++ = 0xD4;
          *index++ = 0x01;
          *index++ = values[SE_TYPE_UICC2];
        }
        break;
    }
  }

  while(status != NFCSTATUS_SUCCESS && retry_cnt < 3) {
    status = phNxpNciHal_send_ext_cmd(sizeof(set_cfg_cmd), set_cfg_cmd);
    retry_cnt++;
    NXPLOG_NCIHAL_E("Get Cfg Retry cnt=%x", retry_cnt);
  }
}

/******************************************************************************
 * Function         phNxpNciHal_read_fw_dw_status
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
NFCSTATUS phNxpNciHal_read_fw_dw_status(uint8_t &value) {
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  mEEPROM_info.buffer = &value;
  mEEPROM_info.bufflen = sizeof(value);
  mEEPROM_info.request_type = EEPROM_FW_DWNLD;
  mEEPROM_info.request_mode = GET_EEPROM_DATA;
  return request_EEPROM(&mEEPROM_info);
}

/******************************************************************************
 * Function         phNxpNciHal_write_fw_dw_status
 *
 * Description      This will update value of fw download status flag
 *                  to eeprom
 *
 * Parameters       value - this value will be updated to eeprom flag.
 *
 * Returns          status of the write
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_write_fw_dw_status(uint8_t value) {
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  mEEPROM_info.buffer = &value;
  mEEPROM_info.bufflen = sizeof(value);
  mEEPROM_info.request_type = EEPROM_FW_DWNLD;
  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  return request_EEPROM(&mEEPROM_info);
}

/*****************************************************************************
 * Function         phNxpNciHal_send_get_cfg
 *
 * Description      This function is called to get the configurations from
 * EEPROM
 *
 * Params           cmd_get_cfg, Buffer to get the get command
 *                  cmd_len,     Length of the command
 * Returns          SUCCESS/FAILURE
 *
 *
 *****************************************************************************/
NFCSTATUS phNxpNciHal_send_get_cfg(const uint8_t *cmd_get_cfg, long cmd_len) {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  NFCSTATUS status = NFCSTATUS_FAILED;
  uint8_t retry_cnt = 0;

  if (cmd_get_cfg == NULL || cmd_len <= NCI_GET_CONFI_MIN_LEN) {
    NXPLOG_NCIHAL_E("%s invalid command..! returning... ", __func__);
    return status;
  }

  do {
    status = phNxpNciHal_send_ext_cmd(cmd_len, (uint8_t *)cmd_get_cfg);
  } while ((status != NFCSTATUS_SUCCESS) &&
           (retry_cnt++ < NXP_MAX_RETRY_COUNT));

  NXPLOG_NCIHAL_D("%s status : 0x%02X", __func__, status);
  return status;
}

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
NFCSTATUS phNxpNciHal_configure_merge_sak() {
  long retlen = 0;
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  NXPLOG_NCIHAL_D("Performing ISODEP sak merge settings");
  uint8_t val = 0;

  if (!GetNxpNumValue(NAME_NXP_ISO_DEP_MERGE_SAK, (void *)&retlen,
                      sizeof(retlen))) {
    retlen = 0x01;
    NXPLOG_NCIHAL_D(
        "ISO_DEP_MERGE_SAK not found. default shall be enabled : 0x%02lx",
        retlen);
  }
  val = (uint8_t)retlen;
  mEEPROM_info.buffer = &val;
  mEEPROM_info.bufflen = sizeof(val);
  mEEPROM_info.request_type = EEPROM_ISODEP_MERGE_SAK;
  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  return request_EEPROM(&mEEPROM_info);
}
#if(NXP_EXTNS== TRUE && NXP_SRD == TRUE)
/******************************************************************************
 * Function         phNxpNciHal_setSrdtimeout
 *
 * Description      This function can be used to set srd SRD Timeout.
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS or
 *                  NFCSTATUS_FEATURE_NOT_SUPPORTED
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setSrdtimeout() {
  long retlen = 0;
  uint8_t *buffer = nullptr;
  long bufflen = 260;
  const int NXP_SRD_TIMEOUT_BUF_LEN = 2;
  const uint16_t TIMEOUT_MASK = 0xFFFF;
  const uint16_t MAX_TIMEOUT_VALUE = 0xFD70;
  uint16_t isValid_timeout;
  uint8_t timeout_buffer[NXP_SRD_TIMEOUT_BUF_LEN];
  NFCSTATUS status = NFCSTATUS_FEATURE_NOT_SUPPORTED;
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};

  NXPLOG_NCIHAL_D("Performing SRD Timeout settings");

  buffer = (uint8_t *)malloc(bufflen * sizeof(uint8_t));
  if (NULL == buffer) {
    return NFCSTATUS_FAILED;
  }
  memset(buffer, 0x00, bufflen);
  if (GetNxpByteArrayValue(NAME_NXP_SRD_TIMEOUT, (char *)buffer, bufflen,
                           &retlen)) {
    if (retlen == NXP_SRD_TIMEOUT_BUF_LEN) {
      isValid_timeout = ((buffer[1] << 8) & TIMEOUT_MASK);
      isValid_timeout = (isValid_timeout | buffer[0]);
      if (isValid_timeout > MAX_TIMEOUT_VALUE) {
        /*if timeout is setting more than 18hrs
         * than setting to MAX limit 0xFD70*/
        buffer[0] = 0x70;
        buffer[1] = 0xFD;
      }
      memcpy(&timeout_buffer, buffer, NXP_SRD_TIMEOUT_BUF_LEN);
      mEEPROM_info.buffer = timeout_buffer;
      mEEPROM_info.bufflen = sizeof(timeout_buffer);
      mEEPROM_info.request_type = EEPROM_SRD_TIMEOUT;
      mEEPROM_info.request_mode = SET_EEPROM_DATA;
      status = request_EEPROM(&mEEPROM_info);
    }
  }
  if (buffer != NULL) {
    free(buffer);
    buffer = NULL;
  }

  return status;
}

/******************************************************************************
 * Function         phNxpNciHal_getChipInfoInFwDnldMode
 *
 * Description      Helper function to get the chip info in download mode
 *
 * Returns          Status
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_getChipInfoInFwDnldMode(bool bIsVenResetReqd) {
  uint8_t get_chip_info_cmd[] = {0x00, 0x04, 0xF1, 0x00,
                                 0x00, 0x00, 0x6E, 0xEF};
  NFCSTATUS status = NFCSTATUS_FAILED;
  int retry_cnt = 0;
  if (bIsVenResetReqd) {
    nfcFL.nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_VEN_RESET;
    status = phTmlNfc_IoCtl(phTmlNfc_e_EnableDownloadMode);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("Enable Download mode failed");
      return status;
    }
  }
  phTmlNfc_EnableFwDnldMode(true);
  nxpncihal_ctrl.fwdnld_mode_reqd = TRUE;

  do {
    status =
        phNxpNciHal_send_ext_cmd(sizeof(get_chip_info_cmd), get_chip_info_cmd);
    if(status == NFCSTATUS_SUCCESS) {
      /* Check FW getResponse command response status byte */
      if(nxpncihal_ctrl.p_rx_data[0] == 0x00) {
        if (nxpncihal_ctrl.p_rx_data[2] != 0x00) {
          status = NFCSTATUS_FAILED;
          if (retry_cnt < MAX_RETRY_COUNT) {
            retry_cnt++;
          }
        }
      } else {
        status = NFCSTATUS_FAILED;
      }
    }
  } while(status != NFCSTATUS_SUCCESS && retry_cnt < MAX_RETRY_COUNT);

  nxpncihal_ctrl.fwdnld_mode_reqd = FALSE;
  phTmlNfc_EnableFwDnldMode(false);

  if (status == NFCSTATUS_SUCCESS) {
    phNxpNciHal_configFeatureList(nxpncihal_ctrl.p_rx_data,
                                  nxpncihal_ctrl.rx_data_len);
    setNxpFwConfigPath(nfcFL._FW_LIB_PATH.c_str());
  }

  return status;
}
#endif

/******************************************************************************
 * Function         phNxpNciHal_setExtendedFieldMode
 *
 * Description      This function can be used to set nfcc extended field mode
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS or
 *                  NFCSTATUS_FEATURE_NOT_SUPPORTED
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setExtendedFieldMode() {
  const uint8_t enable_val = 0x01;
  const uint8_t disable_val = 0x00;
  uint8_t extended_field_mode = disable_val;
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  NFCSTATUS status = NFCSTATUS_FEATURE_NOT_SUPPORTED;

  if (nfcFL.chipType >= sn100u &&
      GetNxpNumValue(NAME_NXP_EXTENDED_FIELD_DETECT_MODE, &extended_field_mode,
        sizeof(extended_field_mode))) {
    if (extended_field_mode == enable_val ||
        extended_field_mode == disable_val) {
      mEEPROM_info.buffer = &extended_field_mode;
      mEEPROM_info.bufflen = sizeof(extended_field_mode);
      mEEPROM_info.request_type = EEPROM_EXT_FIELD_DETECT_MODE;
      mEEPROM_info.request_mode = SET_EEPROM_DATA;
      status = request_EEPROM(&mEEPROM_info);
    } else {
      NXPLOG_NCIHAL_E("Invalid Extended Field Mode in config");
    }
  }
  return status;
}
