/*
 * Copyright (C) 2012-2018 NXP Semiconductors
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

#include <phDal4Nfc_messageQueueLib.h>
#include <phDnldNfc.h>
#include <phNxpConfig.h>
#include <phNxpLog.h>
#include <phNxpNciHal.h>
#include <phNxpNciHal_Adaptation.h>
#include <phNxpNciHal_Dnld.h>
#include <phNxpNciHal_NfcDepSWPrio.h>
#include <phNxpNciHal_ext.h>
#include <phTmlNfc.h>
#include <phTmlNfc_i2c.h>

#include <sys/stat.h>
#include <string.h>
#include <EseAdaptation.h>
using namespace android::hardware::nfc::V1_1;
/*********************** Global Variables *************************************/
#define PN547C2_CLOCK_SETTING
#undef PN547C2_FACTORY_RESET_DEBUG
#define CORE_RES_STATUS_BYTE 3
/* FW Mobile major number */
#define FW_MOBILE_MAJOR_NUMBER_PN553 0x01
#define FW_MOBILE_MAJOR_NUMBER_PN81A 0x02
#define FW_MOBILE_MAJOR_NUMBER_PN551 0x05
#define FW_MOBILE_MAJOR_NUMBER_PN48AD 0x01

#if (NFC_NXP_CHIP_TYPE == PN551)
#define FW_MOBILE_MAJOR_NUMBER FW_MOBILE_MAJOR_NUMBER_PN551
#elif (NFC_NXP_CHIP_TYPE == PN553)
#define FW_MOBILE_MAJOR_NUMBER FW_MOBILE_MAJOR_NUMBER_PN553
#else
#define FW_MOBILE_MAJOR_NUMBER FW_MOBILE_MAJOR_NUMBER_PN48AD
#endif
/* Processing of ISO 15693 EOF */
extern uint8_t icode_send_eof;
extern uint8_t icode_detected;
static uint8_t cmd_icode_eof[] = {0x00, 0x00, 0x00};
static const char *rf_block_num[] = {"1","2","3","4","5","6","7","8","9","10",
"11","12","13","14","15","16","17","18","19","20"};
const char *rf_block_name = "NXP_RF_CONF_BLK_";
static uint8_t read_failed_disable_nfc = false;
/* FW download success flag */
static uint8_t fw_download_success = 0;

static uint8_t config_access = false;
static uint8_t config_success = true;
static NFCSTATUS phNxpNciHal_FwDwnld(uint16_t aType);
/* NCI HAL Control structure */
phNxpNciHal_Control_t nxpncihal_ctrl;

/* NXP Poll Profile structure */
phNxpNciProfile_Control_t nxpprofile_ctrl;

/* TML Context */
extern phTmlNfc_Context_t* gpphTmlNfc_Context;
extern void phTmlNfc_set_fragmentation_enabled(
    phTmlNfc_i2cfragmentation_t result);

extern int phNxpNciHal_CheckFwRegFlashRequired(uint8_t* fw_update_req,
                                               uint8_t* rf_update_req);
extern NFCSTATUS phNxpNciHal_ext_send_sram_config_to_flash();
extern NFCSTATUS phNxpNciHal_enableDefaultUICC2SWPline(uint8_t uicc2_sel);

phNxpNci_getCfg_info_t* mGetCfg_info = NULL;
uint32_t gSvddSyncOff_Delay = 10;
bool_t force_fw_download_req = false;
bool_t gParserCreated = FALSE;
/* global variable to get FW version from NCI response*/
uint32_t wFwVerRsp;
/* External global variable to get FW version */
extern uint16_t wFwVer;

extern uint16_t fw_maj_ver;
extern uint16_t rom_version;
#if (NFC_NXP_CHIP_TYPE != PN547C2)
extern uint8_t gRecFWDwnld;
static uint8_t gRecFwRetryCount;  // variable to hold dummy FW recovery count
#endif
static uint8_t write_unlocked_status = NFCSTATUS_SUCCESS;
static uint8_t Rx_data[NCI_MAX_DATA_LEN];

#if (NFC_NXP_CHIP_TYPE == PN548C2)
uint8_t discovery_cmd[50] = {0};
uint8_t discovery_cmd_len = 0;
#endif
uint32_t timeoutTimerId = 0;
#ifndef FW_DWNLD_FLAG
uint8_t fw_dwnld_flag = false;
#endif
phNxpNciHal_Sem_t config_data;

phNxpNciClock_t phNxpNciClock = {0, {0}, false};

phNxpNciRfSetting_t phNxpNciRfSet = {false, {0}};

phNxpNciMwEepromArea_t phNxpNciMwEepromArea = {false, {0}};

/**************** local methods used in this file only ************************/
static NFCSTATUS phNxpNciHal_fw_download(void);
static void phNxpNciHal_open_complete(NFCSTATUS status);
static void phNxpNciHal_write_complete(void* pContext,
                                       phTmlNfc_TransactInfo_t* pInfo);
static void phNxpNciHal_read_complete(void* pContext,
                                      phTmlNfc_TransactInfo_t* pInfo);
static void phNxpNciHal_close_complete(NFCSTATUS status);
static void phNxpNciHal_core_initialized_complete(NFCSTATUS status);
static void phNxpNciHal_power_cycle_complete(NFCSTATUS status);
static void phNxpNciHal_kill_client_thread(
    phNxpNciHal_Control_t* p_nxpncihal_ctrl);
static void* phNxpNciHal_client_thread(void* arg);
static void phNxpNciHal_get_clk_freq(void);
//static void phNxpNciHal_set_clock(void);
static void phNxpNciHal_nfccClockCfgRead(void);
static NFCSTATUS phNxpNciHal_nfccClockCfgApply(void);
//static void phNxpNciHal_txNfccClockSetCmd(void);

static void phNxpNciHal_check_factory_reset(void);
static void phNxpNciHal_print_res_status(uint8_t* p_rx_data, uint16_t* p_len);
static NFCSTATUS phNxpNciHal_CheckValidFwVersion(void);
//static NFCSTATUS phNxpNciHal_get_mw_eeprom(void);
//static NFCSTATUS phNxpNciHal_set_mw_eeprom(void);
static int phNxpNciHal_fw_mw_ver_check();
NFCSTATUS phNxpNciHal_check_clock_config(void);
NFCSTATUS phNxpNciHal_china_tianjin_rf_setting(void);
#if (NFC_NXP_CHIP_TYPE != PN547C2)
static NFCSTATUS phNxpNciHalRFConfigCmdRecSequence();
static NFCSTATUS phNxpNciHal_CheckRFCmdRespStatus();
#endif
int check_config_parameter();

/******************************************************************************
 * Function         phNxpNciHal_client_thread
 *
 * Description      This function is a thread handler which handles all TML and
 *                  NCI messages.
 *
 * Returns          void
 *
 ******************************************************************************/
static void* phNxpNciHal_client_thread(void* arg) {
  phNxpNciHal_Control_t* p_nxpncihal_ctrl = (phNxpNciHal_Control_t*)arg;
  phLibNfc_Message_t msg;

  NXPLOG_NCIHAL_D("thread started");

  p_nxpncihal_ctrl->thread_running = 1;

  while (p_nxpncihal_ctrl->thread_running == 1) {
    /* Fetch next message from the NFC stack message queue */
    if (phDal4Nfc_msgrcv(p_nxpncihal_ctrl->gDrvCfg.nClientId, &msg, 0, 0) ==
        -1) {
      NXPLOG_NCIHAL_E("NFC client received bad message");
      continue;
    }

    if (p_nxpncihal_ctrl->thread_running == 0) {
      break;
    }

    switch (msg.eMsgType) {
      case PH_LIBNFC_DEFERREDCALL_MSG: {
        phLibNfc_DeferredCall_t* deferCall =
            (phLibNfc_DeferredCall_t*)(msg.pMsgData);

        REENTRANCE_LOCK();
        deferCall->pCallback(deferCall->pParameter);
        REENTRANCE_UNLOCK();

        break;
      }

      case NCI_HAL_OPEN_CPLT_MSG: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
          /* Send the event */
          (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_OPEN_CPLT_EVT,
                                              HAL_NFC_STATUS_OK);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case NCI_HAL_CLOSE_CPLT_MSG: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
          /* Send the event */
          (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_CLOSE_CPLT_EVT,
                                              HAL_NFC_STATUS_OK);
          phNxpNciHal_kill_client_thread(&nxpncihal_ctrl);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case NCI_HAL_POST_INIT_CPLT_MSG: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
          /* Send the event */
          (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_POST_INIT_CPLT_EVT,
                                              HAL_NFC_STATUS_OK);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case NCI_HAL_PRE_DISCOVER_CPLT_MSG: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
          /* Send the event */
          (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_PRE_DISCOVER_CPLT_EVT,
                                              HAL_NFC_STATUS_OK);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case NCI_HAL_ERROR_MSG: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
          /* Send the event */
          (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_ERROR_EVT,
                                              HAL_NFC_STATUS_FAILED);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case NCI_HAL_RX_MSG: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_data_cback != NULL) {
          (*nxpncihal_ctrl.p_nfc_stack_data_cback)(nxpncihal_ctrl.rsp_len,
                                                   nxpncihal_ctrl.p_rsp_data);
        }
        REENTRANCE_UNLOCK();
        break;
      }
    }
  }

  NXPLOG_NCIHAL_D("NxpNciHal thread stopped");

  return NULL;
}

/******************************************************************************
 * Function         phNxpNciHal_kill_client_thread
 *
 * Description      This function safely kill the client thread and clean all
 *                  resources.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_kill_client_thread(
    phNxpNciHal_Control_t* p_nxpncihal_ctrl) {
  NXPLOG_NCIHAL_D("Terminating phNxpNciHal client thread...");

  p_nxpncihal_ctrl->p_nfc_stack_cback = NULL;
  p_nxpncihal_ctrl->p_nfc_stack_data_cback = NULL;
  p_nxpncihal_ctrl->thread_running = 0;

  return;
}

/******************************************************************************
 * Function         phNxpNciHal_fw_download
 *
 * Description      This function download the PN54X secure firmware to IC. If
 *                  firmware version in Android filesystem and firmware in the
 *                  IC is same then firmware download will return with success
 *                  without downloading the firmware.
 *
 * Returns          NFCSTATUS_SUCCESS if firmware download successful
 *                  NFCSTATUS_FAILED in case of failure
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_fw_download(void) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  /*NCI_RESET_CMD*/
  static uint8_t cmd_reset_nci[] = {0x20,0x00,0x01,0x00};
  NFCSTATUS wConfigStatus = NFCSTATUS_FAILED;
  /*phNxpNciHal_get_clk_freq();*/
  phNxpNciHal_nfccClockCfgRead();
  status = phTmlNfc_IoCtl(phTmlNfc_e_EnableDownloadMode);
  if(nfcFL.nfccFL._NFCC_DWNLD_MODE == NFCC_DWNLD_WITH_NCI_CMD)
  /*NCI_RESET_CMD*/
  {
      static uint8_t cmd_reset_nci[] = {0x20,0x00,0x01,0x80};
      nxpncihal_ctrl.fwdnld_mode_reqd = TRUE;
      phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci), cmd_reset_nci);
      nxpncihal_ctrl.fwdnld_mode_reqd = FALSE;
   }
  if (NFCSTATUS_SUCCESS == status) {
    /* Set the obtained device handle to download module */
    phDnldNfc_SetHwDevHandle();
    NXPLOG_NCIHAL_D("Calling Seq handler for FW Download \n");
    status = phNxpNciHal_fw_download_seq(nxpprofile_ctrl.bClkSrcVal,
                                         nxpprofile_ctrl.bClkFreqVal);
    if(nfcFL.nfccFL._NFCC_DWNLD_MODE == NFCC_DWNLD_WITH_NCI_CMD)
    {
      nxpncihal_ctrl.hal_ext_enabled = TRUE;
      nxpncihal_ctrl.nci_info.wait_for_ntf = TRUE;
    }

    if (status != NFCSTATUS_SUCCESS) {
      /* Abort any pending read and write */
      phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci), cmd_reset_nci);
      phTmlNfc_ReadAbort();
      phTmlNfc_WriteAbort();
    }
    phDnldNfc_ReSetHwDevHandle();
    /* call read pending */
    wConfigStatus = phTmlNfc_Read(
        nxpncihal_ctrl.p_cmd_data, NCI_MAX_DATA_LEN,
        (pphTmlNfc_TransactCompletionCb_t)&phNxpNciHal_read_complete, NULL);
    if(nfcFL.nfccFL._NFCC_DWNLD_MODE == NFCC_DWNLD_WITH_NCI_CMD) usleep(100 * 1000);
    if (wConfigStatus != NFCSTATUS_PENDING) {
      NXPLOG_NCIHAL_E("TML Read status error status B= %x", status);
      nxpncihal_ctrl.hal_ext_enabled = FALSE;
      nxpncihal_ctrl.nci_info.wait_for_ntf = FALSE;
      status = NFCSTATUS_FAILED;
    }
  } else {
    status = NFCSTATUS_FAILED;
  }

  return status;
}

/******************************************************************************
 * Function         phNxpNciHal_CheckValidFwVersion
 *
 * Description      This function checks the valid FW for Mobile device.
 *                  If the FW doesn't belong the Mobile device it further
 *                  checks nxp config file to override.
 *
 * Returns          NFCSTATUS_SUCCESS if valid fw version found
 *                  NFCSTATUS_NOT_ALLOWED in case of FW not valid for mobile
 *                  device
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_CheckValidFwVersion(void) {
  NFCSTATUS status = NFCSTATUS_NOT_ALLOWED;
  const unsigned char sfw_infra_major_no = 0x02;
  unsigned char ufw_current_major_no = 0x00;

  /* extract the firmware's major no */
  ufw_current_major_no = ((0x00FF) & (wFwVer >> 8U));

  NXPLOG_NCIHAL_D("%s current_major_no = 0x%x", __func__, ufw_current_major_no);

  if (ufw_current_major_no == nfcFL.nfcMwFL._FW_MOBILE_MAJOR_NUMBER) {
    status = NFCSTATUS_SUCCESS;
  } else if (ufw_current_major_no == sfw_infra_major_no) {
        if(rom_version == FW_MOBILE_ROM_VERSION_PN553 || rom_version == FW_MOBILE_ROM_VERSION_PN557) {
          NXPLOG_NCIHAL_D(" PN557  allow Fw download with major number =  0x%x",
          ufw_current_major_no);
          status = NFCSTATUS_SUCCESS;
        } else {
          status = NFCSTATUS_NOT_ALLOWED;
    }
  }
  else if ((nfcFL.chipType != pn547C2) && (gRecFWDwnld == true)) {
    status = NFCSTATUS_SUCCESS;
  }
  else if (wFwVerRsp == 0) {
    NXPLOG_NCIHAL_E(
        "FW Version not received by NCI command >>> Force Firmware download");
    tNFC_chipType chipType = DEFAULT_CHIP_TYPE;
    CONFIGURE_FEATURELIST(chipType);
    status = NFCSTATUS_SUCCESS;
  } else {
    NXPLOG_NCIHAL_E("Wrong FW Version >>> Firmware download not allowed");
  }

  return status;
}
/******************************************************************************
 * Function         phNxpNciHal_FwDwnld
 *
 * Description      This function is called by libnfc-nci after core init is
 *                  completed, to download the firmware.
 *
 * Returns          This function return NFCSTATUS_SUCCES (0) in case of success
 *                  In case of failure returns other failure value.
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_FwDwnld(uint16_t aType) {
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   if(aType != NFC_STATUS_NOT_INITIALIZED_PROP) {
     if (wFwVerRsp == 0) {
       phDnldNfc_InitImgInfo();
     }
     status= phNxpNciHal_CheckValidFwVersion();
  }
  if (NFCSTATUS_SUCCESS == status) {
    NXPLOG_NCIHAL_D("Found Valid Firmware Type");
    status = phNxpNciHal_fw_download();
    if (status != NFCSTATUS_SUCCESS) {
      if (NFCSTATUS_SUCCESS != phNxpNciHal_fw_mw_ver_check()) {
        NXPLOG_NCIHAL_D("Chip Version Middleware Version mismatch!!!!");
        goto clean_and_return;
      }
      NXPLOG_NCIHAL_E("FW Download failed - NFCC init will continue");
    }
  } else {
    if (wFwVerRsp == 0) phDnldNfc_ReSetHwDevHandle();
  }
clean_and_return:
  return status;
}

static void phNxpNciHal_get_clk_freq(void) {
  unsigned long num = 0;
  int isfound = 0;

  nxpprofile_ctrl.bClkSrcVal = 0;
  nxpprofile_ctrl.bClkFreqVal = 0;
  nxpprofile_ctrl.bTimeout = 0;

  isfound = GetNxpNumValue(NAME_NXP_SYS_CLK_SRC_SEL, &num, sizeof(num));
  if (isfound > 0) {
    nxpprofile_ctrl.bClkSrcVal = num;
  }

  num = 0;
  isfound = 0;
  isfound = GetNxpNumValue(NAME_NXP_SYS_CLK_FREQ_SEL, &num, sizeof(num));
  if (isfound > 0) {
    nxpprofile_ctrl.bClkFreqVal = num;
  }

  num = 0;
  isfound = 0;
  isfound = GetNxpNumValue(NAME_NXP_SYS_CLOCK_TO_CFG, &num, sizeof(num));
  if (isfound > 0) {
    nxpprofile_ctrl.bTimeout = num;
  }

  NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bClkSrcVal = 0x%x",
                  nxpprofile_ctrl.bClkSrcVal);
  NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bClkFreqVal = 0x%x",
                  nxpprofile_ctrl.bClkFreqVal);
  NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bClkFreqVal = 0x%x",
                  nxpprofile_ctrl.bTimeout);

  if ((nxpprofile_ctrl.bClkSrcVal < CLK_SRC_XTAL) ||
      (nxpprofile_ctrl.bClkSrcVal > CLK_SRC_PLL)) {
    NXPLOG_FWDNLD_E(
        "Clock source value is wrong in config file, setting it as default");
    nxpprofile_ctrl.bClkSrcVal = NXP_SYS_CLK_SRC_SEL;
  }
  if (nxpprofile_ctrl.bClkFreqVal == CLK_SRC_PLL &&
      (nxpprofile_ctrl.bClkFreqVal < CLK_FREQ_13MHZ ||
       nxpprofile_ctrl.bClkFreqVal > CLK_FREQ_52MHZ)) {
    NXPLOG_FWDNLD_E(
        "Clock frequency value is wrong in config file, setting it as default");
    nxpprofile_ctrl.bClkFreqVal = NXP_SYS_CLK_FREQ_SEL;
  }
  if ((nxpprofile_ctrl.bTimeout < CLK_TO_CFG_DEF) ||
      (nxpprofile_ctrl.bTimeout > CLK_TO_CFG_MAX)) {
    NXPLOG_FWDNLD_E(
        "Clock timeout value is wrong in config file, setting it as default");
    nxpprofile_ctrl.bTimeout = CLK_TO_CFG_DEF;
  }
}

/******************************************************************************
 * Function         phNxpNciHal_open
 *
 * Description      This function is called by libnfc-nci during the
 *                  initialization of the NFCC. It opens the physical connection
 *                  with NFCC (PN54X) and creates required client thread for
 *                  operation.
 *                  After open is complete, status is informed to libnfc-nci
 *                  through callback function.
 *
 * Returns          This function return NFCSTATUS_SUCCES (0) in case of success
 *                  In case of failure returns other failure value.
 *
 ******************************************************************************/
int phNxpNciHal_open(nfc_stack_callback_t* p_cback,
                     nfc_stack_data_callback_t* p_data_cback) {
  phOsalNfc_Config_t tOsalConfig;
  phTmlNfc_Config_t tTmlConfig;
  char* nfc_dev_node = NULL;
  const uint16_t max_len = 260;
  NFCSTATUS wConfigStatus = NFCSTATUS_SUCCESS;
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  /*NCI_INIT_CMD*/
  static uint8_t cmd_init_nci[] = {0x20, 0x01, 0x00};
  /*NCI_RESET_CMD*/
  static uint8_t cmd_reset_nci[] = {0x20, 0x00, 0x01, 0x00};
  /*NCI2_0_INIT_CMD*/
  static uint8_t cmd_init_nci2_0[] = {0x20, 0x01, 0x02, 0x00, 0x00};
  uint8_t fw_dwld_req = 0;
  int     isfound = 0;
  unsigned long num = 0;
  if (nxpncihal_ctrl.halStatus == HAL_STATUS_OPEN) {
    NXPLOG_NCIHAL_E("phNxpNciHal_open already open");
    return NFCSTATUS_SUCCESS;
  }
  /* reset config cache */
  resetNxpConfig();

  int init_retry_cnt = 0;
  int8_t ret_val = 0x00;

  /* initialize trace level */
  phNxpLog_InitializeLogLevel();

  if (phNxpNciHal_init_monitor() == NULL) {
    NXPLOG_NCIHAL_E("Init monitor failed");
    return NFCSTATUS_FAILED;
  }

  CONCURRENCY_LOCK();
  memset(&nxpncihal_ctrl, 0x00, sizeof(nxpncihal_ctrl));
  memset(&tOsalConfig, 0x00, sizeof(tOsalConfig));
  memset(&tTmlConfig, 0x00, sizeof(tTmlConfig));
  memset(&nxpprofile_ctrl, 0, sizeof(phNxpNciProfile_Control_t));

  /* By default HAL status is HAL_STATUS_OPEN */
  nxpncihal_ctrl.halStatus = HAL_STATUS_OPEN;

  nxpncihal_ctrl.p_nfc_stack_cback = p_cback;
  nxpncihal_ctrl.p_nfc_stack_data_cback = p_data_cback;
  /*nci version NCI_VERSION_UNKNOWN version by default*/
  nxpncihal_ctrl.nci_info.nci_version = NCI_VERSION_UNKNOWN;
  /*Structure related to set config management*/
  mGetCfg_info = NULL;
  mGetCfg_info = (phNxpNci_getCfg_info_t*)nxp_malloc(sizeof(phNxpNci_getCfg_info_t));
  if (mGetCfg_info == NULL) {
    goto clean_and_return;
  }
  memset(mGetCfg_info, 0x00, sizeof(phNxpNci_getCfg_info_t));

  /* Read the nfc device node name */
  nfc_dev_node = (char*)malloc(max_len * sizeof(char));
  if (nfc_dev_node == NULL) {
    NXPLOG_NCIHAL_E("malloc of nfc_dev_node failed ");
    goto clean_and_return;
  } else if (!GetNxpStrValue(NAME_NXP_NFC_DEV_NODE, nfc_dev_node,
                             sizeof(nfc_dev_node))) {
    NXPLOG_NCIHAL_E(
        "Invalid nfc device node name keeping the default device node "
        "/dev/pn54x");
    strcpy(nfc_dev_node, "/dev/pn54x");
  }

  isfound = GetNxpNumValue(NAME_NXP_ALWAYS_FW_UPDATE, &num, sizeof(num));
  if (isfound > 0) {
    fw_dwld_req = num;
  }
  /* Configure hardware link */
  nxpncihal_ctrl.gDrvCfg.nClientId = phDal4Nfc_msgget(0, 0600);
  nxpncihal_ctrl.gDrvCfg.nLinkType = ENUM_LINK_TYPE_I2C; /* For PN54X */
  tTmlConfig.pDevName = (int8_t*)nfc_dev_node;
  tOsalConfig.dwCallbackThreadId = (uintptr_t)nxpncihal_ctrl.gDrvCfg.nClientId;
  tOsalConfig.pLogFile = NULL;
  tTmlConfig.dwGetMsgThreadId = (uintptr_t)nxpncihal_ctrl.gDrvCfg.nClientId;

  /*Create the timer for extns write response*/
  timeoutTimerId = phOsalNfc_Timer_Create();
  /* Initialize TML layer */
  wConfigStatus = phTmlNfc_Init(&tTmlConfig);
  if (wConfigStatus != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("phTmlNfc_Init Failed");
    goto clean_and_return;
  } else {
    if (nfc_dev_node != NULL) {
      free(nfc_dev_node);
      nfc_dev_node = NULL;
    }
  }

  /* Create the client thread */
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  ret_val = pthread_create(&nxpncihal_ctrl.client_thread, &attr,
                           phNxpNciHal_client_thread, &nxpncihal_ctrl);
  pthread_attr_destroy(&attr);
  if (ret_val != 0) {
    NXPLOG_NCIHAL_E("pthread_create failed");
    wConfigStatus = phTmlNfc_Shutdown();
    goto clean_and_return;
  }

  CONCURRENCY_UNLOCK();

  /* call read pending */
  status = phTmlNfc_Read(
      nxpncihal_ctrl.p_cmd_data, NCI_MAX_DATA_LEN,
      (pphTmlNfc_TransactCompletionCb_t)&phNxpNciHal_read_complete, NULL);
  if (status != NFCSTATUS_PENDING) {
    NXPLOG_NCIHAL_E("TML Read status error status = %x", status);
    wConfigStatus = phTmlNfc_Shutdown();
    wConfigStatus = NFCSTATUS_FAILED;
    goto clean_and_return;
  }

init_retry:

  phNxpNciHal_ext_init();

  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci), cmd_reset_nci);
  if ((status != NFCSTATUS_SUCCESS) &&
      (nxpncihal_ctrl.retry_cnt >= MAX_RETRY_COUNT)) {
    NXPLOG_NCIHAL_E("Force FW Download, NFCC not coming out from Standby");
    wConfigStatus = NFCSTATUS_FAILED;
    goto force_download;
  } else if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("NCI_CORE_RESET: Failed");
    if (init_retry_cnt < 3) {
      init_retry_cnt++;
      /*(void)phNxpNciHal_power_cycle();*/
      goto init_retry;
    } else if(init_retry_cnt < MAX_RETRY_COUNT) {
          NXPLOG_NCIHAL_E("invlaid core reset rsp received. Trying Force FW download");
          goto force_download;
      } else init_retry_cnt = 0;
    wConfigStatus = phTmlNfc_Shutdown();
    wConfigStatus = NFCSTATUS_FAILED;
    goto clean_and_return;
  }

    if(nxpncihal_ctrl.nci_info.nci_version == NCI_VERSION_2_0) {
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci2_0), cmd_init_nci2_0);
    } else {
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci), cmd_init_nci);
        if(status == NFCSTATUS_SUCCESS && (nfcFL.chipType == pn557 || (nfcFL.chipType == pn81T)
            || (nfcFL.chipType == sn100u))) {
            NXPLOG_NCIHAL_E("Chip is in NCI1.0 mode reset the chip to 2.0 mode");
            status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci), cmd_reset_nci);
            if(status == NFCSTATUS_SUCCESS) {
                status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci2_0), cmd_init_nci2_0);
                if(status != NFCSTATUS_SUCCESS) {
                    goto init_retry;
                }
            }
        }
      }
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("NCI_CORE_INIT : Failed");
    if (init_retry_cnt < 3) {
      init_retry_cnt++;
      /*(void)phNxpNciHal_power_cycle();*/
      goto init_retry;
    } else
      init_retry_cnt = 0;
    wConfigStatus = phTmlNfc_Shutdown();
    wConfigStatus = NFCSTATUS_FAILED;
    goto clean_and_return;
  }

  /*Get FW version from device*/
  status = phDnldNfc_InitImgInfo();
  NXPLOG_NCIHAL_E("FW version for FW file = 0x%x", wFwVer);
  NXPLOG_NCIHAL_E("FW version from device = 0x%x", wFwVerRsp);
  if ((wFwVerRsp & 0x0000FFFF) == wFwVer && fw_dwld_req == 0) {
    NXPLOG_NCIHAL_D("FW uptodate not required");
    phDnldNfc_ReSetHwDevHandle();
  } else {
  force_download:
    if (wFwVerRsp == 0) {
      phDnldNfc_InitImgInfo();
    }
    if (NFCSTATUS_SUCCESS == phNxpNciHal_CheckValidFwVersion()) {
      NXPLOG_NCIHAL_D("FW update required");
      fw_download_success = 0;
      status = phNxpNciHal_fw_download();
      if (status != NFCSTATUS_SUCCESS) {
        if (NFCSTATUS_SUCCESS != phNxpNciHal_fw_mw_ver_check()) {
          NXPLOG_NCIHAL_D("Chip Version Middleware Version mismatch!!!!");
          phOsalNfc_Timer_Cleanup();
          phTmlNfc_Shutdown();
          wConfigStatus = NFCSTATUS_FAILED;
          goto clean_and_return;
        }
        NXPLOG_NCIHAL_E("FW Download failed - NFCC init will continue");
      } else {
        wConfigStatus = NFCSTATUS_SUCCESS;
        fw_download_success = 1;
        /* call read pending */
        status = phTmlNfc_Read(
            nxpncihal_ctrl.p_cmd_data, NCI_MAX_DATA_LEN,
            (pphTmlNfc_TransactCompletionCb_t)&phNxpNciHal_read_complete, NULL);
        if (status != NFCSTATUS_PENDING) {
          NXPLOG_NCIHAL_E("TML Read status error status A= %x", status);
          nxpncihal_ctrl.hal_ext_enabled = FALSE;
          nxpncihal_ctrl.nci_info.wait_for_ntf = FALSE;
          wConfigStatus = NFCSTATUS_SUCCESS;
        }
      }
    } else {
      if (wFwVerRsp == 0) phDnldNfc_ReSetHwDevHandle();
    }
  }
  /* Call open complete */
  phNxpNciHal_open_complete(wConfigStatus);

  return wConfigStatus;

clean_and_return:
  CONCURRENCY_UNLOCK();
  if (nfc_dev_node != NULL) {
    free(nfc_dev_node);
    nfc_dev_node = NULL;
  }
  if (mGetCfg_info != NULL) {
    free(mGetCfg_info);
    mGetCfg_info = NULL;
  }
  /* Report error status */
  if (p_cback != NULL) {
    (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_OPEN_CPLT_EVT,
                                      HAL_NFC_STATUS_FAILED);
  }

  nxpncihal_ctrl.p_nfc_stack_cback = NULL;
  nxpncihal_ctrl.p_nfc_stack_data_cback = NULL;
  phNxpNciHal_cleanup_monitor();
  nxpncihal_ctrl.halStatus = HAL_STATUS_CLOSE;
  return NFCSTATUS_FAILED;
}

/******************************************************************************
 * Function         phNxpNciHal_fw_mw_check
 *
 * Description      This function inform the status of phNxpNciHal_fw_mw_check
 *                  function to libnfc-nci.
 *
 * Returns          int.
 *
 ******************************************************************************/
int phNxpNciHal_fw_mw_ver_check() {
    NFCSTATUS status = NFCSTATUS_FAILED;

    if (((nfcFL.chipType == pn553)||(nfcFL.chipType == pn80T)) &&
            (rom_version == 0x11) && (fw_maj_ver == 0x01)) {
        status = NFCSTATUS_SUCCESS;
    } else if (((nfcFL.chipType == pn551)||(nfcFL.chipType == pn67T)) &&
            (rom_version == 0x10) && (fw_maj_ver == 0x05)) {
        status = NFCSTATUS_SUCCESS;
    } else if (((nfcFL.chipType == pn548C2)||(nfcFL.chipType == pn66T)) &&
            (rom_version == 0x10) && (fw_maj_ver == 0x01)) {
        status = NFCSTATUS_SUCCESS;
    } else if (((nfcFL.chipType == pn547C2)||(nfcFL.chipType == pn65T)) &&
            (rom_version == 0x08) && (fw_maj_ver == 0x01)) {
        status = NFCSTATUS_SUCCESS;
    }else if ((nfcFL.chipType == sn100u) &&
            (rom_version == 0x01) && (fw_maj_ver == 0x10)) {
        status = NFCSTATUS_SUCCESS;
    }
  return status;
}
/******************************************************************************
 * Function         phNxpNciHal_open_complete
 *
 * Description      This function inform the status of phNxpNciHal_open
 *                  function to libnfc-nci.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_open_complete(NFCSTATUS status) {
  static phLibNfc_Message_t msg;

  if (status == NFCSTATUS_SUCCESS) {
    msg.eMsgType = NCI_HAL_OPEN_CPLT_MSG;
    nxpncihal_ctrl.hal_open_status = true;
  } else {
    msg.eMsgType = NCI_HAL_ERROR_MSG;
  }

  msg.pMsgData = NULL;
  msg.Size = 0;

  phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId,
                        (phLibNfc_Message_t*)&msg);

  return;
}

/******************************************************************************
 * Function         phNxpNciHal_write
 *
 * Description      This function write the data to NFCC through physical
 *                  interface (e.g. I2C) using the PN54X driver interface.
 *                  Before sending the data to NFCC, phNxpNciHal_write_ext
 *                  is called to check if there is any extension processing
 *                  is required for the NCI packet being sent out.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
int phNxpNciHal_write(uint16_t data_len, const uint8_t* p_data) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  static phLibNfc_Message_t msg;
  if (nxpncihal_ctrl.halStatus != HAL_STATUS_OPEN) {
    return NFCSTATUS_FAILED;
  }
  /* Create local copy of cmd_data */
  memcpy(nxpncihal_ctrl.p_cmd_data, p_data, data_len);
  nxpncihal_ctrl.cmd_len = data_len;
  if (nxpncihal_ctrl.cmd_len > NCI_MAX_DATA_LEN) {
    NXPLOG_NCIHAL_D("cmd_len exceeds limit NCI_MAX_DATA_LEN");
    goto clean_and_return;
  }
#ifdef P2P_PRIO_LOGIC_HAL_IMP
  /* Specific logic to block RF disable when P2P priority logic is busy */
  if (p_data[0] == 0x21 && p_data[1] == 0x06 && p_data[2] == 0x01 &&
      EnableP2P_PrioLogic == true) {
    NXPLOG_NCIHAL_D("P2P priority logic busy: Disable it.");
    phNxpNciHal_clean_P2P_Prio();
  }
#endif

  /* Check for NXP ext before sending write */
  status =
      phNxpNciHal_write_ext(&nxpncihal_ctrl.cmd_len, nxpncihal_ctrl.p_cmd_data,
                            &nxpncihal_ctrl.rsp_len, nxpncihal_ctrl.p_rsp_data);
  if (status != NFCSTATUS_SUCCESS) {
    /* Do not send packet to PN54X, send response directly */
    msg.eMsgType = NCI_HAL_RX_MSG;
    msg.pMsgData = NULL;
    msg.Size = 0;

    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId,
                          (phLibNfc_Message_t*)&msg);
    goto clean_and_return;
  }

  CONCURRENCY_LOCK();
  data_len = phNxpNciHal_write_unlocked(nxpncihal_ctrl.cmd_len,
                                        nxpncihal_ctrl.p_cmd_data);
  CONCURRENCY_UNLOCK();

  if (icode_send_eof == 1) {
    usleep(10000);
    icode_send_eof = 2;
    phNxpNciHal_send_ext_cmd(3, cmd_icode_eof);
  }

clean_and_return:
  /* No data written */
  return data_len;
}

/******************************************************************************
 * Function         phNxpNciHal_write_unlocked
 *
 * Description      This is the actual function which is being called by
 *                  phNxpNciHal_write. This function writes the data to NFCC.
 *                  It waits till write callback provide the result of write
 *                  process.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
int phNxpNciHal_write_unlocked(uint16_t data_len, const uint8_t* p_data) {
  NFCSTATUS status = NFCSTATUS_INVALID_PARAMETER;
  phNxpNciHal_Sem_t cb_data;
  nxpncihal_ctrl.retry_cnt = 0;
  static uint8_t reset_ntf[] = {0x60, 0x00, 0x06, 0xA0, 0x00,
                                0xC7, 0xD4, 0x00, 0x00};

  /* Create the local semaphore */
  if (phNxpNciHal_init_cb_data(&cb_data, NULL) != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("phNxpNciHal_write_unlocked Create cb data failed");
    data_len = 0;
    goto clean_and_return;
  }

  /* Create local copy of cmd_data */
  memcpy(nxpncihal_ctrl.p_cmd_data, p_data, data_len);
  nxpncihal_ctrl.cmd_len = data_len;

retry:

  data_len = nxpncihal_ctrl.cmd_len;

  status = phTmlNfc_Write(
      (uint8_t*)nxpncihal_ctrl.p_cmd_data, (uint16_t)nxpncihal_ctrl.cmd_len,
      (pphTmlNfc_TransactCompletionCb_t)&phNxpNciHal_write_complete,
      (void*)&cb_data);
  if (status != NFCSTATUS_PENDING) {
    NXPLOG_NCIHAL_E("write_unlocked status error");
    data_len = 0;
    goto clean_and_return;
  }

  /* Wait for callback response */
  if (SEM_WAIT(cb_data)) {
    NXPLOG_NCIHAL_E("write_unlocked semaphore error");
    data_len = 0;
    goto clean_and_return;
  }

  if (cb_data.status != NFCSTATUS_SUCCESS) {
    data_len = 0;
    if (nxpncihal_ctrl.retry_cnt++ < MAX_RETRY_COUNT) {
      NXPLOG_NCIHAL_E(
          "write_unlocked failed - PN54X Maybe in Standby Mode - Retry");
      if(nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT) {
          /* 5ms delay to give NFCC wake up delay */
          usleep(5000);
      } else {
          /* 1ms delay to give NFCC wake up delay */
          usleep(1000);
}
      goto retry;
    } else {
      NXPLOG_NCIHAL_E(
          "write_unlocked failed - PN54X Maybe in Standby Mode (max count = "
          "0x%x)",
          nxpncihal_ctrl.retry_cnt);

      status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);

      if (NFCSTATUS_SUCCESS == status) {
        NXPLOG_NCIHAL_D("PN54X Reset - SUCCESS\n");
      } else {
        NXPLOG_NCIHAL_D("PN54X Reset - FAILED\n");
      }
      if (nxpncihal_ctrl.p_nfc_stack_data_cback != NULL &&
          nxpncihal_ctrl.hal_open_status == true) {
        if (nxpncihal_ctrl.p_rx_data != NULL) {
          NXPLOG_NCIHAL_D(
              "Send the Core Reset NTF to upper layer, which will trigger the "
              "recovery\n");
          // Send the Core Reset NTF to upper layer, which will trigger the
          // recovery.
          nxpncihal_ctrl.rx_data_len = sizeof(reset_ntf);
          memcpy(nxpncihal_ctrl.p_rx_data, reset_ntf, sizeof(reset_ntf));
          (*nxpncihal_ctrl.p_nfc_stack_data_cback)(nxpncihal_ctrl.rx_data_len,
                                                   nxpncihal_ctrl.p_rx_data);
        } else {
          (*nxpncihal_ctrl.p_nfc_stack_data_cback)(0x00, NULL);
        }
        write_unlocked_status = NFCSTATUS_FAILED;
      }
    }
  } else {
    write_unlocked_status = NFCSTATUS_SUCCESS;
  }

clean_and_return:
  phNxpNciHal_cleanup_cb_data(&cb_data);
  return data_len;
}

/******************************************************************************
 * Function         phNxpNciHal_write_complete
 *
 * Description      This function handles write callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_write_complete(void* pContext,
                                       phTmlNfc_TransactInfo_t* pInfo) {
  phNxpNciHal_Sem_t* p_cb_data = (phNxpNciHal_Sem_t*)pContext;

  if (pInfo->wStatus == NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("write successful status = 0x%x", pInfo->wStatus);
  } else {
    NXPLOG_NCIHAL_E("write error status = 0x%x", pInfo->wStatus);
  }

  p_cb_data->status = pInfo->wStatus;

  SEM_POST(p_cb_data);

  return;
}

/******************************************************************************
 * Function         phNxpNciHal_read_complete
 *
 * Description      This function is called whenever there is an NCI packet
 *                  received from NFCC. It could be RSP or NTF packet. This
 *                  function provide the received NCI packet to libnfc-nci
 *                  using data callback of libnfc-nci.
 *                  There is a pending read called from each
 *                  phNxpNciHal_read_complete so each a packet received from
 *                  NFCC can be provide to libnfc-nci.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_read_complete(void* pContext,
                                      phTmlNfc_TransactInfo_t* pInfo) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  UNUSED_PROP(pContext);
  if (nxpncihal_ctrl.read_retry_cnt == 1) {
    nxpncihal_ctrl.read_retry_cnt = 0;
  }
  if (nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT &&
          (pInfo->wStatus == NFCSTATUS_READ_FAILED)) {
      if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
          read_failed_disable_nfc = true;
          /* Send the event */
          (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_ERROR_EVT,
                  HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
      }
      return;
  }

  if (pInfo->wStatus == NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("read successful status = 0x%x", pInfo->wStatus);

    nxpncihal_ctrl.p_rx_data = pInfo->pBuff;
    nxpncihal_ctrl.rx_data_len = pInfo->wLength;

    status = phNxpNciHal_process_ext_rsp(nxpncihal_ctrl.p_rx_data,
                                         &nxpncihal_ctrl.rx_data_len);

    phNxpNciHal_print_res_status(nxpncihal_ctrl.p_rx_data,
                                 &nxpncihal_ctrl.rx_data_len);
    /* Check if response should go to hal module only */
    if (nxpncihal_ctrl.hal_ext_enabled == TRUE &&
        (nxpncihal_ctrl.p_rx_data[0x00] & NCI_MT_MASK) == NCI_MT_RSP) {
      if (status == NFCSTATUS_FAILED) {
        NXPLOG_NCIHAL_D("enter into NFCC init recovery");
        nxpncihal_ctrl.ext_cb_data.status = status;
      }
      /* Unlock semaphore only for responses*/
      if ((nxpncihal_ctrl.p_rx_data[0x00] & NCI_MT_MASK) == NCI_MT_RSP ||
          ((icode_detected == true) && (icode_send_eof == 3))) {
        /* Unlock semaphore */
        SEM_POST(&(nxpncihal_ctrl.ext_cb_data));
      }
    }  // Notification Checking
    else if ((nxpncihal_ctrl.hal_ext_enabled == TRUE) &&
             ((nxpncihal_ctrl.p_rx_data[0x00] & NCI_MT_MASK) == NCI_MT_NTF) &&
             (nxpncihal_ctrl.nci_info.wait_for_ntf == TRUE)) {
      /* Unlock semaphore waiting for only  ntf*/
      SEM_POST(&(nxpncihal_ctrl.ext_cb_data));
      nxpncihal_ctrl.nci_info.wait_for_ntf = FALSE;
    }
    /* Read successful send the event to higher layer */
    else if ((nxpncihal_ctrl.p_nfc_stack_data_cback != NULL) &&
             (status == NFCSTATUS_SUCCESS)) {
      (*nxpncihal_ctrl.p_nfc_stack_data_cback)(nxpncihal_ctrl.rx_data_len,
                                               nxpncihal_ctrl.p_rx_data);
    }
  } else {
    NXPLOG_NCIHAL_E("read error status = 0x%x", pInfo->wStatus);
  }

  if (nxpncihal_ctrl.halStatus == HAL_STATUS_CLOSE &&
      nxpncihal_ctrl.nci_info.wait_for_ntf == FALSE) {
    NXPLOG_NCIHAL_E(" Ignoring read , HAL close triggered");
    return;
  }
  /* Read again because read must be pending always.*/
  status = phTmlNfc_Read(
      Rx_data, NCI_MAX_DATA_LEN,
      (pphTmlNfc_TransactCompletionCb_t)&phNxpNciHal_read_complete, NULL);
  if (status != NFCSTATUS_PENDING) {
    NXPLOG_NCIHAL_E("read status error status = %x", status);
    /* TODO: Not sure how to handle this ? */
  }

  return;
}

void read_retry() {
  /* Read again because read must be pending always.*/
  NFCSTATUS status = phTmlNfc_Read(
      Rx_data, NCI_MAX_DATA_LEN,
      (pphTmlNfc_TransactCompletionCb_t)&phNxpNciHal_read_complete, NULL);
  if (status != NFCSTATUS_PENDING) {
    NXPLOG_NCIHAL_E("read status error status = %x", status);
    /* TODO: Not sure how to handle this ? */
  }
}
/*******************************************************************************
 **
 ** Function:        phNxpNciHal_lastResetNtfReason()
 **
 ** Description:     Returns and clears last reset notification reason.
 **                      Intended to be called only once during recovery.
 **
 ** Returns:         reasonCode
 **
 ********************************************************************************/
uint8_t phNxpNciHal_lastResetNtfReason(void) {
  uint8_t reasonCode = nxpncihal_ctrl.nci_info.lastResetNtfReason;

  nxpncihal_ctrl.nci_info.lastResetNtfReason = 0;

  return reasonCode;
}
/******************************************************************************
 * Function         phNxpNciHal_core_initialized
 *
 * Description      This function is called by libnfc-nci after successful open
 *                  of NFCC. All proprietary setting for PN54X are done here.
 *                  After completion of proprietary settings notification is
 *                  provided to libnfc-nci through callback function.
 *
 * Returns          Always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_core_initialized(uint8_t* p_core_init_rsp_params) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  uint8_t* buffer = NULL;
  uint8_t isfound = false;
#ifdef FW_DWNLD_FLAG
  uint8_t fw_dwnld_flag = false;
#endif
  uint8_t setConfigAlways = false;
  static uint8_t p2p_listen_mode_routing_cmd[] = {0x21, 0x01, 0x07, 0x00, 0x01,
                                                  0x01, 0x03, 0x00, 0x01, 0x05};

  uint8_t swp_full_pwr_mode_on_cmd[] = {0x20, 0x02, 0x05, 0x01,
                                        0xA0, 0xF1, 0x01, 0x01};

  static uint8_t android_l_aid_matching_mode_on_cmd[] = {
      0x20, 0x02, 0x05, 0x01, 0xA0, 0x91, 0x01, 0x01};
  static uint8_t swp_switch_timeout_cmd[] = {0x20, 0x02, 0x06, 0x01, 0xA0,
                                             0xF3, 0x02, 0x00, 0x00};
  static uint8_t cmd_get_cfg_dbg_info[] = {0x20, 0x03, 0x4, 0xA0, 0x1B, 0xA0, 0x27};
  config_success = true;
  long bufflen = 260;
  long retlen = 0;
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
#if (NFC_NXP_HFO_SETTINGS == TRUE)
  /* Temp fix to re-apply the proper clock setting */
  int temp_fix = 1;
#endif
  unsigned long num = 0;
  /*initialize dummy FW recovery variables*/
  if(nfcFL.chipType != pn547C2) {
      gRecFwRetryCount = 0;
      gRecFWDwnld = false;
  }
  // recovery --start
  /*NCI_INIT_CMD*/
  static uint8_t cmd_init_nci[] = {0x20, 0x01, 0x00};
  /*NCI_RESET_CMD*/
  static uint8_t cmd_reset_nci[] = {0x20, 0x00, 0x01,
                                    0x00};  // keep configuration
  static uint8_t cmd_init_nci2_0[] = {0x20, 0x01, 0x02, 0x00, 0x00};
  /* reset config cache */
  static uint8_t retry_core_init_cnt;
  if (nxpncihal_ctrl.halStatus != HAL_STATUS_OPEN) {
    return NFCSTATUS_FAILED;
  }
  if ((*p_core_init_rsp_params > 0) &&
      (*p_core_init_rsp_params < 4))  // initializing for recovery.
  {
  retry_core_init:
    config_access = false;
    if (mGetCfg_info != NULL) {
      mGetCfg_info->isGetcfg = false;
    }
    if (buffer != NULL) {
      free(buffer);
      buffer = NULL;
    }
    if (retry_core_init_cnt > 3) {
        if (nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT &&
                (nxpncihal_ctrl.p_nfc_stack_cback != NULL)) {
            NXPLOG_NCIHAL_D("Posting Core Init Failed\n");
            read_failed_disable_nfc = true;
            (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_ERROR_EVT,
                    HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
        }
        return NFCSTATUS_FAILED;
    }
    if(nfcFL.chipType != sn100u) {
      status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);
      if (NFCSTATUS_SUCCESS == status) {
        NXPLOG_NCIHAL_D("PN54X Reset - SUCCESS\n");
      } else {
        NXPLOG_NCIHAL_D("PN54X Reset - FAILED\n");
      }
    }

    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci), cmd_reset_nci);
    if ((status != NFCSTATUS_SUCCESS) &&
        (nxpncihal_ctrl.retry_cnt >= MAX_RETRY_COUNT)) {
      NXPLOG_NCIHAL_E("Force FW Download, NFCC not coming out from Standby");
      retry_core_init_cnt++;
      goto retry_core_init;
    } else if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("NCI_CORE_RESET: Failed");
      retry_core_init_cnt++;
      goto retry_core_init;
    }

    if (nxpncihal_ctrl.nci_info.nci_version == NCI_VERSION_2_0) {
      status =
          phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci2_0), cmd_init_nci2_0);
    } else {
      status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci), cmd_init_nci);
    }
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("NCI_CORE_INIT : Failed");
      retry_core_init_cnt++;
      goto retry_core_init;
    }

  }
  // recovery --end

  buffer = (uint8_t*)malloc(bufflen * sizeof(uint8_t));
  if (NULL == buffer) {
    return NFCSTATUS_FAILED;
  }
  config_access = true;
  retlen = 0;
  isfound = GetNxpByteArrayValue(NAME_NXP_ACT_PROP_EXTN, (char*)buffer, bufflen,
                                 &retlen);
  if (retlen > 0) {
    /* NXP ACT Proprietary Ext */
    status = phNxpNciHal_send_ext_cmd(retlen, buffer);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("NXP ACT Proprietary Ext failed");
      retry_core_init_cnt++;
      goto retry_core_init;
    }
  }

  retlen = 0;
  isfound = GetNxpByteArrayValue(NAME_NXP_CORE_STANDBY, (char*)buffer, bufflen,
                                 &retlen);
  if (retlen > 0) {
    /* NXP ACT Proprietary Ext */
    status = phNxpNciHal_send_ext_cmd(retlen, buffer);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("Stand by mode enable failed");
      retry_core_init_cnt++;
      goto retry_core_init;    }
  }

  config_access = false;
  if(nfcFL.eseFL._EXCLUDE_NV_MEM_DEPENDENCY == false) {
  phNxpNciHal_check_factory_reset();
  }

  if(nfcFL.chipType == sn100u) {
    uint8_t flash_update_done = FALSE;
    fw_dwnld_flag = fw_download_success;
    if(fw_dwnld_flag)
    {
      mEEPROM_info.buffer = &flash_update_done;
      mEEPROM_info.bufflen = sizeof(flash_update_done);
      mEEPROM_info.request_type = EEPROM_FLASH_UPDATE;
      mEEPROM_info.request_mode = SET_EEPROM_DATA;
      request_EEPROM(&mEEPROM_info);
    }
    else {
      mEEPROM_info.buffer = &flash_update_done;
      mEEPROM_info.bufflen = sizeof(flash_update_done);
      mEEPROM_info.request_type = EEPROM_FLASH_UPDATE;
      mEEPROM_info.request_mode = GET_EEPROM_DATA;
      request_EEPROM(&mEEPROM_info);
      if(flash_update_done == FALSE)
        fw_dwnld_flag = TRUE;
    }
  }
  retlen = 0;
  config_access = true;
  isfound = GetNxpByteArrayValue(NAME_NXP_NFC_PROFILE_EXTN, (char*)buffer,
                                 bufflen, &retlen);
  if (retlen > 0) {
    /* NXP ACT Proprietary Ext */
    status = phNxpNciHal_send_ext_cmd(retlen, buffer);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("NXP ACT Proprietary Ext failed");
      retry_core_init_cnt++;
      goto retry_core_init;
    }
  }

  if (isNxpConfigModified() || (fw_download_success == 1)) {
    retlen = 0;
    fw_download_success = 0;

#if (NFC_NXP_CHIP_TYPE != PN547C2)
    NXPLOG_NCIHAL_D("Performing TVDD Settings");
    isfound = GetNxpNumValue(NAME_NXP_EXT_TVDD_CFG, &num, sizeof(num));
    if (isfound > 0) {
      if (num == 1) {
        isfound = GetNxpByteArrayValue(NAME_NXP_EXT_TVDD_CFG_1, (char*)buffer,
                                       bufflen, &retlen);
        if (retlen > 0) {
          status = phNxpNciHal_send_ext_cmd(retlen, buffer);
          if (status != NFCSTATUS_SUCCESS) {
            NXPLOG_NCIHAL_E("EXT TVDD CFG 1 Settings failed");
            retry_core_init_cnt++;
            goto retry_core_init;
          }
        }
      } else if (num == 2) {
        isfound = GetNxpByteArrayValue(NAME_NXP_EXT_TVDD_CFG_2, (char*)buffer,
                                       bufflen, &retlen);
        if (retlen > 0) {
          status = phNxpNciHal_send_ext_cmd(retlen, buffer);
          if (status != NFCSTATUS_SUCCESS) {
            NXPLOG_NCIHAL_E("EXT TVDD CFG 2 Settings failed");
            retry_core_init_cnt++;
            goto retry_core_init;
          }
        }
      } else if (num == 3) {
        isfound = GetNxpByteArrayValue(NAME_NXP_EXT_TVDD_CFG_3, (char*)buffer,
                                       bufflen, &retlen);
        if (retlen > 0) {
          status = phNxpNciHal_send_ext_cmd(retlen, buffer);
          if (status != NFCSTATUS_SUCCESS) {
            NXPLOG_NCIHAL_E("EXT TVDD CFG 3 Settings failed");
            retry_core_init_cnt++;
            goto retry_core_init;
          }
        }
      } else {
        NXPLOG_NCIHAL_E("Wrong Configuration Value %ld", num);
      }
    }
#endif
  if(phNxpNciHal_lastResetNtfReason() == FW_DBG_REASON_AVAILABLE){

    phNxpNciHal_send_ext_cmd(sizeof(cmd_get_cfg_dbg_info), cmd_get_cfg_dbg_info);
    NXPLOG_NCIHAL_D("NFCC txed reset ntf with reason code 0xA3");
  }
  setConfigAlways = false;
  isfound = GetNxpNumValue(NAME_NXP_SET_CONFIG_ALWAYS, &num, sizeof(num));
  if (isfound > 0) {
    setConfigAlways = num;
  }
  NXPLOG_NCIHAL_D("EEPROM_fw_dwnld_flag : 0x%02x SetConfigAlways flag : 0x%02x",
                  fw_dwnld_flag, setConfigAlways);

  }
  if ((true == fw_dwnld_flag) || (true == setConfigAlways) ||
      isNxpRFConfigModified() || isNxpConfigModified()) {
    config_access = true;
    setConfigAlways = true;

    if (nfcFL.chipType != pn547C2) {
        config_access = true;
    }

    retlen = 0;
    /*Select UICC2/UICC3 SWP line from config param*/
    if (GetNxpNumValue(NAME_NXP_DEFAULT_UICC2_SELECT, (void*)&retlen,
                       sizeof(retlen))) {
      if(retlen > 0)
        phNxpNciHal_enableDefaultUICC2SWPline((uint8_t)retlen);
    }

    if (phNxpNciHal_nfccClockCfgApply() != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("phNxpNciHal_nfccClockCfgApply failed");
        retry_core_init_cnt++;
        goto retry_core_init;
    }

    retlen = 0;
    NXPLOG_NCIHAL_D("Performing NAME_NXP_CORE_CONF_EXTN Settings");
    isfound = GetNxpByteArrayValue(NAME_NXP_CORE_CONF_EXTN, (char*)buffer,
                                   bufflen, &retlen);
    if (retlen > 0) {
      /* NXP ACT Proprietary Ext */
      status = phNxpNciHal_send_ext_cmd(retlen, buffer);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("NXP Core configuration failed");
        retry_core_init_cnt++;
        goto retry_core_init;
      }
    }

    NXPLOG_NCIHAL_D("Performing NAME_NXP_CORE_CONF Settings");
    retlen = 0;
    isfound = GetNxpByteArrayValue(NAME_NXP_CORE_CONF, (char*)buffer, bufflen,
                                   &retlen);
    if (retlen > 0) {
      /* NXP ACT Proprietary Ext */
      status = phNxpNciHal_send_ext_cmd(retlen, buffer);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Core Set Config failed");
        retry_core_init_cnt++;
        goto retry_core_init;
      }
    }

  //if (config_success == false) return NFCSTATUS_FAILED;

#if (NFC_NXP_CHIP_TYPE != PN547C2)
    config_access = false;
#endif
    {
        unsigned long maxBlocks = 0;
        unsigned long loopcnt = 0;

        if (!(GetNxpNumValue(NAME_NXP_RF_CONF_BLK_MAX, &maxBlocks,
                             sizeof(maxBlocks)))) {
          maxBlocks = 0x06;
          NXPLOG_NCIHAL_D("Max rf blocks = 0x%0lX", maxBlocks);
        }
        for(loopcnt = 0; loopcnt < maxBlocks; loopcnt++)
        {
            char rf_conf_block[22] = {'\0'};
            strcpy(rf_conf_block, rf_block_name);
            isfound = GetNxpByteArrayValue(strcat(rf_conf_block, rf_block_num[loopcnt]), (char*)buffer,
                    bufflen, &retlen);
            if (retlen > 0) {
              NXPLOG_NCIHAL_D("Performing RF Settings BLK %ld", loopcnt+1);
              status = phNxpNciHal_send_ext_cmd(retlen, buffer);
#if (NFC_NXP_CHIP_TYPE != PN547C2)
              if (status == NFCSTATUS_SUCCESS) {
                status = phNxpNciHal_CheckRFCmdRespStatus();
                /*STATUS INVALID PARAM 0x09*/
                if (status == 0x09) {
                  phNxpNciHalRFConfigCmdRecSequence();
                  retry_core_init_cnt++;
                  goto retry_core_init;
                }
              } else
#endif
                  if (status != NFCSTATUS_SUCCESS) {
                NXPLOG_NCIHAL_E("RF Settings BLK %ld failed", loopcnt);
                retry_core_init_cnt++;
                goto retry_core_init;
              }
            }
            retlen = 0;
        }
    }
    retlen = 0;
#if (NFC_NXP_CHIP_TYPE != PN547C2)
    config_access = true;
#endif

    isfound = GetNxpByteArrayValue(NAME_NXP_CORE_MFCKEY_SETTING, (char*)buffer,
                                   bufflen, &retlen);
    if (retlen > 0) {
      /* NXP ACT Proprietary Ext */
      status = phNxpNciHal_send_ext_cmd(retlen, buffer);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Setting mifare keys failed");
        retry_core_init_cnt++;
        goto retry_core_init;
      }
    }

    retlen = 0;
    if (nfcFL.chipType != pn547C2) {
        config_access = false;
    }
    isfound = GetNxpByteArrayValue(NAME_NXP_CORE_RF_FIELD, (char*)buffer,
                                   bufflen, &retlen);
    if (retlen > 0) {
      /* NXP ACT Proprietary Ext */
      status = phNxpNciHal_send_ext_cmd(retlen, buffer);
#if (NFC_NXP_CHIP_TYPE != PN547C2)
      if (status == NFCSTATUS_SUCCESS) {
        status = phNxpNciHal_CheckRFCmdRespStatus();
        /*STATUS INVALID PARAM 0x09*/
        if (status == 0x09) {
          phNxpNciHalRFConfigCmdRecSequence();
          retry_core_init_cnt++;
          goto retry_core_init;
        }
      } else
#endif
          if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Setting NXP_CORE_RF_FIELD status failed");
        retry_core_init_cnt++;
        goto retry_core_init;
      }
    }
#if (NFC_NXP_CHIP_TYPE != PN547C2)
    config_access = true;
#endif

    retlen = 0;
#if (NFC_NXP_CHIP_TYPE != PN547C2)
    /* NXP SWP switch timeout Setting*/
    if (GetNxpNumValue(NAME_NXP_SWP_SWITCH_TIMEOUT, (void*)&retlen,
                       sizeof(retlen))) {
      // Check the permissible range [0 - 60]
      if (0 <= retlen && retlen <= 60) {
        if (0 < retlen) {
          unsigned int timeout = retlen * 1000;
          unsigned int timeoutHx = 0x0000;

          char tmpbuffer[10] = {0};
          snprintf((char*)tmpbuffer, 10, "%04x", timeout);
          sscanf((char*)tmpbuffer, "%x", &timeoutHx);

          swp_switch_timeout_cmd[7] = (timeoutHx & 0xFF);
          swp_switch_timeout_cmd[8] = ((timeoutHx & 0xFF00) >> 8);
        }

        status = phNxpNciHal_send_ext_cmd(sizeof(swp_switch_timeout_cmd),
                                          swp_switch_timeout_cmd);
        if (status != NFCSTATUS_SUCCESS) {
          NXPLOG_NCIHAL_E("SWP switch timeout Setting Failed");
          retry_core_init_cnt++;
          goto retry_core_init;
        }
      } else {
        NXPLOG_NCIHAL_E("SWP switch timeout Setting Failed - out of range!");
      }
    }

    status = phNxpNciHal_china_tianjin_rf_setting();
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("phNxpNciHal_china_tianjin_rf_setting failed");
      return NFCSTATUS_FAILED;
    }
#endif
    fw_dwnld_flag = false;
  }

  retlen = 0;

  config_access = false;
  // if recovery mode and length of last command is 0 then only reset the P2P
  // listen mode routing.
  if ((*p_core_init_rsp_params > 0) && (*p_core_init_rsp_params < 4) &&
      p_core_init_rsp_params[35] == 0) {
    /* P2P listen mode routing */
    status = phNxpNciHal_send_ext_cmd(sizeof(p2p_listen_mode_routing_cmd),
                                      p2p_listen_mode_routing_cmd);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("P2P listen mode routing failed");
      retry_core_init_cnt++;
      goto retry_core_init;
    }
  }

  retlen = 0;

  /* SWP FULL PWR MODE SETTING ON */
  if (GetNxpNumValue(NAME_NXP_SWP_FULL_PWR_ON, (void*)&retlen,
                     sizeof(retlen))) {
    if (1 == retlen) {
      status = phNxpNciHal_send_ext_cmd(sizeof(swp_full_pwr_mode_on_cmd),
                                        swp_full_pwr_mode_on_cmd);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("SWP FULL PWR MODE SETTING ON CMD FAILED");
        retry_core_init_cnt++;
        goto retry_core_init;
      }
    } else {
      swp_full_pwr_mode_on_cmd[7] = 0x00;
      status = phNxpNciHal_send_ext_cmd(sizeof(swp_full_pwr_mode_on_cmd),
                                        swp_full_pwr_mode_on_cmd);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("SWP FULL PWR MODE SETTING OFF CMD FAILED");
        retry_core_init_cnt++;
        goto retry_core_init;
      }
    }
  }

  /* Android L AID Matching Platform Setting*/
  if (GetNxpNumValue(NAME_AID_MATCHING_PLATFORM, (void*)&retlen,
                     sizeof(retlen))) {
    if (1 == retlen) {
      status =
          phNxpNciHal_send_ext_cmd(sizeof(android_l_aid_matching_mode_on_cmd),
                                   android_l_aid_matching_mode_on_cmd);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Android L AID Matching Platform Setting Failed");
        retry_core_init_cnt++;
        goto retry_core_init;
      }
    } else if (2 == retlen) {
      android_l_aid_matching_mode_on_cmd[7] = 0x00;
      status =
          phNxpNciHal_send_ext_cmd(sizeof(android_l_aid_matching_mode_on_cmd),
                                   android_l_aid_matching_mode_on_cmd);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Android L AID Matching Platform Setting Failed");
        retry_core_init_cnt++;
        goto retry_core_init;
      }
    }
  }
    config_access = false;
  {
    if(true == setConfigAlways)
    {
        if(nfcFL.chipType == sn100u)
        {
          status = phNxpNciHal_ext_send_sram_config_to_flash();
        }
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci), cmd_reset_nci);
        if (status == NFCSTATUS_SUCCESS) {
            if (nxpncihal_ctrl.nci_info.nci_version == NCI_VERSION_2_0) {
              status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci2_0), cmd_init_nci2_0);
            } else {
          status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci), cmd_init_nci);
        }
        if (status != NFCSTATUS_SUCCESS)
            return status;
        } else {
          return NFCSTATUS_FAILED;
        }
  }
  status = phNxpNciHal_send_get_cfgs();
  if (status == NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("Send get Configs SUCCESS");
  } else {
    NXPLOG_NCIHAL_E("Send get Configs FAILED");
  }
 }
  retry_core_init_cnt = 0;

  if (buffer) {
    free(buffer);
    buffer = NULL;
  }
#if (NFC_NXP_CHIP_TYPE != PN547C2)
  // initialize dummy FW recovery variables
  gRecFWDwnld = 0;
  gRecFwRetryCount = 0;
#endif

  phNxpNciHal_core_initialized_complete(status);

  return NFCSTATUS_SUCCESS;
}
#if (NFC_NXP_CHIP_TYPE != PN547C2)
/******************************************************************************
 * Function         phNxpNciHal_CheckRFCmdRespStatus
 *
 * Description      This function is called to check the resp status of
 *                  RF update commands.
 *
 * Returns          NFCSTATUS_SUCCESS           if successful,
 *                  NFCSTATUS_INVALID_PARAMETER if parameter is inavlid
 *                  NFCSTATUS_FAILED            if failed response
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_CheckRFCmdRespStatus() {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  static uint16_t INVALID_PARAM = 0x09;
  if ((nxpncihal_ctrl.rx_data_len > 0) && (nxpncihal_ctrl.p_rx_data[2] > 0)) {
    if (nxpncihal_ctrl.p_rx_data[3] == 0x09) {
      status = INVALID_PARAM;
    } else if (nxpncihal_ctrl.p_rx_data[3] != NFCSTATUS_SUCCESS) {
      status = NFCSTATUS_FAILED;
    }
  }
  return status;
}
/******************************************************************************
 * Function         phNxpNciHalRFConfigCmdRecSequence
 *
 * Description      This function is called to handle dummy FW recovery sequence
 *                  Whenever RF settings are failed to apply with invalid param
 *                  response, recovery mechanism includes dummy firmware
 *download
 *                  followed by firmware download and then config settings. The
 *dummy
 *                  firmware changes the major number of the firmware inside
 *NFCC.
 *                  Then actual firmware dowenload will be successful. This can
 *be
 *                  retried maximum three times.
 *
 * Returns          Always returns NFCSTATUS_SUCCESS
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHalRFConfigCmdRecSequence() {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  uint16_t recFWState = 1;
  if(nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD == true) {
      gRecFWDwnld = false;
      force_fw_download_req = true;
  } else {
      gRecFWDwnld = true;
  }
  gRecFwRetryCount++;
  if (gRecFwRetryCount > 0x03) {
    NXPLOG_NCIHAL_D("Max retry count for RF config FW recovery exceeded ");
    gRecFWDwnld = false;
    if(nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD == true) {
        force_fw_download_req = false;
    }
    return NFCSTATUS_FAILED;
  }
  do {
    phDnldNfc_InitImgInfo();
    if (NFCSTATUS_SUCCESS == phNxpNciHal_CheckValidFwVersion()) {
      status = phNxpNciHal_fw_download();
      if (status == NFCSTATUS_FAILED) {
        break;
      }
    }
    gRecFWDwnld = false;
  } while (recFWState--);
  gRecFWDwnld = false;
  if(nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD == true) {
      force_fw_download_req = false;
  }
  return status;
}
#endif
/******************************************************************************
 * Function         phNxpNciHal_core_initialized_complete
 *
 * Description      This function is called when phNxpNciHal_core_initialized
 *                  complete all proprietary command exchanges. This function
 *                  informs libnfc-nci about completion of core initialize
 *                  and result of that through callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_core_initialized_complete(NFCSTATUS status) {
  static phLibNfc_Message_t msg;

  if (status == NFCSTATUS_SUCCESS) {
    msg.eMsgType = NCI_HAL_POST_INIT_CPLT_MSG;
  } else {
    msg.eMsgType = NCI_HAL_ERROR_MSG;
  }
  msg.pMsgData = NULL;
  msg.Size = 0;

  phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId,
                        (phLibNfc_Message_t*)&msg);

  return;
}

/******************************************************************************
 * Function         phNxpNciHal_pre_discover
 *
 * Description      This function is called by libnfc-nci to perform any
 *                  proprietary exchange before RF discovery.
 *
 * Returns          It always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_pre_discover(void) {
  /* Nothing to do here for initial version */
  return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_release_info
 *
 * Description      This function frees allocated memory for mGetCfg_info
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_release_info(void) {
  NXPLOG_NCIHAL_D("phNxpNciHal_release_info mGetCfg_info");
  if (mGetCfg_info != NULL) {
    free(mGetCfg_info);
    mGetCfg_info = NULL;
  }
}

/******************************************************************************
 * Function         phNxpNciHal_close
 *
 * Description      This function close the NFCC interface and free all
 *                  resources.This is called by libnfc-nci on NFC service stop.
 *
 * Returns          Always return NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_close(bool bShutdown) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  uint8_t cmd_ce_discovery_nci[10] = {
      0x21, 0x03,
  };
  static uint8_t cmd_core_reset_nci[] = {0x20, 0x00, 0x01, 0x00};
  static uint8_t cmd_ven_disable_nci[] = {0x20, 0x02, 0x05, 0x01,
                                         0xA0, 0x07, 0x01, 0x02};
  uint8_t length = 0;
  uint8_t numPrms = 0;
  uint8_t ptr = 4;
  unsigned long uiccListenMask = 0x00;
  unsigned long eseListenMask = 0x00;

  if (nxpncihal_ctrl.halStatus == HAL_STATUS_CLOSE) {
    NXPLOG_NCIHAL_E("phNxpNciHal_close is already closed, ignoring close");
    return NFCSTATUS_FAILED;
  }
  if (!(GetNxpNumValue(NAME_NXP_UICC_LISTEN_TECH_MASK, &uiccListenMask,
                       sizeof(uiccListenMask)))) {
    uiccListenMask = 0x07;
    NXPLOG_NCIHAL_D("UICC_LISTEN_TECH_MASK = 0x%0lX", uiccListenMask);
  }

  if (!(GetNxpNumValue(NAME_NXP_ESE_LISTEN_TECH_MASK, &eseListenMask,
                      sizeof(eseListenMask)))) {
    eseListenMask = 0x07;
    NXPLOG_NCIHAL_D ("NXP_ESE_LISTEN_TECH_MASK = 0x%0lX", eseListenMask);
  }
    /* Avoiding sending flush RAM to flash during NFC close.
       This is called during recovery sequence also.
       To be taken up after all discussion.
     */
#if 0
  if(nfcFL.chipType == sn100u)
      status = phNxpNciHal_ext_send_sram_config_to_flash();
#endif
  CONCURRENCY_LOCK();
  if(nfcFL.chipType != sn100u) {
    if(!bShutdown){
      status = phNxpNciHal_send_ext_cmd(sizeof(cmd_ven_disable_nci), cmd_ven_disable_nci);
      if(status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("CMD_VEN_DISABLE_NCI: Failed");
      }
    }
  }
  if (nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT &&
          read_failed_disable_nfc) {
      read_failed_disable_nfc = false;
      goto close_and_return;
  }

  if (write_unlocked_status == NFCSTATUS_FAILED) {
    NXPLOG_NCIHAL_D("phNxpNciHal_close i2c write failed .Clean and Return");
    goto close_and_return;
  }

  if((uiccListenMask & 0x1) == 0x01 || (eseListenMask & 0x1) == 0x01) {
    NXPLOG_NCIHAL_D("phNxpNciHal_close (): Adding A passive listen");
    numPrms++;
    cmd_ce_discovery_nci[ptr++] = 0x80;
    cmd_ce_discovery_nci[ptr++] = 0x01;
    length += 2;
  }
  if((uiccListenMask & 0x2) == 0x02 || (eseListenMask & 0x4) == 0x02) {
    NXPLOG_NCIHAL_D("phNxpNciHal_close (): Adding B passive listen");
    numPrms++;
    cmd_ce_discovery_nci[ptr++] = 0x81;
    cmd_ce_discovery_nci[ptr++] = 0x01;
    length += 2;
  }
  if((uiccListenMask & 0x4) == 0x04 || (eseListenMask & 0x4) == 0x04) {
    NXPLOG_NCIHAL_D("phNxpNciHal_close (): Adding F passive listen");
    numPrms++;
    cmd_ce_discovery_nci[ptr++] = 0x82;
    cmd_ce_discovery_nci[ptr++] = 0x01;
    length += 2;
  }

  if (length != 0) {
    cmd_ce_discovery_nci[2] = length + 1;
    cmd_ce_discovery_nci[3] = numPrms;
    status = phNxpNciHal_send_ext_cmd(length + 4, cmd_ce_discovery_nci);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("CMD_CE_DISC_NCI: Failed");
    }
  } else {
    NXPLOG_NCIHAL_E(
        "No changes in the discovery command, sticking to last discovery "
        "command sent");
  }

  nxpncihal_ctrl.halStatus = HAL_STATUS_CLOSE;

  status =
      phNxpNciHal_send_ext_cmd(sizeof(cmd_core_reset_nci), cmd_core_reset_nci);

  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("NCI_CORE_RESET: Failed");
  }

close_and_return:
  if (NULL != gpphTmlNfc_Context->pDevHandle) {
    phNxpNciHal_close_complete(NFCSTATUS_SUCCESS);
    /* Abort any pending read and write */
    status = phTmlNfc_ReadAbort();
    status = phTmlNfc_WriteAbort();

    phOsalNfc_Timer_Cleanup();

    status = phTmlNfc_Shutdown();

    phDal4Nfc_msgrelease(nxpncihal_ctrl.gDrvCfg.nClientId);

    memset(&nxpncihal_ctrl, 0x00, sizeof(nxpncihal_ctrl));

    NXPLOG_NCIHAL_D("phNxpNciHal_close - phOsalNfc_DeInit completed");
  }

  CONCURRENCY_UNLOCK();

  phNxpNciHal_cleanup_monitor();
  write_unlocked_status = NFCSTATUS_SUCCESS;
  phNxpNciHal_release_info();
  /* Return success always */
  return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_close_complete
 *
 * Description      This function inform libnfc-nci about result of
 *                  phNxpNciHal_close.
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_close_complete(NFCSTATUS status) {
  static phLibNfc_Message_t msg;

  if (status == NFCSTATUS_SUCCESS) {
    msg.eMsgType = NCI_HAL_CLOSE_CPLT_MSG;
  } else {
    msg.eMsgType = NCI_HAL_ERROR_MSG;
  }
  msg.pMsgData = NULL;
  msg.Size = 0;

  phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId, &msg);

  return;
}

/******************************************************************************
 * Function         phNxpNciHal_configDiscShutdown
 *
 * Description      Enable the CE and VEN config during shutdown.
 *
 * Returns          Always return NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_configDiscShutdown(void) {
  NFCSTATUS status;
  /*NCI_RESET_CMD*/
  static uint8_t cmd_reset_nci[] = {0x20, 0x00, 0x01, 0x00};

  static uint8_t cmd_disable_disc[] = {0x21, 0x06, 0x01, 0x00};

  static uint8_t cmd_ce_disc_nci[] = {0x21, 0x03, 0x07, 0x03, 0x80,
                                      0x01, 0x81, 0x01, 0x82, 0x01};

  static uint8_t cmd_ven_pulld_enable_nci[] = {0x20, 0x02, 0x05, 0x01,
                                         0xA0, 0x07, 0x01, 0x03};

  CONCURRENCY_LOCK();

  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_disable_disc), cmd_disable_disc);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("CMD_DISABLE_DISCOVERY: Failed");
  }

  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_ven_pulld_enable_nci), cmd_ven_pulld_enable_nci);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("CMD_VEN_PULLD_ENABLE_NCI: Failed");
  }

  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_ce_disc_nci), cmd_ce_disc_nci);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("CMD_CE_DISC_NCI: Failed");
  }

  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci), cmd_reset_nci);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("NCI_CORE_RESET: Failed");
  }
  CONCURRENCY_UNLOCK();

  status = phNxpNciHal_close(true);
  if(status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("NCI_HAL_CLOSE: Failed");
  }

  /* Return success always */
  return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_getVendorConfig
 *
 * Description      This function can be used by HAL to inform
 *                 to update vendor configuration parametres
 *
 * Returns          void.
 *
 ******************************************************************************/

void phNxpNciHal_getVendorConfig(NfcConfig& config) {
  unsigned long num = 0;
  std::array<uint8_t, NXP_MAX_CONFIG_STRING_LEN> buffer;
  buffer.fill(0);
  long retlen = 0;
  memset(&config, 0x00, sizeof(NfcConfig));

  if (GetNxpNumValue(NAME_ISO_DEP_MAX_TRANSCEIVE, &num, sizeof(num))) {
    config.maxIsoDepTransceiveLength = num;
  }
  if (GetNxpNumValue(NAME_NFA_POLL_BAIL_OUT_MODE, &num, sizeof(num))
       && (num == 1)) {
    config.nfaPollBailOutMode = true;
  }
  if (GetNxpNumValue(NAME_ACTIVE_SE, &num, sizeof(num))) {
    config.defaultOffHostRoute = num;
  }
  if (GetNxpNumValue(NAME_ACTIVE_SE_NFCF, &num, sizeof(num))) {
    config.defaultOffHostRouteFelica = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_FELICA_SYS_CODE_ROUTE, &num, sizeof(num))) {
    config.defaultSystemCodeRoute = num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_ISODEP_ROUTE, &num, sizeof(num))) {
    config.defaultRoute = num;
  }
  if (GetNxpByteArrayValue(NAME_DEVICE_HOST_WHITE_LIST, (char*)buffer.data(), buffer.size(), &retlen)) {
    config.hostWhitelist.setToExternal(buffer.data(),retlen);
  }
  if (GetNxpNumValue(NAME_NFA_HCI_STATIC_PIPE_ID_ESE, &num, sizeof(num))) {
    config.offHostESEPipeId = num;
  }
  if (GetNxpNumValue(NAME_NFA_HCI_STATIC_PIPE_ID_SIM, &num, sizeof(num))) {
    config.offHostSIMPipeId = num;
  }
  if ((GetNxpByteArrayValue(NAME_NFA_PROPRIETARY_CFG, (char*)buffer.data(), buffer.size(), &retlen))
         && (retlen == 9)) {
    config.nfaProprietaryCfg.protocol18092Active = (uint8_t) buffer[0];
    config.nfaProprietaryCfg.protocolBPrime = (uint8_t) buffer[1];
    config.nfaProprietaryCfg.protocolDual = (uint8_t) buffer[2];
    config.nfaProprietaryCfg.protocol15693 = (uint8_t) buffer[3];
    config.nfaProprietaryCfg.protocolKovio = (uint8_t) buffer[4];
    config.nfaProprietaryCfg.protocolMifare = (uint8_t) buffer[5];
    config.nfaProprietaryCfg.discoveryPollKovio = (uint8_t) buffer[6];
    config.nfaProprietaryCfg.discoveryPollBPrime = (uint8_t) buffer[7];
    config.nfaProprietaryCfg.discoveryListenBPrime = (uint8_t) buffer[8];
  } else {
    memset(&config.nfaProprietaryCfg, 0xFF, sizeof(ProtocolDiscoveryConfig));
  }
  if ((GetNxpNumValue(NAME_PRESENCE_CHECK_ALGORITHM, &num, sizeof(num))) && (num <= 2) ) {
      config.presenceCheckAlgorithm = (PresenceCheckAlgorithm)num;
  }
}

/******************************************************************************
 * Function         phNxpNciHal_notify_i2c_fragmentation
 *
 * Description      This function can be used by HAL to inform
 *                 libnfc-nci that i2c fragmentation is enabled/disabled
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_notify_i2c_fragmentation(void) {
  if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
    /*inform libnfc-nci that i2c fragmentation is enabled/disabled */
    (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_ENABLE_I2C_FRAGMENTATION_EVT,
                                        HAL_NFC_STATUS_OK);
  }
}
/******************************************************************************
 * Function         phNxpNciHal_control_granted
 *
 * Description      Called by libnfc-nci when NFCC control is granted to HAL.
 *
 * Returns          Always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_control_granted(void) {
  /* Take the concurrency lock so no other calls from upper layer
   * will be allowed
   */
  CONCURRENCY_LOCK();

  if (NULL != nxpncihal_ctrl.p_control_granted_cback) {
    (*nxpncihal_ctrl.p_control_granted_cback)();
  }
  /* At the end concurrency unlock so calls from upper layer will
   * be allowed
   */
  CONCURRENCY_UNLOCK();
  return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_request_control
 *
 * Description      This function can be used by HAL to request control of
 *                  NFCC to libnfc-nci. When control is provided to HAL it is
 *                  notified through phNxpNciHal_control_granted.
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_request_control(void) {
  if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
    /* Request Control of NCI Controller from NCI NFC Stack */
    (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_REQUEST_CONTROL_EVT,
                                        HAL_NFC_STATUS_OK);
  }

  return;
}

/******************************************************************************
 * Function         phNxpNciHal_release_control
 *
 * Description      This function can be used by HAL to release the control of
 *                  NFCC back to libnfc-nci.
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_release_control(void) {
  if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
    /* Release Control of NCI Controller to NCI NFC Stack */
    (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_RELEASE_CONTROL_EVT,
                                        HAL_NFC_STATUS_OK);
  }

  return;
}

/******************************************************************************
 * Function         phNxpNciHal_power_cycle
 *
 * Description      This function is called by libnfc-nci when power cycling is
 *                  performed. When processing is complete it is notified to
 *                  libnfc-nci through phNxpNciHal_power_cycle_complete.
 *
 * Returns          Always return NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_power_cycle(void) {
  NXPLOG_NCIHAL_D("Power Cycle");
  NFCSTATUS status = NFCSTATUS_FAILED;
  if (nxpncihal_ctrl.halStatus != HAL_STATUS_OPEN) {
    NXPLOG_NCIHAL_D("Power Cycle failed due to hal status not open");
    return NFCSTATUS_FAILED;
  }
  status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);

  if (NFCSTATUS_SUCCESS == status) {
    NXPLOG_NCIHAL_D("PN54X Reset - SUCCESS\n");
  } else {
    NXPLOG_NCIHAL_D("PN54X Reset - FAILED\n");
  }

  phNxpNciHal_power_cycle_complete(NFCSTATUS_SUCCESS);
  return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_power_cycle_complete
 *
 * Description      This function is called to provide the status of
 *                  phNxpNciHal_power_cycle to libnfc-nci through callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_power_cycle_complete(NFCSTATUS status) {
  static phLibNfc_Message_t msg;

  if (status == NFCSTATUS_SUCCESS) {
    msg.eMsgType = NCI_HAL_OPEN_CPLT_MSG;
  } else {
    msg.eMsgType = NCI_HAL_ERROR_MSG;
  }
  msg.pMsgData = NULL;
  msg.Size = 0;

  phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId, &msg);

  return;
}

/******************************************************************************
 * Function         phNxpNciHal_ioctl
 *
 * Description      This function is called by jni when wired mode is
 *                  performed.First Pn54x driver will give the access
 *                  permission whether wired mode is allowed or not
 *                  arg (0):
 * Returns          return 0 on success and -1 on fail, On success
 *                  update the acutual state of operation in arg pointer
 *
 ******************************************************************************/
int phNxpNciHal_ioctl(long arg, void* p_data) {
  NXPLOG_NCIHAL_D("%s : enter - arg = %ld", __func__, arg);
  nfc_nci_IoctlInOutData_t* pInpOutData = (nfc_nci_IoctlInOutData_t*)p_data;
  int ret = -1;
  NFCSTATUS status = NFCSTATUS_FAILED;
  phNxpNciHal_FwRfupdateInfo_t* FwRfInfo;
  NFCSTATUS fm_mw_ver_check = NFCSTATUS_FAILED;

  switch (arg) {
    case HAL_NFC_IOCTL_GET_CONFIG_INFO:
      if (mGetCfg_info) {
        memcpy(pInpOutData->out.data.nxpNciAtrInfo, mGetCfg_info,
               sizeof(phNxpNci_getCfg_info_t));
      } else {
        NXPLOG_NCIHAL_E("%s : Error! mgetCfg_info is Empty ", __func__);
      }
      ret = 0;
      break;
    case HAL_NFC_IOCTL_CHECK_FLASH_REQ:
      FwRfInfo =
          (phNxpNciHal_FwRfupdateInfo_t*)&pInpOutData->out.data.fwUpdateInf;
      status = phNxpNciHal_CheckFwRegFlashRequired(&FwRfInfo->fw_update_reqd,
                                                   &FwRfInfo->rf_update_reqd);
      if (NFCSTATUS_SUCCESS == status) {
#ifndef FW_DWNLD_FLAG
        fw_dwnld_flag = FwRfInfo->fw_update_reqd;
#endif
        ret = 0;
      }
      break;
    case HAL_NFC_IOCTL_FW_DWNLD:
      status = phNxpNciHal_FwDwnld(*(uint16_t*)p_data);
      pInpOutData->out.data.fwDwnldStatus = (uint16_t)status;
      if (NFCSTATUS_SUCCESS == status) {
        ret = 0;
        fw_dwnld_flag = true;
      }
      if(nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD) {
          force_fw_download_req = false;
      }
      break;
    case HAL_NFC_IOCTL_FW_MW_VER_CHECK:
      fm_mw_ver_check = phNxpNciHal_fw_mw_ver_check();
      pInpOutData->out.data.fwMwVerStatus = fm_mw_ver_check;
      ret = 0;
      break;
    case HAL_NFC_IOCTL_NCI_TRANSCEIVE:
      if (p_data == NULL) {
        ret = -1;
        break;
      }

      if (pInpOutData->inp.data.nciCmd.cmd_len <= 0) {
        ret = -1;
        break;
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
      break;
    case HAL_NFC_IOCTL_SEND_FLASH_UPDATE:
    {
        uint8_t flash_update_done = TRUE;
        phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
        mEEPROM_info.buffer = &flash_update_done;
        mEEPROM_info.bufflen = sizeof(flash_update_done);
        mEEPROM_info.request_type = EEPROM_FLASH_UPDATE;
        mEEPROM_info.request_mode = SET_EEPROM_DATA;
        request_EEPROM(&mEEPROM_info);
        break;
    }
    case HAL_NFC_IOCTL_GET_FEATURE_LIST:
        pInpOutData->out.data.chipType = (uint8_t)phNxpNciHal_getChipType();
        ret = 0;
        break;
    default:
      NXPLOG_NCIHAL_E("%s : Wrong arg = %ld", __func__, arg);
      break;
  }
  NXPLOG_NCIHAL_D("%s : exit - ret = %d", __func__, ret);
  return ret;
}

/******************************************************************************
 * Function         phNxpNciHal_nfccClockCfgRead
 *
 * Description      This function is called for loading a data strcuture from
 *                  the config file with clock source and clock frequency values
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_nfccClockCfgRead(void)
{
    unsigned long num = 0;
    int isfound = 0;

    nxpprofile_ctrl.bClkSrcVal = 0;
    nxpprofile_ctrl.bClkFreqVal = 0;
    nxpprofile_ctrl.bTimeout = 0;

    isfound = GetNxpNumValue(NAME_NXP_SYS_CLK_SRC_SEL, &num, sizeof(num));
    if (isfound > 0)
    {
        nxpprofile_ctrl.bClkSrcVal = num;
    }

    num = 0;
    isfound = 0;
    isfound = GetNxpNumValue(NAME_NXP_SYS_CLK_FREQ_SEL, &num, sizeof(num));
    if (isfound > 0)
    {
        nxpprofile_ctrl.bClkFreqVal = num;
    }

    num = 0;
    isfound = 0;
    isfound = GetNxpNumValue(NAME_NXP_SYS_CLOCK_TO_CFG, &num, sizeof(num));
    if (isfound > 0)
    {
        nxpprofile_ctrl.bTimeout = num;
    }

    NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bClkSrcVal = 0x%x", nxpprofile_ctrl.bClkSrcVal);
    NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bClkFreqVal = 0x%x", nxpprofile_ctrl.bClkFreqVal);
    NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bClkFreqVal = 0x%x", nxpprofile_ctrl.bTimeout);

    if ((nxpprofile_ctrl.bClkSrcVal < CLK_SRC_XTAL) ||
            (nxpprofile_ctrl.bClkSrcVal > CLK_SRC_PLL))
    {
        NXPLOG_FWDNLD_E("Clock source value is wrong in config file, setting it as default");
        nxpprofile_ctrl.bClkSrcVal = NXP_SYS_CLK_SRC_SEL;
    }
    if ((nxpprofile_ctrl.bClkFreqVal < CLK_FREQ_13MHZ) ||
            (nxpprofile_ctrl.bClkFreqVal > CLK_FREQ_52MHZ))
    {
        NXPLOG_FWDNLD_E("Clock frequency value is wrong in config file, setting it as default");
        nxpprofile_ctrl.bClkFreqVal = NXP_SYS_CLK_FREQ_SEL;
    }
    if ((nxpprofile_ctrl.bTimeout < CLK_TO_CFG_DEF) || (nxpprofile_ctrl.bTimeout > CLK_TO_CFG_MAX))
    {
        NXPLOG_FWDNLD_E("Clock timeout value is wrong in config file, setting it as default");
        nxpprofile_ctrl.bTimeout = CLK_TO_CFG_DEF;
    }
}

/******************************************************************************
 * Function         phNxpNciHal_determineConfiguredClockSrc
 *
 * Description      This function determines and encodes clock source based on
 *                  clock frequency
 *
 * Returns          encoded form of clock source
 *
 *****************************************************************************/
int   phNxpNciHal_determineConfiguredClockSrc()
{
    //NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t param_clock_src = CLK_SRC_PLL;
    if (nxpprofile_ctrl.bClkSrcVal == CLK_SRC_PLL)
    {

    if(nfcFL.chipType == pn553) {
        param_clock_src = param_clock_src << 3;
    }
    else if(nfcFL.chipType == sn100u)
    {
        param_clock_src = 0;
    }

        if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_13MHZ)
        {
            param_clock_src |= 0x00;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_19_2MHZ)
        {
            param_clock_src |= 0x01;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_24MHZ)
        {
            param_clock_src |= 0x02;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_26MHZ)
        {
            param_clock_src |= 0x03;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_38_4MHZ)
        {
            param_clock_src |= 0x04;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_52MHZ)
        {
            param_clock_src |= 0x05;
        }
        else
        {
            NXPLOG_NCIHAL_E("Wrong clock freq, send default PLL@19.2MHz");
            if(nfcFL.chipType != sn100u)
                param_clock_src = 0x11;
            else
                param_clock_src = 0x01;
        }
    }
    else if(nxpprofile_ctrl.bClkSrcVal == CLK_SRC_XTAL)
    {
        param_clock_src = 0x08;

    }
    else
    {
        NXPLOG_NCIHAL_E("Wrong clock source. Dont apply any modification");
    }
    return param_clock_src;
  }

/******************************************************************************
 * Function         phNxpNciHal_determineConfiguredClockSrc
 *
 * Description      This function determines and encodes clock source based on
 *                  clock frequency
 *
 * Returns          encoded form of clock source
 *
 *****************************************************************************/
int phNxpNciHal_determineClockDelayRequest(uint8_t nfcc_cfg_clock_src)
{
    unsigned long num = 0;
    int isfound = 0;
    uint8_t nfcc_clock_delay_req = 0;
    uint8_t nfcc_clock_set_needed = false;

    isfound = GetNxpNumValue(NAME_NXP_CLOCK_REQ_DELAY, &num, sizeof(num));
    if (isfound > 0)
    {
        nxpprofile_ctrl.clkReqDelay = num;
    }
    if ((nxpprofile_ctrl.clkReqDelay < CLK_REQ_DELAY_MIN) || (nxpprofile_ctrl.clkReqDelay > CLK_REQ_DELAY_MAX))
    {
        NXPLOG_FWDNLD_E("default delay to start clock value is wrong in config file, setting it as default");
        nxpprofile_ctrl.clkReqDelay = CLK_REQ_DELAY_DEF;
        return nfcc_clock_set_needed;
    }
    nfcc_clock_delay_req = nxpprofile_ctrl.clkReqDelay;

    /*Check if the clock source is XTAL as per config*/
    if(nfcc_cfg_clock_src == CLK_CFG_XTAL)
    {
        if(nfcc_clock_delay_req != (phNxpNciClock.p_rx_data[CLK_REQ_DELAY_XTAL_OFFSET] & CLK_REQ_DELAY_MASK))
        {
            nfcc_clock_set_needed = true;
            phNxpNciClock.p_rx_data[CLK_REQ_DELAY_XTAL_OFFSET] &= ~(CLK_REQ_DELAY_MASK);
            phNxpNciClock.p_rx_data[CLK_REQ_DELAY_XTAL_OFFSET] |= (nfcc_clock_delay_req & CLK_REQ_DELAY_MASK);
        }
    }
    /*Check if the clock source is PLL as per config*/
    else if(nfcc_cfg_clock_src >= 0 && nfcc_cfg_clock_src < 6)
    {
        if(nfcc_clock_delay_req != (phNxpNciClock.p_rx_data[CLK_REQ_DELAY_PLL_OFFSET] & CLK_REQ_DELAY_MASK))
        {
            nfcc_clock_set_needed = true;
            phNxpNciClock.p_rx_data[CLK_REQ_DELAY_PLL_OFFSET] &= ~(CLK_REQ_DELAY_MASK);
            phNxpNciClock.p_rx_data[CLK_REQ_DELAY_PLL_OFFSET] |= (nfcc_clock_delay_req & CLK_REQ_DELAY_MASK);
        }
    }
    return nfcc_clock_set_needed;
}

/******************************************************************************
 * Function         phNxpNciHal_nfccClockCfgApply
 *
 * Description      This function is called after successfull download
 *                  to check if clock settings in config file and chip
 *                  is same
 *
 * Returns          void.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_nfccClockCfgApply(void) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  uint8_t nfcc_cfg_clock_src, nfcc_cur_clock_src;
  uint8_t nfcc_clock_set_needed;
  uint8_t nfcc_clock_delay_req;
  static uint8_t* get_clock_cmd;
  uint8_t get_clck_cmd[] = {0x20, 0x03, 0x07, 0x03, 0xA0,
                                    0x02, 0xA0, 0x03, 0xA0, 0x04};
  uint8_t get_clck_cmd_sn100[] = {0x20, 0x03, 0x03, 0x01, 0xA0, 0x11};
  uint8_t set_clck_cmd[] = {0x20, 0x02, 0x0B, 0x01, 0xA0, 0x11,0x07,\
      0x01, 0x0A, 0x32, 0x02, 0x01, 0xF6, 0xF6};
  uint8_t get_clk_size = 0;

  if(nfcFL.chipType != sn100u)
  {
     get_clock_cmd = get_clck_cmd;
     get_clk_size = sizeof(get_clck_cmd);
  }
  else
  {
     get_clock_cmd = get_clck_cmd_sn100;
     get_clk_size = sizeof(get_clck_cmd_sn100);
  }
  phNxpNciHal_nfccClockCfgRead();
  phNxpNciClock.isClockSet = true;
  status = phNxpNciHal_send_ext_cmd(get_clk_size, get_clock_cmd);
  phNxpNciClock.isClockSet = false;

  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("unable to retrieve get_clk_src_sel");
    return status;
  }

  nfcc_cfg_clock_src = phNxpNciHal_determineConfiguredClockSrc();
  if(nfcFL.chipType != sn100u)
  {
      nfcc_cur_clock_src = phNxpNciClock.p_rx_data[12];
  }else
  {
      nfcc_cur_clock_src = phNxpNciClock.p_rx_data[8];
  }

  if(nfcFL.chipType != sn100u)
  {
      nfcc_clock_set_needed = (nfcc_cfg_clock_src != nfcc_cur_clock_src ||
                                  phNxpNciClock.p_rx_data[16] == nxpprofile_ctrl.bTimeout) ?\
                                  true : false;
  }
  else
  {
      nfcc_clock_delay_req = phNxpNciHal_determineClockDelayRequest(nfcc_cfg_clock_src);
      /**Determine clock src is as expected*/
      nfcc_clock_set_needed = ((nfcc_cfg_clock_src != nfcc_cur_clock_src || nfcc_clock_delay_req )? true : false);
  }

  if(nfcc_clock_set_needed) {
    NXPLOG_NCIHAL_D ("Setting Clock Source and Frequency");
    {
        /*Read the preset value from FW*/
        memcpy(&set_clck_cmd[7], &phNxpNciClock.p_rx_data[8], phNxpNciClock.p_rx_data[7]);
        /*Update clock source and frequency as per DH configuration*/
        set_clck_cmd[7] = nfcc_cfg_clock_src;
        status = phNxpNciHal_send_ext_cmd(sizeof(set_clck_cmd), set_clck_cmd);
    }
  }

  return status;
}
#if 0
/******************************************************************************
 * Function         phNxpNciHal_get_mw_eeprom
 *
 * Description      This function is called to retreive data in mw eeprom area
 *
 * Returns          NFCSTATUS.
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_get_mw_eeprom(void) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  uint8_t retry_cnt = 0;
  static uint8_t get_mw_eeprom_cmd[] = {0x20, 0x03, 0x03, 0x01, 0xA0, 0x0F};

retry_send_ext:
  if (retry_cnt > 3) {
    return NFCSTATUS_FAILED;
  }

  phNxpNciMwEepromArea.isGetEepromArea = true;
  status =
      phNxpNciHal_send_ext_cmd(sizeof(get_mw_eeprom_cmd), get_mw_eeprom_cmd);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("unable to get the mw eeprom data");
    phNxpNciMwEepromArea.isGetEepromArea = false;
    retry_cnt++;
    goto retry_send_ext;
  }
  phNxpNciMwEepromArea.isGetEepromArea = false;

  if (phNxpNciMwEepromArea.p_rx_data[12]) {
    fw_download_success = 1;
  }
  return status;
}

/******************************************************************************
 * Function         phNxpNciHal_set_mw_eeprom
 *
 * Description      This function is called to update data in mw eeprom area
 *
 * Returns          void.
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_set_mw_eeprom(void) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  uint8_t retry_cnt = 0;
  uint8_t set_mw_eeprom_cmd[39] = {0};
  uint8_t cmd_header[] = {0x20, 0x02, 0x24, 0x01, 0xA0, 0x0F, 0x20};

  memcpy(set_mw_eeprom_cmd, cmd_header, sizeof(cmd_header));
  phNxpNciMwEepromArea.p_rx_data[12] = 0;
  memcpy(set_mw_eeprom_cmd + sizeof(cmd_header), phNxpNciMwEepromArea.p_rx_data,
         sizeof(phNxpNciMwEepromArea.p_rx_data));

retry_send_ext:
  if (retry_cnt > 3) {
    return NFCSTATUS_FAILED;
  }

  status =
      phNxpNciHal_send_ext_cmd(sizeof(set_mw_eeprom_cmd), set_mw_eeprom_cmd);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("unable to update the mw eeprom data");
    retry_cnt++;
    goto retry_send_ext;
  }
  return status;
}

/******************************************************************************
 * Function         phNxpNciHal_set_clock
 *
 * Description      This function is called after successfull download
 *                  to apply the clock setting provided in config file
 *
 * Returns          void.
 *
 *****************************************************************************/
static void phNxpNciHal_set_clock(void) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  int retryCount = 0;

retrySetclock:
  phNxpNciClock.isClockSet = true;
  if (nxpprofile_ctrl.bClkSrcVal == CLK_SRC_PLL) {
    static uint8_t set_clock_cmd[] = {0x20, 0x02, 0x09, 0x02, 0xA0, 0x03,
                                      0x01, 0x11, 0xA0, 0x04, 0x01, 0x01};
#if (NFC_NXP_CHIP_TYPE == PN553)
    uint8_t param_clock_src = 0x00;
#else
    uint8_t param_clock_src = CLK_SRC_PLL;
    param_clock_src = param_clock_src << 3;
#endif

    if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_13MHZ) {
      param_clock_src |= 0x00;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_19_2MHZ) {
      param_clock_src |= 0x01;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_24MHZ) {
      param_clock_src |= 0x02;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_26MHZ) {
      param_clock_src |= 0x03;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_38_4MHZ) {
      param_clock_src |= 0x04;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_52MHZ) {
      param_clock_src |= 0x05;
    } else {
      NXPLOG_NCIHAL_E("Wrong clock freq, send default PLL@19.2MHz");
#if (NFC_NXP_CHIP_TYPE == PN553)
      param_clock_src = 0x01;
#else
      param_clock_src = 0x11;
#endif
    }

    set_clock_cmd[7] = param_clock_src;
    set_clock_cmd[11] = nxpprofile_ctrl.bTimeout;
    status = phNxpNciHal_send_ext_cmd(sizeof(set_clock_cmd), set_clock_cmd);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("PLL colck setting failed !!");
    }
  } else if (nxpprofile_ctrl.bClkSrcVal == CLK_SRC_XTAL) {
    static uint8_t set_clock_cmd[] = {0x20, 0x02, 0x05, 0x01,
                                      0xA0, 0x03, 0x01, 0x08};
    status = phNxpNciHal_send_ext_cmd(sizeof(set_clock_cmd), set_clock_cmd);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("XTAL colck setting failed !!");
    }
  } else {
    NXPLOG_NCIHAL_E("Wrong clock source. Dont apply any modification")
  }

  // Checking for SET CONFG SUCCESS, re-send the command  if not.
  phNxpNciClock.isClockSet = false;
  if (phNxpNciClock.p_rx_data[3] != NFCSTATUS_SUCCESS) {
    if (retryCount++ < 3) {
      NXPLOG_NCIHAL_E("Set-clk failed retry again ");
      goto retrySetclock;
    } else {
      NXPLOG_NCIHAL_D("Set clk  failed -  max count = 0x%x exceeded ",
                      retryCount);
      //            NXPLOG_NCIHAL_E("Set Config is failed for Clock Due to
      //            elctrical disturbances, aborting the NFC process");
      //            abort ();
    }
  }
}
#endif
/******************************************************************************
 * Function         phNxpNciHal_check_clock_config
 *
 * Description      This function is called after successfull download
 *                  to check if clock settings in config file and chip
 *                  is same
 *
 * Returns          void.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_check_clock_config(void) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  uint8_t param_clock_src;
  static uint8_t get_clock_cmd[] = {0x20, 0x03, 0x07, 0x03, 0xA0,
                                    0x02, 0xA0, 0x03, 0xA0, 0x04};
  phNxpNciClock.isClockSet = true;
  phNxpNciHal_get_clk_freq();
  status = phNxpNciHal_send_ext_cmd(sizeof(get_clock_cmd), get_clock_cmd);

  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("unable to retrieve get_clk_src_sel");
    return status;
  }
  param_clock_src = check_config_parameter();
  if (phNxpNciClock.p_rx_data[12] == param_clock_src &&
      phNxpNciClock.p_rx_data[16] == nxpprofile_ctrl.bTimeout) {
    phNxpNciClock.issetConfig = false;
  } else {
    phNxpNciClock.issetConfig = true;
  }
  phNxpNciClock.isClockSet = false;

  return status;
}

/******************************************************************************
 * Function         phNxpNciHal_china_tianjin_rf_setting
 *
 * Description      This function is called to check RF Setting
 *
 * Returns          Status.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_china_tianjin_rf_setting(void) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  int isfound = 0;
  int rf_enable = false;
  int rf_val = 0;
  int send_flag;
  uint8_t retry_cnt = 0;
  int enable_bit = 0;
  static uint8_t get_rf_cmd[] = {0x20, 0x03, 0x03, 0x01, 0xA0, 0x85};

retry_send_ext:
  if (retry_cnt > 3) {
    return NFCSTATUS_FAILED;
  }
  send_flag = true;
  phNxpNciRfSet.isGetRfSetting = true;
  status = phNxpNciHal_send_ext_cmd(sizeof(get_rf_cmd), get_rf_cmd);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("unable to get the RF setting");
    phNxpNciRfSet.isGetRfSetting = false;
    retry_cnt++;
    goto retry_send_ext;
  }
  phNxpNciRfSet.isGetRfSetting = false;
  if (phNxpNciRfSet.p_rx_data[3] != 0x00) {
    NXPLOG_NCIHAL_E("GET_CONFIG_RSP is FAILED for CHINA TIANJIN");
    return status;
  }
  rf_val = phNxpNciRfSet.p_rx_data[10];
  isfound = (GetNxpNumValue(NAME_NXP_CHINA_TIANJIN_RF_ENABLED,
                            (void*)&rf_enable, sizeof(rf_enable)));
  if (isfound > 0) {
    enable_bit = rf_val & 0x40;
    if ((enable_bit != 0x40) && (rf_enable == 1)) {
      phNxpNciRfSet.p_rx_data[10] |= 0x40;  // Enable if it is disabled
    } else if ((enable_bit == 0x40) && (rf_enable == 0)) {
      phNxpNciRfSet.p_rx_data[10] &= 0xBF;  // Disable if it is Enabled
    } else {
      send_flag = false;  // No need to change in RF setting
    }

    if (send_flag == true) {
      static uint8_t set_rf_cmd[] = {0x20, 0x02, 0x08, 0x01, 0xA0, 0x85,
                                     0x04, 0x50, 0x08, 0x68, 0x00};
      memcpy(&set_rf_cmd[4], &phNxpNciRfSet.p_rx_data[5], 7);
      status = phNxpNciHal_send_ext_cmd(sizeof(set_rf_cmd), set_rf_cmd);
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("unable to set the RF setting");
        retry_cnt++;
        goto retry_send_ext;
      }
    }
  }

  return status;
}

int check_config_parameter() {
  uint8_t param_clock_src = CLK_SRC_PLL;
  if (nxpprofile_ctrl.bClkSrcVal == CLK_SRC_PLL) {
#if (NFC_NXP_CHIP_TYPE != PN553)
    param_clock_src = param_clock_src << 3;
#endif
    if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_13MHZ) {
      param_clock_src |= 0x00;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_19_2MHZ) {
      param_clock_src |= 0x01;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_24MHZ) {
      param_clock_src |= 0x02;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_26MHZ) {
      param_clock_src |= 0x03;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_38_4MHZ) {
      param_clock_src |= 0x04;
    } else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_52MHZ) {
      param_clock_src |= 0x05;
    } else {
      NXPLOG_NCIHAL_E("Wrong clock freq, send default PLL@19.2MHz");
      param_clock_src = 0x11;
    }
  } else if (nxpprofile_ctrl.bClkSrcVal == CLK_SRC_XTAL) {
    param_clock_src = 0x08;

  } else {
        NXPLOG_NCIHAL_E("Wrong clock source. Dont apply any modification");
  }
  return param_clock_src;
}
/******************************************************************************
 * Function         phNxpNciHal_do_factory_reset
 *
 * Description      This function is called during factory reset to set
 *                  the session id to default value.
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_do_factory_reset(void) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  static uint8_t reset_ese_session_identity_set[] = {
      0x20, 0x02, 0x17, 0x02, 0xA0, 0xEA, 0x08, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xA0, 0xEB, 0x08,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      status = phNxpNciHal_send_ext_cmd(sizeof(reset_ese_session_identity_set),
                                      reset_ese_session_identity_set);
      if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("NXP reset_ese_session_identity_set command failed");
    }
}
/******************************************************************************
 * Function         phNxpNciHal_check_factory_reset
 *
 * Description      This function is called at init time to check
 *                  the presence of ese related info. If file are not
 *                  present set the SWP_INT_SESSION_ID_CFG to FF to
 *                  force the NFCEE to re-run its initialization sequence.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_check_factory_reset(void) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  uint8_t *reset_ese_session_identity_set;
  uint8_t ese_session_dyn_uicc_nv[] = {
            0x20, 0x02, 0x17, 0x02,0xA0, 0xEA, 0x08, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xA0, 0x1E, 0x08, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t ese_session_dyn_uicc[] = {
            0x20, 0x02, 0x22, 0x03, 0xA0, 0xEA, 0x08, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xA0, 0x1E, 0x08, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xA0, 0xEB, 0x08, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t ese_session_nv[] = {
            0x20, 0x02, 0x0C, 0x01, 0xA0, 0xEA, 0x08, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t ese_session[] = {
            0x20, 0x02, 0x17, 0x02, 0xA0, 0xEA, 0x08, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xA0, 0xEB, 0x08,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t len = 0;
  if(nfcFL.nfccFL._NFC_NXP_STAT_DUAL_UICC_WO_EXT_SWITCH || nfcFL.nfccFL._NFCC_DYNAMIC_DUAL_UICC) {
    if(nfcFL.eseFL._EXCLUDE_NV_MEM_DEPENDENCY) {
      reset_ese_session_identity_set = ese_session_dyn_uicc_nv;
      len = sizeof(ese_session_dyn_uicc_nv);
    }
    else {
      reset_ese_session_identity_set = ese_session_dyn_uicc;
      len = sizeof(ese_session_dyn_uicc);
    }
  }
  else {
    if(nfcFL.eseFL._EXCLUDE_NV_MEM_DEPENDENCY) {
      reset_ese_session_identity_set = ese_session_nv;
      len = sizeof(ese_session_nv);
    }
    else {
      reset_ese_session_identity_set = ese_session;
      len = sizeof(ese_session);
    }
  }

    if((nfcFL.nfccFL._NFC_NXP_STAT_DUAL_UICC_WO_EXT_SWITCH) ||
            (nfcFL.nfccFL._NFCC_DYNAMIC_DUAL_UICC)) {
        status = phNxpNciHal_send_ext_cmd(len, reset_ese_session_identity_set);
    }
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("NXP reset_ese_session_identity_set command failed");
    }
}

/******************************************************************************
 * Function         phNxpNciHal_print_res_status
 *
 * Description      This function is called to process the response status
 *                  and print the status byte.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_print_res_status(uint8_t* p_rx_data, uint16_t* p_len) {
  static uint8_t response_buf[][30] = {"STATUS_OK",
                                       "STATUS_REJECTED",
                                       "STATUS_RF_FRAME_CORRUPTED",
                                       "STATUS_FAILED",
                                       "STATUS_NOT_INITIALIZED",
                                       "STATUS_SYNTAX_ERROR",
                                       "STATUS_SEMANTIC_ERROR",
                                       "RFU",
                                       "RFU",
                                       "STATUS_INVALID_PARAM",
                                       "STATUS_MESSAGE_SIZE_EXCEEDED",
                                       "STATUS_UNDEFINED"};
  int status_byte;
  if (p_rx_data[0] == 0x40 && (p_rx_data[1] == 0x02 || p_rx_data[1] == 0x03)) {
    if (p_rx_data[2] && p_rx_data[3] <= 10) {
      status_byte = p_rx_data[CORE_RES_STATUS_BYTE];
      NXPLOG_NCIHAL_D("%s: response status =%s", __func__,
                      response_buf[status_byte]);
    } else {
      NXPLOG_NCIHAL_D("%s: response status =%s", __func__, response_buf[11]);
    }
    if (phNxpNciClock.isClockSet) {
      int i;
      for (i = 0; i < *p_len; i++) {
        phNxpNciClock.p_rx_data[i] = p_rx_data[i];
      }
    }

    else if (phNxpNciRfSet.isGetRfSetting) {
      int i;
      for (i = 0; i < *p_len; i++) {
        phNxpNciRfSet.p_rx_data[i] = p_rx_data[i];
        // NXPLOG_NCIHAL_D("%s: response status =0x%x",__func__,p_rx_data[i]);
      }
    } else if (phNxpNciMwEepromArea.isGetEepromArea) {
      int i;
      for (i = 8; i < *p_len; i++) {
        phNxpNciMwEepromArea.p_rx_data[i - 8] = p_rx_data[i];
      }
    }
  }

  if (p_rx_data[2] && (config_access == true)) {
    if (p_rx_data[3] != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_W("Invalid Data from config file.");
      config_success = false;
    }
  }
}
/*****************************************************************************
 * Function         phNxpNciHal_send_get_cfgs
 *
 * Description      This function is called to  send get configs
 *                  for all the types in get_cfg_arr.
 *                  Response of getConfigs(EEPROM stored) will be
 *                  compared with request coming from MW during discovery.
 *                  If same, then current setConfigs will be dropped
 *
 * Returns          Returns NFCSTATUS_SUCCESS if sending cmd is successful and
 *                  response is received.
 *
 *****************************************************************************/
NFCSTATUS phNxpNciHal_send_get_cfgs() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  NFCSTATUS status = NFCSTATUS_FAILED;
  uint8_t num_cfgs = sizeof(get_cfg_arr) / sizeof(uint8_t);
  uint8_t cfg_count = 0, retry_cnt = 0;
  if (mGetCfg_info != NULL) {
      mGetCfg_info->isGetcfg = true;
  }
  uint8_t cmd_get_cfg[] = {0x20, 0x03, 0x02, 0x01, 0x00};

  while (cfg_count < num_cfgs) {
    cmd_get_cfg[sizeof(cmd_get_cfg) - 1] = get_cfg_arr[cfg_count];

  retry_get_cfg:
    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_get_cfg), cmd_get_cfg);
    if (status != NFCSTATUS_SUCCESS && retry_cnt < 3) {
      NXPLOG_NCIHAL_E("cmd_get_cfg failed");
      retry_cnt++;
      goto retry_get_cfg;
    }
    if (retry_cnt == 3) {
      break;
    }
    cfg_count++;
    retry_cnt = 0;
  }
  mGetCfg_info->isGetcfg = false;
  return status;
}
/*******************************************************************************
**
** Function         phNxpNciHal_getFWDownloadFlags
**
** Description      Returns the current mode
**
** Parameters       none
**
** Returns          Current mode download/NCI
*******************************************************************************/
int phNxpNciHal_getFWDownloadFlag(uint8_t* fwDnldRequest) {
  int status = NFCSTATUS_FAILED;

  NXPLOG_NCIHAL_D("notifyFwrequest %d", notifyFwrequest);
  if (fwDnldRequest != NULL) {
    status = NFCSTATUS_OK;
    if (notifyFwrequest == true) {
      *fwDnldRequest = true;
      notifyFwrequest = false;
    } else {
      *fwDnldRequest = false;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_configFeatureList
**
** Description      Configures the featureList based on chip type
**                  HW Version information number will provide chipType.
**                  HW Version can be obtained from CORE_INIT_RESPONSE(NCI 1.0)
**                  or CORE_RST_NTF(NCI 2.0)
**
** Parameters       CORE_INIT_RESPONSE/CORE_RST_NTF, len
**
** Returns          none
*******************************************************************************/
void phNxpNciHal_configFeatureList(uint8_t* init_rsp, uint16_t rsp_len) {
    nxpncihal_ctrl.chipType = configChipType(init_rsp,rsp_len);
    tNFC_chipType chipType = nxpncihal_ctrl.chipType;
    CONFIGURE_FEATURELIST(chipType);
    NXPLOG_NCIHAL_D("phNxpNciHal_configFeatureList ()chipType = %d", chipType);
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
tNFC_chipType phNxpNciHal_getChipType() {
    return nxpncihal_ctrl.chipType;
}



#if (NFC_NXP_CHIP_TYPE == PN548C2)
NFCSTATUS phNxpNciHal_core_reset_recovery() {
  NFCSTATUS status = NFCSTATUS_FAILED;

  /*NCI_INIT_CMD*/
  static uint8_t cmd_init_nci[] = {0x20, 0x01, 0x00};
  /*NCI_RESET_CMD*/
  static uint8_t cmd_reset_nci[] = {0x20, 0x00, 0x01,
                                    0x00};  // keep configuration
  static uint8_t cmd_init_nci2_0[] = {0x20, 0x01, 0x02, 0x00, 0x00};
  /* reset config cache */
  uint8_t retry_core_init_cnt = 0;

  if (discovery_cmd_len == 0) {
    goto FAILURE;
  }
  NXPLOG_NCIHAL_D("%s: recovery", __func__);

retry_core_init:
  if (retry_core_init_cnt > 3) {
    goto FAILURE;
  }

  status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("PN54X Reset - FAILED\n");
    goto FAILURE;
  }
  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci), cmd_reset_nci);
  if ((status != NFCSTATUS_SUCCESS) &&
      (nxpncihal_ctrl.retry_cnt >= MAX_RETRY_COUNT)) {
    retry_core_init_cnt++;
    goto retry_core_init;
  } else if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("NCI_CORE_RESET: Failed");
    retry_core_init_cnt++;
    goto retry_core_init;
  }
  if (nxpncihal_ctrl.nci_info.nci_version == NCI_VERSION_2_0) {
    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci2_0), cmd_init_nci2_0);
  } else {
    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci), cmd_init_nci);
  }
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("NCI_CORE_INIT : Failed");
    retry_core_init_cnt++;
    goto retry_core_init;
  }

  status = phNxpNciHal_send_ext_cmd(discovery_cmd_len, discovery_cmd);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("RF_DISCOVERY : Failed");
    retry_core_init_cnt++;
    goto retry_core_init;
  }

  return NFCSTATUS_SUCCESS;
FAILURE:
  abort();
}

void phNxpNciHal_discovery_cmd_ext(uint8_t* p_cmd_data, uint16_t cmd_len) {
  NXPLOG_NCIHAL_D("phNxpNciHal_discovery_cmd_ext");
  if (cmd_len > 0 && cmd_len <= sizeof(discovery_cmd)) {
    memcpy(discovery_cmd, p_cmd_data, cmd_len);
    discovery_cmd_len = cmd_len;
  }
}
#endif