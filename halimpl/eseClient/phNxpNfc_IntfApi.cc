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
#include <phNxpNfc_IntfApi.h>
#include <android-base/stringprintf.h>
#include <base/logging.h>
#include <phNfcCommon.h>
#include <phNxpNciHal.h>

#define ESE_HCI_APDU_PIPE_ID 0x19
#define EVT_END_OF_APDU_TRANSFER 0x61

#if(NXP_EXTNS == TRUE)
 /*********************** Global Variables *************************************/
static void nfaDeviceManagementCallback(uint8_t dmEvent,
                                 tNFA_DM_CBACK_DATA* eventData);
static void nfaConnectionCallback(uint8_t connEvent,
                                  tNFA_CONN_EVT_DATA* eventData);
static void setHalFunctionEntries(tHAL_NFC_ENTRY* halFuncEntries);
void nfaHciCallback(tNFA_HCI_EVT event,
                                   tNFA_HCI_EVT_DATA* eventData);
void nfaEeCallback(tNFA_EE_EVT event,
                                   tNFA_EE_CBACK_DATA* eventData);
/*********************** Function impl*************************************/
using android::base::StringPrintf;
uint8_t sIsNfaEnabled = FALSE;
const char* APP_NAME = "nfc_jni";
const uint8_t MAX_NUM_EE = 4;
static const int ESE_HANDLE = 0x4C0;
tNFA_HANDLE     mNfaHciHandle;          //NFA handle to NFA's HCI component
int     mActualResponseSize;         //number of bytes in the response received from secure element
static const unsigned int MAX_RESPONSE_SIZE = 0x8800;//1024; //34K
uint8_t mResponseData [MAX_RESPONSE_SIZE];
tNFA_HANDLE  mActiveEeHandle = 0x4C0;
phNxpNciHal_Sem_t cb_data;
phNxpNciHal_Sem_t cb_data_trans;
phNxpNciHal_Sem_t cd_mode_set;
phNxpNciHal_Sem_t cb_powerlink;
struct timespec ts;
uint8_t isNfcInitialzed = false;
NFCSTATUS phNxpNfc_ResetEseJcopUpdate() {

  NXPLOG_NCIHAL_E("phNxpNfc_ResetEseJcopUpdate enter");
  tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
  nfaStat = NFA_SendPowerLinkCommand((uint8_t)ESE_HANDLE, 0x00);
  if(nfaStat ==  NFA_STATUS_OK) {
    if (SEM_WAIT(cb_powerlink)) {
      NXPLOG_NCIHAL_E("semaphore error");
      return nfaStat;
    }
  }
  NXPLOG_NCIHAL_E("phNxpNfc_ResetEseJcopUpdate mode set");
  nfaStat =  NFA_EeModeSet((uint8_t)ESE_HANDLE, NFA_EE_MD_DEACTIVATE);
  if(nfaStat == NFA_STATUS_OK)
  {
   if (SEM_WAIT(cd_mode_set)) {
      NXPLOG_NCIHAL_E("semaphore error");
      return nfaStat;
    }
    //usleep(500 * 1000);
    usleep(10 * 1000);
  }
  NXPLOG_NCIHAL_E("phNxpNfc_ResetEseJcopUpdate power link");
  nfaStat = NFA_SendPowerLinkCommand((uint8_t)ESE_HANDLE, 0x03);
  if(nfaStat ==  NFA_STATUS_OK) {
    if (SEM_WAIT(cb_powerlink)) {
      NXPLOG_NCIHAL_E("semaphore error");
      return nfaStat;
    }
  }
  NXPLOG_NCIHAL_E("phNxpNfc_ResetEseJcopUpdate mode set");
  nfaStat =  NFA_EeModeSet((uint8_t)ESE_HANDLE, NFA_EE_MD_ACTIVATE);
  if(nfaStat == NFA_STATUS_OK)
  {
      if (SEM_WAIT(cd_mode_set)) {
        NXPLOG_NCIHAL_E("semaphore error");
        return nfaStat;
      }
    //usleep(500 * 1000);
      usleep(500 * 1000);
  }
  NXPLOG_NCIHAL_E("phNxpNfc_ResetEseJcopUpdate exit");

  return nfaStat;
}

NFCSTATUS phNxpNfc_openEse() {
  tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
  NXPLOG_NCIHAL_E("phNxpNfc_openEse enter");

  nfaStat = NFA_SendPowerLinkCommand((uint8_t)ESE_HANDLE, 0x03);
  if(nfaStat ==  NFA_STATUS_OK) {
    if (SEM_WAIT(cb_powerlink)) {
      NXPLOG_NCIHAL_E("semaphore error");
      return nfaStat;
    }
  }
  NXPLOG_NCIHAL_E("phNxpNfc_openEse mode set");
  nfaStat =  NFA_EeModeSet((uint8_t)ESE_HANDLE, NFA_EE_MD_ACTIVATE);
  if(nfaStat == NFA_STATUS_OK)
  {
      if (SEM_WAIT(cd_mode_set)) {
        NXPLOG_NCIHAL_E("semaphore error");
        return nfaStat;
      }
      usleep(100 * 1000);
  }
  NXPLOG_NCIHAL_E("phNxpNfc_openEse exit: status : %d",nfaStat);

  return nfaStat;
}

NFCSTATUS phNxpNfc_closeEse() {
  tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
  NXPLOG_NCIHAL_E("phNxpNfc_closeEse enter");

  nfaStat = NFA_SendPowerLinkCommand((uint8_t)ESE_HANDLE, 0x01);
  if(nfaStat ==  NFA_STATUS_OK) {
    if (SEM_WAIT(cb_powerlink)) {
      NXPLOG_NCIHAL_E("semaphore error");
      return nfaStat;
    }
  }
  NXPLOG_NCIHAL_E("phNxpNfc_closeEse END OF APDU");

  NFA_HciSendEvent (mNfaHciHandle, ESE_HCI_APDU_PIPE_ID, EVT_END_OF_APDU_TRANSFER, 0x00, NULL, 0x00,NULL, 0);
  NXPLOG_NCIHAL_E("phNxpNfc_closeEse exit: status : %d",nfaStat);

  return nfaStat;
}

NFCSTATUS phNxpNfc_InitLib() {
  static const char fn [] = "phNxpNfc_InitLib";
  tNFA_STATUS stat = NFCSTATUS_SUCCESS;
  ALOGE("phNxpNfc_InitLib enter");
  if(isNfcInitialzed)
  {
	  ALOGE("phNxpNfc_InitLib already initialized");
	  return stat;
  }
  if (sem_init(&((cb_data).sem), 0, 0) == -1)
    NXPLOG_NCIHAL_E("semaphore init error");
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        NXPLOG_NCIHAL_E("clock getitme  failed");

  //SEM_TIMED_WAIT(cb_data, ts);
  NfcAdaptation& theInstance = NfcAdaptation::GetInstance();
  theInstance.Initialize();  // start GKI, NCI task, NFC task
  {
    tHAL_NFC_ENTRY* halFuncEntries = theInstance.GetHalEntryFuncs();
    setHalFunctionEntries(halFuncEntries);
    NFA_Init(halFuncEntries);
    stat = NFA_Enable(nfaDeviceManagementCallback, nfaConnectionCallback);
    if (stat == NFA_STATUS_OK) {
      NXPLOG_NCIHAL_E("phNxpNfc_InitLib before waiting");
      if (SEM_WAIT(cb_data)) {
        NXPLOG_NCIHAL_E("semaphore error");
        return stat;
      }
      NXPLOG_NCIHAL_E("phNxpNfc_InitLib after waiting");
    }
    isNfcInitialzed = true;
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: try ee register", fn);
    stat = NFA_EeRegister(nfaEeCallback);
    if (stat != NFA_STATUS_OK) {
      LOG(ERROR) << StringPrintf("%s: fail ee register; error=0x%X", fn,
                              stat);
      return false;
    }
    if (SEM_WAIT(cb_data)) {
      NXPLOG_NCIHAL_E("semaphore error");
      return stat;
    }
    tNFA_EE_INFO mEeInfo [MAX_NUM_EE];  //actual size stored in mActualNumEe
    uint8_t max_num_nfcee = MAX_NUM_EE;
    NFA_EeGetInfo(&max_num_nfcee, mEeInfo);
    for (size_t xx = 0; xx < MAX_NUM_EE; xx++)
    {
      if((mEeInfo[xx].ee_handle != ESE_HANDLE)
         || ((((mEeInfo[xx].ee_interface[0] == NCI_NFCEE_INTERFACE_HCI_ACCESS)
                 && (mEeInfo[xx].ee_status == NFC_NFCEE_STATUS_ACTIVE)) || (NFA_GetNCIVersion() == NCI_VERSION_2_0))))
      {
          LOG(INFO) << StringPrintf("%s: Found HCI network, try hci register", fn);
          stat = NFA_HciRegister (const_cast<char*>(APP_NAME), nfaHciCallback, true);
          if (stat != NFA_STATUS_OK)
          {
            LOG(ERROR) << StringPrintf("%s: fail hci register; error=0x%X", fn, stat);
            return (false);
          }
          if (SEM_WAIT(cb_data)) {
            NXPLOG_NCIHAL_E("semaphore error");
          }
          break;
      }
    }
  }

    if (phNxpNciHal_init_cb_data(&cb_data_trans, NULL) != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("phNxpNfc_InitLib failed");
    return stat;
  }
  if (phNxpNciHal_init_cb_data(&cd_mode_set, NULL) != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("phNxpNfc_InitLib failed");
    return stat;
  }
  if (phNxpNciHal_init_cb_data(&cb_powerlink, NULL) != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("phNxpNfc_InitLib failed");
    return stat;
  }  

      ALOGE("phNxpNfc_InitLib exit");
  return stat;
}

static void nfaDeviceManagementCallback(uint8_t dmEvent,
                                 tNFA_DM_CBACK_DATA* eventData) {
  NXPLOG_NCIHAL_E("nfaDeviceManagementCallback enter");
  switch (dmEvent) {
    case NFA_DM_ENABLE_EVT: /* Result of NFA_Enable */
    {
      DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf(
          "%s: NFA_DM_ENABLE_EVT; status=0x%X", __func__, eventData->status);
      sIsNfaEnabled = eventData->status == NFA_STATUS_OK;
      ALOGE("NFA_DM_ENABLE_EVT event");
      SEM_POST(&cb_data);
    }
    break;
    case NFA_DM_DISABLE_EVT: /* Result of NFA_Disable */
    {
      DLOG_IF(INFO, nfc_debug_enabled)
          << StringPrintf("%s: NFA_DM_DISABLE_EVT", __func__);
      sIsNfaEnabled = false;
      ALOGE("NFA_DM_DISABLE_EVT event");
      SEM_POST(&cb_data_trans);
    }
    break;
  }
}


static void nfaConnectionCallback(uint8_t connEvent,
                                  tNFA_CONN_EVT_DATA* eventData) {
UNUSED(connEvent);
UNUSED(eventData);
}


NFCSTATUS phNxpNfc_DeInitLib() {
  NXPLOG_NCIHAL_E("phNxpNfc_DeInitLib enter");
  tNFA_STATUS stat = NFA_STATUS_FAILED;
	if (sIsNfaEnabled) {
    tNFA_STATUS stat = NFA_Disable(TRUE /* graceful */);
    if (stat == NFA_STATUS_OK) {
      LOG(ERROR)
      << StringPrintf("%s: wait for completion", __func__);
      //if (SEM_WAIT(cb_data)) {
      if(SEM_WAIT(cb_data_trans)) {
        NXPLOG_NCIHAL_E("semaphore error");
         return stat;
      }
  } else {
      LOG(ERROR) << StringPrintf("%s: fail disable; error=0x%X", __func__,
                               stat);
    }
  }
  isNfcInitialzed = false;
  NXPLOG_NCIHAL_E("phNxpNfc_DeInitLib exit");
  return stat;
}

void HalOpen(tHAL_NFC_CBACK* p_hal_cback,
                            tHAL_NFC_DATA_CBACK* p_data_cback) {
  ese_update_state_t old_state =  ese_update;
  ese_update = ESE_UPDATE_COMPLETED;
  phNxpNciHal_open(p_hal_cback, p_data_cback);
  ese_update = old_state;
  ALOGE("HalOpen exit");
}

void nfaEeCallback(tNFA_EE_EVT event,
                tNFA_EE_CBACK_DATA* eventData) {
  NXPLOG_NCIHAL_E("nfaEeCallback enter");
  UNUSED(eventData);
  switch (event) {
    case NFA_EE_REGISTER_EVT: {
      NXPLOG_NCIHAL_E("NFA_EE_REGISTER_EVT");
      SEM_POST(&cb_data);
    } break;

    case NFA_EE_MODE_SET_EVT: {
      NXPLOG_NCIHAL_E("NFA_EE_MODE_SET_EVT");
      SEM_POST(&cd_mode_set);
    } break;
    case NFA_EE_PWR_LINK_CTRL_EVT:
    {
      NXPLOG_NCIHAL_E("NFA_EE_PWR_LINK_CTRL_EVT");
      SEM_POST(&cb_powerlink);
    }
    break;
  }
}

void HalClose() {
  phNxpNciHal_close(false);
}
/*******************************************************************************
**
** Function:    NfcAdaptation::HalWrite
**
** Description: Write NCI message to the controller.
**
** Returns:     None.
**
*******************************************************************************/
void HalWrite(uint16_t data_len, uint8_t* p_data) {
  phNxpNciHal_write(data_len , p_data);
}
#if 0
/*******************************************************************************
**
** Function:        HAL_NfcControlGranted
**
** Description:     Grant control to HAL control for sending NCI commands.
**                  Call in response to HAL_REQUEST_CONTROL_EVT.
**                  Must only be called when there are no NCI commands pending.
**                  HAL_RELEASE_CONTROL_EVT will notify when HAL no longer
**                  needs control of NCI.
**
** Returns:         void
**
*******************************************************************************/
void HalControlGranted() {
  const char* func = "NfcAdaptation::HalControlGranted";
  //DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s", func);
 
}
#endif
/*******************************************************************************
**
** Function:    NfcAdaptation::HalCoreInitialized
**
** Description: Adjust the configurable parameters in the controller.
**
** Returns:     None.
**
*******************************************************************************/
void HalCoreInitialized(uint16_t data_len,
                                       uint8_t* p_core_init_rsp_params) {
  const char* func = "NfcAdaptation::HalCoreInitialized";
  UNUSED(data_len);
  UNUSED(p_core_init_rsp_params);

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s", func);
  phNxpNciHal_core_initialized(p_core_init_rsp_params);
}

static void setHalFunctionEntries(tHAL_NFC_ENTRY* halFuncEntries)
{

  halFuncEntries->initialize = NULL;
  halFuncEntries->terminate = NULL;
  halFuncEntries->open = HalOpen;
  halFuncEntries->close = HalClose;
  halFuncEntries->core_initialized = HalCoreInitialized;
  halFuncEntries->write = HalWrite;
  halFuncEntries->get_max_ee = NULL;
  return;  
}

void nfaHciCallback(tNFA_HCI_EVT event,
                                   tNFA_HCI_EVT_DATA* eventData) {

  static const char fn [] = "nfaHciCallback";
  LOG(INFO) << StringPrintf("%s: event=0x%X", fn, event);
          NXPLOG_NCIHAL_E("nfaHciCallback");
  switch (event)
  {
    case NFA_HCI_REGISTER_EVT:
    {
      LOG(INFO) << StringPrintf("%s: NFA_HCI_REGISTER_EVT; status=0x%X; handle=0x%X", fn,
      eventData->hci_register.status, eventData->hci_register.hci_handle);
      mNfaHciHandle = eventData->hci_register.hci_handle;
      SEM_POST(&cb_data);

    }
    break;
    case NFA_HCI_RSP_APDU_RCVD_EVT:
    {
            LOG(INFO) << StringPrintf("%s: NFA_HCI_RSP_APDU_RCVD_EVT", fn);
      if(eventData->apdu_rcvd.apdu_len > 0)
      {
          //mTransceiveWaitOk = true;
        mActualResponseSize = (eventData->apdu_rcvd.apdu_len > MAX_RESPONSE_SIZE) ? MAX_RESPONSE_SIZE : eventData->apdu_rcvd.apdu_len;
      }

            SEM_POST(&cb_data_trans);
    }
    break;
  }    
}

bool phNxpNfc_EseTransceive(uint8_t* xmitBuffer, int32_t xmitBufferSize, uint8_t* recvBuffer,
                     int32_t recvBufferMaxSize, int32_t& recvBufferActualSize, int32_t timeoutMillisec)
{
  static const char fn [] = "SE_Transmit";
  tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
  bool isSuccess = false;

  NXPLOG_NCIHAL_E("phNxpNfc_EseTransceive enter");
    {
      mActualResponseSize = 0;
      memset (mResponseData, 0, sizeof(mResponseData));
      nfaStat = NFA_HciSendApdu (mNfaHciHandle, mActiveEeHandle, xmitBufferSize, xmitBuffer, sizeof(mResponseData), mResponseData, timeoutMillisec);
      LOG(ERROR) << StringPrintf("%s: status code; nfaStat=0x%X", fn, nfaStat);
      if (nfaStat == NFA_STATUS_OK)
      {
        NXPLOG_NCIHAL_E("phNxpNfc_EseTransceive before waiting");
        if (SEM_WAIT(cb_data_trans)) {
          NXPLOG_NCIHAL_E("semaphore error");
        return nfaStat;
      }
        NXPLOG_NCIHAL_E("phNxpNfc_EseTransceive after waiting");
      }
      else
      {
        LOG(ERROR) << StringPrintf("%s: fail send data; error=0x%X", fn, nfaStat);
        goto TheEnd;
      }
    }
    if (mActualResponseSize > recvBufferMaxSize)
      recvBufferActualSize = recvBufferMaxSize;
    else
      recvBufferActualSize = mActualResponseSize;

    memcpy (recvBuffer, mResponseData, recvBufferActualSize);
    isSuccess = true;
    TheEnd:
  NXPLOG_NCIHAL_E("phNxpNfc_EseTransceive exit");
    return (isSuccess);
}
#endif

