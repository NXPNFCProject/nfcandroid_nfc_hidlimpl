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
#include <set>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <android-base/parseint.h>
#include <cutils/properties.h>
#include "phNxpNciHal_ext.h"
#include "phNxpNciHal_utils.h"
#include "phDnldNfc_Internal.h"
#include "phNxpNciHal_Adaptation.h"
#include "phNfcCommon.h"
#include "phNxpNciHal_IoctlOperations.h"
#include "EseAdaptation.h"
#include "phNfcCommon.h"
#include "phNxpNciHal_Adaptation.h"
#include "phNxpNciHal_extOperations.h"

using android::base::WriteStringToFile;
using namespace ::std;
using namespace ::android::base;

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

extern size_t readConfigFile(const char *fileName, uint8_t **p_data);

static string phNxpNciHal_parseBytesString(string in);
static bool phNxpNciHal_parseValueFromString(string &in);
static bool phNxpNciHal_CheckKeyNeeded(string key);
static string phNxpNciHal_UpdatePwrStateConfigs(string &config);
static bool phNxpNciHal_IsAutonmousModeSet(string config);
static string phNxpNciHal_extractConfig(string &config);
static void phNxpNciHal_getFilteredConfig(string &config);

typedef std::map<std::string, std::string> systemProperty;
systemProperty gsystemProperty = {
    {"nfc.fw.rfreg_ver", "0"},     {"nfc.fw.rfreg_display_ver", "0"},
    {"nfc.fw.dfl_areacode", "0"},  {"nfc.fw.downloadmode_force", "0"},
    {"nfc.nxp.fwdnldstatus", "0"},
};
const char default_nxp_config_path[] = "/vendor/etc/libnfc-nxp.conf";
std::set<string> gNciConfigs = {"NXP_SE_COLD_TEMP_ERROR_DELAY",
                                "NXP_SWP_RD_TAG_OP_TIMEOUT",
                                "NXP_DUAL_UICC_ENABLE",
                                "DEFAULT_AID_ROUTE",
                                "DEFAULT_MIFARE_CLT_ROUTE",
                                "DEFAULT_FELICA_CLT_ROUTE",
                                "DEFAULT_AID_PWR_STATE",
                                "DEFAULT_DESFIRE_PWR_STATE",
                                "DEFAULT_MIFARE_CLT_PWR_STATE",
                                "DEFAULT_FELICA_CLT_PWR_STATE",
                                "HOST_LISTEN_TECH_MASK",
                                "FORWARD_FUNCTIONALITY_ENABLE",
                                "DEFUALT_GSMA_PWR_STATE",
                                "NXP_DEFAULT_UICC2_SELECT",
                                "NXP_SMB_TRANSCEIVE_TIMEOUT",
                                "NXP_SMB_ERROR_RETRY",
                                "NXP_CHECK_DEFAULT_PROTO_SE_ID",
                                "NXPLOG_NCIHAL_LOGLEVEL",
                                "NXPLOG_EXTNS_LOGLEVEL",
                                "NXPLOG_TML_LOGLEVEL",
                                "NXPLOG_FWDNLD_LOGLEVEL",
                                "NXPLOG_NCIX_LOGLEVEL",
                                "NXPLOG_NCIR_LOGLEVEL",
                                "NXP_NFC_SE_TERMINAL_NUM",
                                "NXP_POLL_FOR_EFD_TIMEDELAY",
                                "NXP_NFCC_MERGE_SAK_ENABLE",
                                "NXP_STAG_TIMEOUT_CFG",
                                "DEFAULT_T4TNFCEE_AID_POWER_STATE",
                                "RF_STORAGE",
                                "FW_STORAGE",
                                "NXP_CORE_CONF",
                                "NXP_RF_FILE_VERSION_INFO",
                                "NXP_AUTONOMOUS_ENABLE",
                                "NXP_PROP_RESET_EMVCO_CMD",
                                "NFA_CONFIG_FORMAT",
                                "NXP_T4T_NFCEE_ENABLE",
                                "NXP_DISCONNECT_TAG_IN_SCRN_OFF",
                                "OFF_HOST_SIM2_PIPE_ID"};

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
  (void)reason;
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
string phNxpNciHal_loadPersistLog(uint8_t index) {
  /* This is dummy API */
  string reason;
  switch (index) {
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
string phNxpNciHal_getSystemProperty(string key) {
  string propValue;
  std::map<std::string, std::string>::iterator prop;

  if (key == "libnfc-nxp.conf") {
    return phNxpNciHal_getNxpConfigIf();
  } else {
    prop = gsystemProperty.find(key);
    if (prop != gsystemProperty.end()) {
      propValue = prop->second;
    } else {
      /* else Pass a null string */
    }
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
  NXPLOG_NCIHAL_D("%s : Enter Key = %s, value = %s", __func__, key.c_str(),
                  value.c_str());

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
  uint8_t *p_config = nullptr;
  size_t config_size = readConfigFile(default_nxp_config_path, &p_config);
  if (config_size) {
    config.assign((char *)p_config, config_size);
    free(p_config);
    phNxpNciHal_getFilteredConfig(config);
  }
  return config;
}

/*******************************************************************************
**
** Function         phNxpNciHal_getFilteredConfig
**
** Description      It reads only configs needed for libnfc from
*                   libnfc-nxp.conf
**
** Parameters       string config
**
** Returns          void
*******************************************************************************/
static void phNxpNciHal_getFilteredConfig(string &config) {
  config = phNxpNciHal_extractConfig(config);

  if (phNxpNciHal_IsAutonmousModeSet(config)) {
    config = phNxpNciHal_UpdatePwrStateConfigs(config);
  }
}

/*******************************************************************************
**
** Function         phNxpNciHal_extractConfig
**
** Description      It parses complete config file and extracts only
*                   enabled options ignores comments etc.
**
** Parameters       string config
**
** Returns          Resultant string
*******************************************************************************/
static string phNxpNciHal_extractConfig(string &config) {
  stringstream ss(config);
  string line;
  string result;
  while (getline(ss, line)) {
    line = Trim(line);
    if (line.empty())
      continue;
    if (line.at(0) == '#')
      continue;
    if (line.at(0) == 0)
      continue;

    auto search = line.find('=');
    if (search == string::npos)
      continue;

    string key(Trim(line.substr(0, search)));
    if (!phNxpNciHal_CheckKeyNeeded(key))
      continue;
    if (key == "NXP_NFC_SE_TERMINAL_NUM") {
      line = "NXP_SE_APDU_GATE_SUPPORT=0x01\n";
      result += line;
      continue;
    }
    string value_string(Trim(line.substr(search + 1, string::npos)));

    if (!phNxpNciHal_parseValueFromString(value_string))
      continue;

    line = key + "=" + value_string + "\n";
    result += line;
  }

  return result;
}

/*******************************************************************************
**
** Function         phNxpNciHal_IsAutonmousModeSet
**
** Description      It check whether autonomous mode is enabled
*                   in config file
**
** Parameters       string config
**
** Returns          boolean(TRUE/FALSE)
*******************************************************************************/
static bool phNxpNciHal_IsAutonmousModeSet(string config) {
  stringstream ss(config);
  string line;
  unsigned tmp = 0;
  while (getline(ss, line)) {
    auto search = line.find('=');
    if (search == string::npos)
      continue;

    string key(Trim(line.substr(0, search)));
    if (key == "NXP_AUTONOMOUS_ENABLE") {
      string value(Trim(line.substr(search + 1, string::npos)));
      if (ParseUint(value.c_str(), &tmp)) {
        if (tmp == 1) {
          return true;
        } else {
          NXPLOG_NCIHAL_D("Autonomous flag disabled");
          return false;
        }
      }
    } else {
      continue;
    }
  }
  NXPLOG_NCIHAL_D("Autonomous flag disabled");
  return false;
}

/*******************************************************************************
**
** Function         phNxpNciHal_UpdatePwrStateConfigs
**
** Description      Updates default pwr state accordingly if autonomous mode
*                   is enabled
**
** Parameters       string config
**
** Returns          Resultant string
*******************************************************************************/
static string phNxpNciHal_UpdatePwrStateConfigs(string &config) {
  stringstream ss(config);
  string line;
  string result;
  unsigned tmp = 0;
  while (getline(ss, line)) {
    auto search = line.find('=');
    if (search == string::npos)
      continue;

    string key(Trim(line.substr(0, search)));
    if ((key == "DEFAULT_AID_PWR_STATE" || key == "DEFAULT_DESFIRE_PWR_STATE" ||
         key == "DEFAULT_MIFARE_CLT_PWR_STATE" ||
         key == "DEFAULT_FELICA_CLT_PWR_STATE")) {
      string value(Trim(line.substr(search + 1, string::npos)));
      if (ParseUint(value.c_str(), &tmp)) {
        tmp = phNxpNciHal_updateAutonomousPwrState(tmp);
        value = to_string(tmp);
        line = key + "=" + value + "\n";
        result += line;
      }
    } else {
      result += (line + "\n");
      continue;
    }
  }
  return result;
}

/*******************************************************************************
**
** Function         phNxpNciHal_CheckKeyNeeded
**
** Description      Check if the config needed for libnfc as per gNciConfigs
*                   list
**
** Parameters       string config
**
** Returns          bool(true/false)
*******************************************************************************/
static bool phNxpNciHal_CheckKeyNeeded(string key) {
  return ((gNciConfigs.find(key) != gNciConfigs.end()) ? true : false);
}

/*******************************************************************************
**
** Function         phNxpNciHal_parseValueFromString
**
** Description      Parse value determine data type of config option
**
** Parameters       string config
**
** Returns          bool(true/false)
*******************************************************************************/
static bool phNxpNciHal_parseValueFromString(string &in) {
  unsigned tmp = 0;
  bool stat = false;
  if (in.length() >= 1) {
    switch (in[0]) {
    case '"':
      if (in[in.length() - 1] == '"' && in.length() > 2)
        stat = true;
      break;
    case '{':
      if (in[in.length() - 1] == '}' && in.length() >= 3) {
        in = phNxpNciHal_parseBytesString(in);
        stat = true;
      }
      break;
    default:
      if (ParseUint(in.c_str(), &tmp))
        stat = true;
      break;
    }
  } else {
    NXPLOG_NCIHAL_E("%s : Invalid config string ", __func__);
  }
  return stat;
}

/*******************************************************************************
**
** Function         phNxpNciHal_parseBytesString
**
** Description      Parse bytes from string
**
** Parameters       string config
**
** Returns          Resultant string
*******************************************************************************/
static string phNxpNciHal_parseBytesString(string in) {
  size_t pos;
  in.erase(remove(in.begin(), in.end(), ' '), in.end());
  pos = in.find(",");
  while (pos != string::npos) {
    in = in.replace(pos, 1, ":");
    pos = in.find(",", pos);
  }
  return in;
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

  if (pInpOutData == NULL || pInpOutData->inp.data.nciCmd.cmd_len == 0) {
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
