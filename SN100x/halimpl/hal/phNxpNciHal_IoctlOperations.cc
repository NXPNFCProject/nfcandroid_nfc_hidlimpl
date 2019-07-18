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

#include <map>
#include <android-base/file.h>
#include <cutils/properties.h>
#include "phNxpNciHal_ext.h"
#include "phNxpNciHal_utils.h"
#include "phDnldNfc_Internal.h"
#include "phNxpNciHal_Adaptation.h"
#include "phNfcCommon.h"
#include "phNxpNciHal_IoctlOperations.h"
#include "EseAdaptation.h"

using android::base::WriteStringToFile;

#define TERMINAL_LEN 5

/****************************************************************
 * Global Variables Declaration
 ***************************************************************/
/* External global variable to get FW version from NCI response*/
extern uint32_t wFwVerRsp;
/* External global variable to get FW version from FW file*/
extern uint16_t wFwVer;
/* NCI HAL Control structure */
extern phNxpNciHal_Control_t nxpncihal_ctrl;
extern phNxpNci_getCfg_info_t *mGetCfg_info;
extern EseAdaptation *gpEseAdapt;
extern nfc_stack_callback_t *p_nfc_stack_cback_backup;
#ifndef FW_DWNLD_FLAG
extern uint8_t fw_dwnld_flag;
#endif

extern size_t readConfigFile(const char* fileName, uint8_t** p_data);

typedef std::map<std::string, std::string> systemProperty;
systemProperty gsystemProperty = {
        {"nfc.fw.rfreg_ver",          "0"},
        {"nfc.fw.rfreg_display_ver",  "0"},
        {"nfc.fw.dfl_areacode",       "0"},
        {"nfc.fw.downloadmode_force", "0"},
        {"nfc.nxp.fwdnldstatus",      "0"},
};


/****************************************************************
 * Local Functions
 ***************************************************************/

/******************************************************************************
 ** Function         phNxpNciHal_nfcStackCb
 **
 ** Description      This function shall be used to post events to the nfc
 *stack.
 **
 ** Parameters       pCb: Callback handle for NFC stack callback
 **                  evt: event to be posted to the given callback.
 **
 ** Returns          Always Zero.
 **
 *******************************************************************************/
static int phNxpNciHal_nfcStackCb(nfc_stack_callback_t *pCb, int evt);

/*******************************************************************************
 **
 ** Function         phNxpNciHal_getChipType
 **
 ** Description      Gets the chipType which is configured during bootup
 **
 ** Parameters       none
 **
 ** Returns          chipType
 *******************************************************************************/
static tNFC_chipType phNxpNciHal_getChipType();

/*******************************************************************************
**
** Function         phNxpNciHal_NciTransceive
**
** Description      This method shall be called to send NCI command and receive
*RSP
**
** Parameters       Pointer to the IOCTL data
**
** Returns          status of the Transceive
**                  return 0 on success and -1 on fail
*******************************************************************************/
static int phNxpNciHal_NciTransceive(nfc_nci_IoctlInOutData_t *pInpOutData);

/*******************************************************************************
**
** Function         phNxpNciHal_CheckFlashReq
**
** Description      Updates FW and Reg configurations if required
**
** Parameters       Pointer to the IOCTL data
**
** Returns          return 0 on success and -1 on fail
*******************************************************************************/
static int phNxpNciHal_CheckFlashReq(nfc_nci_IoctlInOutData_t *pInpOutData);

/*******************************************************************************
**
** Function         phNxpNciHal_GetFeatureList
**
** Description      Gets the chipType which is configured during bootup
**
** Parameters       Pointer to the IOCTL data
**
** Returns          return 0 on success and -1 on fail
*******************************************************************************/
static int phNxpNciHal_GetFeatureList(nfc_nci_IoctlInOutData_t *pInpOutData);

/*******************************************************************************/

/******************************************************************************
 ** Function         phNxpNciHal_ioctlIf
 **
 ** Description      This function shall be called from HAL when libnfc-nci
 **                  calls phNxpNciHal_ioctl() to perform any IOCTL operation
 **
 ** Returns          return 0 on success and -1 on fail,
 ******************************************************************************/
int phNxpNciHal_ioctlIf(long arg, void *p_data) {
  NXPLOG_NCIHAL_D("%s : enter - arg = %ld", __func__, arg);
  nfc_nci_IoctlInOutData_t *pInpOutData = (nfc_nci_IoctlInOutData_t *)p_data;
  int ret = -1;

  switch (arg) {
  case HAL_NFC_IOCTL_CHECK_FLASH_REQ:
    ret = phNxpNciHal_CheckFlashReq(pInpOutData);
    break;
  case HAL_NFC_IOCTL_NCI_TRANSCEIVE:
    ret = phNxpNciHal_NciTransceive(pInpOutData);
    break;
  case HAL_NFC_IOCTL_GET_FEATURE_LIST:
    ret = phNxpNciHal_GetFeatureList(pInpOutData);
    break;
  case HAL_ESE_IOCTL_NFC_JCOP_DWNLD:
    if (pInpOutData == NULL) {
      NXPLOG_NCIHAL_E("%s : received invalid param", __func__);
    } else{
      NXPLOG_NCIHAL_D("HAL_ESE_IOCTL_NFC_JCOP_DWNLD Enter value is %d: \n",
              pInpOutData->inp.data.nciCmd.p_cmd[0]);
    }
    if (gpEseAdapt == NULL) {
      gpEseAdapt = &EseAdaptation::GetInstance();
      gpEseAdapt->Initialize();
    }
    if (gpEseAdapt != NULL)
      ret = gpEseAdapt->HalIoctl(HAL_ESE_IOCTL_NFC_JCOP_DWNLD, pInpOutData);
    [[fallthrough]];
  case HAL_NFC_IOCTL_ESE_JCOP_DWNLD:
    if (pInpOutData == NULL) {
      NXPLOG_NCIHAL_E("%s : received invalid param", __func__);
    } else{
      NXPLOG_NCIHAL_D("HAL_NFC_IOCTL_ESE_JCOP_DWNLD Enter value is %d: \n",
              pInpOutData->inp.data.nciCmd.p_cmd[0]);
    }
    phNxpNciHal_nfcStackCb(p_nfc_stack_cback_backup, HAL_NFC_HCI_NV_RESET);
    ret = 0;
    break;
  case HAL_NFC_IOCTL_GET_ESE_UPDATE_STATE:
    ret = 0;
    break;
  case HAL_NFC_IOCTL_ESE_UPDATE_COMPLETE:
    ese_update = ESE_UPDATE_COMPLETED;
    NXPLOG_NCIHAL_D("HAL_NFC_IOCTL_ESE_UPDATE_COMPLETE \n");
    phNxpNciHal_nfcStackCb(p_nfc_stack_cback_backup, HAL_NFC_STATUS_RESTART);
    ret = 0;
    break;
  case HAL_NFC_IOCTL_ESE_HARD_RESET:
    [[fallthrough]];
  case HAL_NFC_SET_SPM_PWR: {
    long level = pInpOutData->inp.level;
    if (NCI_ESE_HARD_RESET_IOCTL == level) {
      ret = phNxpNciHal_resetEse();
    }
    break;
  }
  case HAL_NFC_IOCTL_SET_TRANSIT_CONFIG:
    if (pInpOutData != NULL) {
      phNxpNciHal_setNxpTransitConfig(pInpOutData->inp.data.transitConfig.val);
      ret = 0;
    } else {
      NXPLOG_NCIHAL_E("%s : received invalid param", __func__);
    }
    break;
  default:
    NXPLOG_NCIHAL_E("%s : Wrong arg = %ld", __func__, arg);
    break;
  }
  NXPLOG_NCIHAL_D("%s : exit - ret = %d", __func__, ret);
  return ret;
}


/*******************************************************************************
 **
 ** Function         phNxpNciHal_savePersistLog
 **
 ** Description      Save persist log with “reason” at available index.
 **
 ** Parameters       uint8_t reason
 **
 ** Returns          returns the  index of saved reason/Log.
 *******************************************************************************/
uint8_t phNxpNciHal_savePersistLog(uint8_t reason) {
  /* This is dummy API */
  (void) reason;
  uint8_t index = 1;
  NXPLOG_NCIHAL_D(" %s returning index %d", __func__, index);
  return index;

}

/*******************************************************************************
 **
 ** Function         phNxpNciHal_loadPersistLog
 **
 ** Description      If given index is valid, return a log at the given index.
 **
 ** Parameters       uint8_t index
 **
 ** Returns          If index found, return a log as string else
 **                  return a "" string
 *******************************************************************************/
string phNxpNciHal_loadPersistLog(uint8_t index)
{
  /* This is dummy API */
  string reason;
  switch(index)
  {
  case 1:
    NXPLOG_NCIHAL_D("index found");
    reason = "Reason";
    break;
  default:
    NXPLOG_NCIHAL_E("index not found");
  }
  return reason;
}
/*******************************************************************************
 **
 ** Function         phNxpNciHal_getSystemProperty
 **
 ** Description      It shall be used to get property value of the given Key
 **
 ** Parameters       string key
 **
 ** Returns          If Key is found, returns the respective property values
 **                  else returns the null/empty string
 *******************************************************************************/
string phNxpNciHal_getSystemProperty(string key)
{
  string propValue;
  std::map<std::string, std::string>::iterator prop;

  prop = gsystemProperty.find(key);
  if(prop != gsystemProperty.end()){
    propValue = prop->second;
  }else{
    if(key == "libnfc-nxp.conf")
      return phNxpNciHal_getNxpConfigIf();
   /* else Pass a null string */
  }

  return propValue;
}
/*******************************************************************************
 **
 ** Function         phNxpNciHal_setSystemProperty
 **
 ** Description      It shall be used to save/change value to system property
 **                  based on provided key.
 **
 ** Parameters       string key, string value
 **
 ** Returns          true if success, false if fail
 *******************************************************************************/
bool phNxpNciHal_setSystemProperty(string key, string value) {
  NXPLOG_NCIHAL_D("%s : Enter Key = %s, value = %s", __func__,
          key.c_str(), value.c_str());

  gsystemProperty[key] = value;
  return true;
}

/*******************************************************************************
**
** Function         phNxpNciHal_getNxpConfig
**
** Description      It shall be used to read config values from the
*libnfc-nxp.conf
**
** Parameters       nxpConfigs config
**
** Returns          void
*******************************************************************************/
string phNxpNciHal_getNxpConfigIf() {
  std::string config;
  uint8_t* p_config = nullptr;
  NXPLOG_NCIHAL_D("phNxpNciHal_getNxpConfigIf: Enter");

  size_t config_size = readConfigFile("/vendor/etc/libnfc-nxp.conf", &p_config);
  if (config_size) {
    config.assign((char *)p_config, config_size);
    free(p_config);
  }
  NXPLOG_NCIHAL_D("phNxpNciHal_getNxpConfigIf: Exit");
  return config;
}

/*******************************************************************************
**
** Function         phNxpNciHal_resetEse
**
** Description      It shall be used to reset eSE by proprietary command.
**
** Parameters
**
** Returns          status of eSE reset response
*******************************************************************************/
NFCSTATUS phNxpNciHal_resetEse() {
  NFCSTATUS status = NFCSTATUS_FAILED;
  uint8_t cmd_ese_pwrcycle[] = {0x2F, 0x1E, 0x00};

  if (nxpncihal_ctrl.halStatus == HAL_STATUS_CLOSE) {
    if (NFCSTATUS_SUCCESS != phNxpNciHal_MinOpen()) {
      return NFCSTATUS_FAILED;
    }
  }

  CONCURRENCY_LOCK();
  status = phNxpNciHal_send_ext_cmd(
      sizeof(cmd_ese_pwrcycle) / sizeof(cmd_ese_pwrcycle[0]), cmd_ese_pwrcycle);
  CONCURRENCY_UNLOCK();
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("EsePowerCycle failed");
  }

  if (nxpncihal_ctrl.halStatus == HAL_STATUS_MIN_OPEN) {
    phNxpNciHal_close(false);
  }

  return status;
}

/******************************************************************************
 * Function         phNxpNciHal_getNxpTransitConfig
 *
 * Description      This function overwrite libnfc-nxpTransit.conf file
 *                  with transitConfValue.
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_setNxpTransitConfig(char *transitConfValue) {
  NXPLOG_NCIHAL_D("%s : Enter", __func__);
  std::string transitConfFileName = "/data/vendor/nfc/libnfc-nxpTransit.conf";

  if (transitConfValue != NULL) {
    if (!WriteStringToFile(transitConfValue, transitConfFileName)) {
      NXPLOG_NCIHAL_E("WriteStringToFile: Failed");
    }
  } else {
    if (!WriteStringToFile("", transitConfFileName)) {
      NXPLOG_NCIHAL_E("WriteStringToFile: Failed");
    }
    if (!remove(transitConfFileName.c_str())) {
      NXPLOG_NCIHAL_E("Unable to remove file");
    }
  }
  NXPLOG_NCIHAL_D("%s : Exit", __func__);
}

/******************************************************************************
** Function         phNxpNciHal_nfcStackCb
**
** Description      This function shall be used to post events to the nfc stack.
**
** Parameters       pCb: Callback handle for NFC stack callback
**                  evt: event to be posted to the given callback.
**
** Returns          void.
**
*******************************************************************************/
static int phNxpNciHal_nfcStackCb(nfc_stack_callback_t *pCb, int evt) {
  int ret = 0;
  if (pCb != NULL) {
    (*pCb)(HAL_NFC_OPEN_CPLT_EVT, evt);
  } else {
    NXPLOG_NCIHAL_D("p_nfc_stack_cback_backup cback NULL \n");
    ret = -1;
  }
  return ret;
}

/*******************************************************************************
 **
 ** Function:        phNxpNciHal_CheckFwRegFlashRequired()
 **
 ** Description:     Updates FW and Reg configurations if required
 **
 ** Returns:         status
 **
 ********************************************************************************/
int phNxpNciHal_CheckFwRegFlashRequired(uint8_t *fw_update_req,
                                        uint8_t *rf_update_req,
                                        uint8_t skipEEPROMRead) {
  NXPLOG_NCIHAL_D("phNxpNciHal_CheckFwRegFlashRequired() : enter");
  int status = NFCSTATUS_OK;
  long option;
  if (fpRegRfFwDndl != NULL) {
    status = fpRegRfFwDndl(fw_update_req, rf_update_req, skipEEPROMRead);
  } else {
    status = phDnldNfc_InitImgInfo();
    NXPLOG_NCIHAL_D("FW version of the libsn100u.so binary = 0x%x", wFwVer);
    NXPLOG_NCIHAL_D("FW version found on the device = 0x%x", wFwVerRsp);

    if (!GetNxpNumValue(NAME_NXP_FLASH_CONFIG, &option,
                        sizeof(unsigned long))) {
      NXPLOG_NCIHAL_D("Flash option not found; giving default value");
      option = 1;
    }
    switch (option) {
    case FLASH_UPPER_VERSION:
      wFwUpdateReq = (utf8_t)wFwVer > (utf8_t)wFwVerRsp ? true : false;
      break;
    case FLASH_DIFFERENT_VERSION:
      wFwUpdateReq = ((wFwVerRsp & 0x0000FFFF) != wFwVer) ? true : false;
      break;
    case FLASH_ALWAYS:
      wFwUpdateReq = true;
      break;
    default:
      NXPLOG_NCIHAL_D("Invalid flash option selected");
      status = NFCSTATUS_INVALID_PARAMETER;
      break;
    }
  }
  *fw_update_req = wFwUpdateReq;

  if (false == wFwUpdateReq) {
    NXPLOG_NCIHAL_D("FW update not required");
    phDnldNfc_ReSetHwDevHandle();
  } else {
    property_set("nfc.fw.downloadmode_force", "1");
  }

  NXPLOG_NCIHAL_D("phNxpNciHal_CheckFwRegFlashRequired() : exit - status = %x "
                  "wFwUpdateReq=%u, wRfUpdateReq=%u",
                  status, *fw_update_req, *rf_update_req);
  return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_getChipType
**
** Description      Gets the chipType which is configured during bootup
**
** Parameters       none
**
** Returns          chipType
*******************************************************************************/
static tNFC_chipType phNxpNciHal_getChipType() {
  return nxpncihal_ctrl.chipType;
}

/*******************************************************************************
**
** Function         phNxpNciHal_NciTransceive
**
** Description      This API shall be called to to send NCI command and receive
*RSP
**
** Parameters       IOCTL data pointer
**
** Returns          status of the Transceive
**                  return 0 on success and -1 on fail
*******************************************************************************/
static int phNxpNciHal_NciTransceive(nfc_nci_IoctlInOutData_t *pInpOutData) {
  int ret = -1;

  if (pInpOutData == NULL && pInpOutData->inp.data.nciCmd.cmd_len == 0) {
    NXPLOG_NCIHAL_E("%s : received invalid arguments", __func__);
    return ret;
  }

  if (nxpncihal_ctrl.halStatus == HAL_STATUS_CLOSE) {
    if (NFCSTATUS_SUCCESS != phNxpNciHal_MinOpen()) {
      pInpOutData->out.data.nciRsp.p_rsp[3] = 1;
      return ret;
    }
  }

  ret = phNxpNciHal_send_ext_cmd(pInpOutData->inp.data.nciCmd.cmd_len,
                                 pInpOutData->inp.data.nciCmd.p_cmd);

  pInpOutData->out.data.nciRsp.rsp_len = nxpncihal_ctrl.rx_data_len;

  if ((nxpncihal_ctrl.rx_data_len > 0) &&
      (nxpncihal_ctrl.rx_data_len <= MAX_IOCTL_TRANSCEIVE_RESP_LEN) &&
      (nxpncihal_ctrl.p_rx_data != NULL)) {
    memcpy(pInpOutData->out.data.nciRsp.p_rsp, nxpncihal_ctrl.p_rx_data,
           nxpncihal_ctrl.rx_data_len);
  }

  if (nxpncihal_ctrl.halStatus == HAL_STATUS_MIN_OPEN) {
    phNxpNciHal_close(false);
  }
  return ret;
}

/*******************************************************************************
**
** Function         phNxpNciHal_CheckFlashReq
**
** Description      Updates FW and Reg configurations if required
**
** Parameters       IOCTL data pointer
**
** Returns          return 0 on success and -1 on fail
*******************************************************************************/
static int phNxpNciHal_CheckFlashReq(nfc_nci_IoctlInOutData_t *pInpOutData) {
  int ret = -1;
  phNxpNciHal_FwRfupdateInfo_t *FwRfInfo;

  if (pInpOutData == NULL) {
    NXPLOG_NCIHAL_E("%s : received invalid param", __func__);
    return ret;
  }
  if (nxpncihal_ctrl.halStatus == HAL_STATUS_CLOSE) {
    if (NFCSTATUS_SUCCESS != phNxpNciHal_MinOpen()) {
      return ret;
    }
  }

  FwRfInfo = (phNxpNciHal_FwRfupdateInfo_t *)&pInpOutData->out.data.fwUpdateInf;

  if (NFCSTATUS_SUCCESS ==
      phNxpNciHal_CheckFwRegFlashRequired(&FwRfInfo->fw_update_reqd,
                                          &FwRfInfo->rf_update_reqd, false)) {
#ifndef FW_DWNLD_FLAG
    fw_dwnld_flag = FwRfInfo->fw_update_reqd;
#endif
    ret = 0;
  }

  if (nxpncihal_ctrl.halStatus == HAL_STATUS_MIN_OPEN) {
    phNxpNciHal_close(false);
  }

  return ret;
}

/*******************************************************************************
**
** Function         phNxpNciHal_GetFeatureList
**
** Description      Gets the chipType which is configured during bootup
**
** Parameters       IOCTL data pointer
**
** Returns          return 0 on success and -1 on fail
*******************************************************************************/
static int phNxpNciHal_GetFeatureList(nfc_nci_IoctlInOutData_t *pInpOutData) {
  int ret = -1;
  if (pInpOutData == NULL) {
    NXPLOG_NCIHAL_E("%s : received invalid param", __func__);
    return ret;
  }
  if (nxpncihal_ctrl.halStatus == HAL_STATUS_CLOSE) {
    if (NFCSTATUS_SUCCESS != phNxpNciHal_MinOpen()) {
      return ret;
    }
  }

  pInpOutData->out.data.chipType = (uint8_t)phNxpNciHal_getChipType();
  ret = 0;
  if (nxpncihal_ctrl.halStatus == HAL_STATUS_MIN_OPEN) {
    phNxpNciHal_close(false);
  }
  return ret;
}
