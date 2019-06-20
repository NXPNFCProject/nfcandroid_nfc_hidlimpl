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
    if (pInpOutData == NULL) {
      NXPLOG_NCIHAL_E("%s : received invalid param", __func__);
    } else {
      phNxpNciHal_setNxpTransitConfig(pInpOutData->inp.data.transitConfig.val);
      ret = 0;
    }
    break;
  case HAL_NFC_IOCTL_GET_NXP_CONFIG:
    phNxpNciHal_getNxpConfig(pInpOutData);
    ret = 0;
    break;
  default:
    NXPLOG_NCIHAL_E("%s : Wrong arg = %ld", __func__, arg);
    break;
  }
  NXPLOG_NCIHAL_D("%s : exit - ret = %d", __func__, ret);
  return ret;
}

void phNxpNciHal_loadPersistLog(uint8_t index) { (void)index; }

void phNxpNciHal_savePersistLog(uint8_t index) { (void)index; }

void phNxpNciHal_getSystemProperty(string key) { (void)key; }

void phNxpNciHal_setSystemProperty(string key, string value) {
  (void)key;
  (void)value;
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
void phNxpNciHal_getNxpConfigIf(nxp_nfc_config_t *configs) {
  unsigned long num = 0;
  char val[TERMINAL_LEN] = {0};
  uint8_t *buffer = NULL;
  long bufflen = 260;
  long retlen = 0;

  buffer = (uint8_t *)malloc(bufflen * sizeof(uint8_t));

  NXPLOG_NCIHAL_D("phNxpNciHal_getNxpConfig: Enter");
  if (GetNxpNumValue(NAME_NXP_SE_COLD_TEMP_ERROR_DELAY, &num, sizeof(num))) {
    configs->eSeLowTempErrorDelay = num;
  }
  if (GetNxpNumValue(NAME_NXP_SWP_RD_TAG_OP_TIMEOUT, &num, sizeof(num))) {
    configs->tagOpTimeout = num;
  }
  if (GetNxpNumValue(NAME_NXP_DUAL_UICC_ENABLE, &num, sizeof(num))) {
    configs->dualUiccEnable = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_AID_ROUTE, &num, sizeof(num))) {
    configs->defaultAidRoute = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_MIFARE_CLT_ROUTE, &num, sizeof(num))) {
    configs->defaultMifareCltRoute = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_FELICA_CLT_ROUTE, &num, sizeof(num))) {
    configs->defautlFelicaCltRoute = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_AID_PWR_STATE, &num, sizeof(num))) {
    configs->defaultAidPwrState = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_DESFIRE_PWR_STATE, &num, sizeof(num))) {
    configs->defaultDesfirePwrState = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_MIFARE_CLT_PWR_STATE, &num, sizeof(num))) {
    configs->defaultMifareCltPwrState = num;
  }
  if (GetNxpNumValue(NAME_HOST_LISTEN_TECH_MASK, &num, sizeof(num))) {
    configs->hostListenTechMask = num;
  }
  if (GetNxpNumValue(NAME_FORWARD_FUNCTIONALITY_ENABLE, &num, sizeof(num))) {
    configs->fwdFunctionalityEnable = num;
  }
  if (GetNxpNumValue(NAME_DEFUALT_GSMA_PWR_STATE, &num, sizeof(num))) {
    configs->gsmaPwrState = num;
  }
  if (GetNxpNumValue(NAME_NXP_DEFAULT_UICC2_SELECT, &num, sizeof(num))) {
    configs->defaultUicc2Select = num;
  }
  if (GetNxpNumValue(NAME_NXP_SMB_TRANSCEIVE_TIMEOUT, &num, sizeof(num))) {
    configs->smbTransceiveTimeout = num;
  }
  if (GetNxpNumValue(NAME_NXP_SMB_ERROR_RETRY, &num, sizeof(num))) {
    configs->smbErrorRetry = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_FELICA_CLT_PWR_STATE, &num, sizeof(num))) {
    configs->felicaCltPowerState = num;
  }
  if (GetNxpNumValue(NAME_CHECK_DEFAULT_PROTO_SE_ID, &num, sizeof(num))) {
    configs->checkDefaultProtoSeId = num;
  }
  if (GetNxpNumValue(NAME_NXPLOG_NCIHAL_LOGLEVEL, &num, sizeof(num))) {
    configs->nxpLogHalLoglevel = num;
  }
  if (GetNxpNumValue(NAME_NXPLOG_EXTNS_LOGLEVEL, &num, sizeof(num))) {
    configs->nxpLogExtnsLogLevel = num;
  }
  if (GetNxpNumValue(NAME_NXPLOG_TML_LOGLEVEL, &num, sizeof(num))) {
    configs->nxpLogTmlLogLevel = num;
  }
  if (GetNxpNumValue(NAME_NXPLOG_FWDNLD_LOGLEVEL, &num, sizeof(num))) {
    configs->nxpLogFwDnldLogLevel = num;
  }
  if (GetNxpNumValue(NAME_NXPLOG_NCIX_LOGLEVEL, &num, sizeof(num))) {
    configs->nxpLogNcixLogLevel = num;
  }
  if (GetNxpNumValue(NAME_NXPLOG_NCIR_LOGLEVEL, &num, sizeof(num))) {
    configs->nxpLogNcirLogLevel = num;
  }
  if (GetNxpStrValue(NAME_NXP_NFC_SE_TERMINAL_NUM, val, TERMINAL_LEN)) {
    NXPLOG_NCIHAL_D("NfcSeTerminalId found val = %s ", val);
    configs->seApduGateEnabled = 1;
  } else {
    configs->seApduGateEnabled = 0;
  }
  if (GetNxpNumValue(NAME_NXP_POLL_FOR_EFD_TIMEDELAY, &num, sizeof(num))) {
    configs->pollEfdDelay = num;
  }
  if (GetNxpNumValue(NAME_NXP_NFCC_MERGE_SAK_ENABLE, &num, sizeof(num))) {
    configs->mergeSakEnable = num;
  }
  if (GetNxpNumValue(NAME_NXP_STAG_TIMEOUT_CFG, &num, sizeof(num))) {
    configs->stagTimeoutCfg = num;
  }
  if (buffer) {
    if (GetNxpStrValue(NAME_RF_STORAGE, (char *)buffer, bufflen)) {
      retlen = strlen((char *)buffer) + 1;
      memcpy(configs->rfStorage.path, (char *)buffer, retlen);
      configs->rfStorage.len = retlen;
    }
    if (GetNxpStrValue(NAME_FW_STORAGE, (char *)buffer, bufflen)) {
      retlen = strlen((char *)buffer) + 1;
      memcpy(configs->fwStorage.path, (char *)buffer, retlen);
      configs->fwStorage.len = retlen;
    }
    if (GetNxpByteArrayValue(NAME_NXP_CORE_CONF, (char *)buffer, bufflen,
                             &retlen)) {
      memcpy(configs->coreConf.cmd, (char *)buffer, retlen);
      configs->coreConf.len = retlen;
    }
    if (GetNxpByteArrayValue(NAME_NXP_RF_FILE_VERSION_INFO, (char *)buffer,
                             bufflen, &retlen)) {
      memcpy(configs->rfFileVersInfo.ver, (char *)buffer, retlen);
      configs->rfFileVersInfo.len = retlen;
    }
    free(buffer);
    buffer = NULL;
  }
  NXPLOG_NCIHAL_D("phNxpNciHal_getNxpConfig: Exit");
  return;
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
