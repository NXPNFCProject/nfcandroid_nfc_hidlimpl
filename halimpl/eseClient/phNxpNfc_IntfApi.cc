/*
 * Copyright (C) 2018-2019 NXP Semiconductors
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
#define LOG_TAG "JcDnld_DWP"
#include <phNxpNfc_IntfApi.h>
#include <phNfcCommon.h>
#include <SyncEvent.h>

#define ESE_HCI_APDU_PIPE_ID 0x19
#define EVT_END_OF_APDU_TRANSFER 0x61
uint8_t EVT_SEND_DATA = 0x10;

#if (NXP_EXTNS == TRUE)
/*********************** Global Variables *************************************/
static void nfaDeviceManagementCallback(uint8_t dmEvent,
                                        tNFA_DM_CBACK_DATA* eventData);
static void nfaConnectionCallback(uint8_t connEvent,
                                  tNFA_CONN_EVT_DATA* eventData);
static void setHalFunctionEntries(tHAL_NFC_ENTRY* halFuncEntries);
void nfaHciCallback(tNFA_HCI_EVT event, tNFA_HCI_EVT_DATA* eventData);
void nfaEeCallback(tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData);
/*********************** Function impl*************************************/
uint8_t sIsNfaEnabled = FALSE;
const char* APP_NAME = "nfc_jni";
const uint8_t MAX_NUM_EE = 4;
static const int ESE_HANDLE = 0x4C0;
tNFA_HANDLE mNfaHciHandle;  // NFA handle to NFA's HCI component
static const unsigned int MAX_RESPONSE_SIZE = 0x8800;  // 1024; //34K
uint8_t mResponseData[MAX_RESPONSE_SIZE];
tNFA_HANDLE mActiveEeHandle = 0x4C0;
phNxpNciHal_Sem_t cb_data;

struct timespec ts;
uint8_t isNfcInitialzed = false;

NFCSTATUS phNxpNfc_InitLib() {
  static const char fn[] = "phNxpNfc_InitLib";
  tNFA_STATUS stat = NFCSTATUS_SUCCESS;
  ALOGE("phNxpNfc_InitLib enter");
  if (isNfcInitialzed) {
    ALOGE("phNxpNfc_InitLib already initialized");
    return stat;
  }
  if (sem_init(&((cb_data).sem), 0, 0) == -1) ALOGE("semaphore init error");
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1) ALOGE("clock getitme  failed");

  HalNfcAdaptation& theInstance = HalNfcAdaptation::GetInstance();
  theInstance.Initialize();  // start GKI, NCI task, NFC task
  {
    tHAL_NFC_ENTRY* halFuncEntries = theInstance.GetHalEntryFuncs();
    setHalFunctionEntries(halFuncEntries);
    NFA_Init(halFuncEntries);
    stat = NFA_Enable(nfaDeviceManagementCallback, nfaConnectionCallback);
    if (stat == NFA_STATUS_OK) {
      ALOGE("phNxpNfc_InitLib before waiting");
      if (SEM_WAIT(cb_data)) {
        ALOGE("semaphore error");
        return stat;
      }
      ALOGE("phNxpNfc_InitLib after waiting");
    }
    isNfcInitialzed = true;
    ALOGD("%s: try ee register", fn);
    stat = NFA_EeRegister(nfaEeCallback);
    if (stat != NFA_STATUS_OK) {
      ALOGE("%s: fail ee register; error=0x%X", fn, stat);
      return false;
    }
    if (SEM_WAIT(cb_data)) {
      ALOGE("semaphore error");
      return stat;
    }
    tNFA_EE_INFO mEeInfo[MAX_NUM_EE];  // actual size stored in mActualNumEe
    uint8_t max_num_nfcee = MAX_NUM_EE;
    NFA_EeGetInfo(&max_num_nfcee, mEeInfo);
    for (size_t xx = 0; xx < MAX_NUM_EE; xx++) {
      if ((mEeInfo[xx].ee_handle != ESE_HANDLE) ||
          ((((mEeInfo[xx].ee_interface[0] == NCI_NFCEE_INTERFACE_HCI_ACCESS) &&
             (mEeInfo[xx].ee_status == NFC_NFCEE_STATUS_ACTIVE)) ||
            (NFA_GetNCIVersion() == NCI_VERSION_2_0)))) {
        ALOGD("%s: Found HCI network, try hci register", fn);
        stat =
            NFA_HciRegister(const_cast<char*>(APP_NAME), nfaHciCallback, true);
        if (stat != NFA_STATUS_OK) {
          ALOGE("%s: fail hci register; error=0x%X", fn, stat);
          return (false);
        }
        if (SEM_WAIT(cb_data)) {
          ALOGE("semaphore error");
        }
        break;
      }
    }
  }

  ALOGE("phNxpNfc_InitLib exit");
  return stat;
}

static void nfaDeviceManagementCallback(uint8_t dmEvent,
                                        tNFA_DM_CBACK_DATA* eventData) {
  ALOGE("nfaDeviceManagementCallback enter");
  switch (dmEvent) {
    case NFA_DM_ENABLE_EVT: /* Result of NFA_Enable */
    {
      ALOGD("%s: NFA_DM_ENABLE_EVT; status=0x%X", __func__, eventData->status);
      sIsNfaEnabled = eventData->status == NFA_STATUS_OK;
      ALOGE("NFA_DM_ENABLE_EVT event");
      SEM_POST(&cb_data);
    } break;
    case NFA_DM_DISABLE_EVT: /* Result of NFA_Disable */
    {
      ALOGD("%s: NFA_DM_DISABLE_EVT", __func__);
      sIsNfaEnabled = false;
      ALOGE("NFA_DM_DISABLE_EVT event");
      SyncEventGuard guard(DwpSeChannelCallback::mTransEvt);
      DwpSeChannelCallback::mTransEvt.notifyOne();
    } break;
  }
}

static void nfaConnectionCallback(uint8_t connEvent,
                                  tNFA_CONN_EVT_DATA* eventData) {
  UNUSED(connEvent);
  UNUSED(eventData);
  ALOGE("nfaConnectionCallback connEvent = %d", connEvent);
}

void HalOpen(tHAL_NFC_CBACK* p_hal_cback, tHAL_NFC_DATA_CBACK* p_data_cback) {
  ESE_UPDATE_STATE old_state_dwp = eseUpdateDwp;
  eseUpdateDwp = ESE_UPDATE_COMPLETED;
  phNxpNciHal_open(p_hal_cback, p_data_cback);
  eseUpdateDwp = old_state_dwp;
}

void nfaEeCallback(tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData) {
  ALOGE("%s: event = %x", __func__, event);
  UNUSED(eventData);
  switch (event) {
    case NFA_EE_REGISTER_EVT: {
      ALOGE("NFA_EE_REGISTER_EVT");
      SEM_POST(&cb_data);
    } break;

    case NFA_EE_MODE_SET_EVT: {
      ALOGE("NFA_EE_MODE_SET_EVT");
      SyncEventGuard guard(DwpSeChannelCallback::mModeSetEvt);
      DwpSeChannelCallback::mModeSetEvt.notifyOne();
    } break;
    case NFA_EE_PWR_LINK_CTRL_EVT: {
      ALOGE("NFA_EE_PWR_LINK_CTRL_EVT");
      SyncEventGuard guard(DwpSeChannelCallback::mPowerLinkEvt);
      DwpSeChannelCallback::mPowerLinkEvt.notifyOne();
    } break;
  }
}

void HalClose() { phNxpNciHal_close(false); }
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
  phNxpNciHal_write(data_len, p_data);
}

/*******************************************************************************
**
** Function:    NfcAdaptation::HalCoreInitialized
**
** Description: Adjust the configurable parameters in the controller.
**
** Returns:     None.
**
*******************************************************************************/
void HalCoreInitialized(uint16_t data_len, uint8_t* p_core_init_rsp_params) {
  const char* func = "NfcAdaptation::HalCoreInitialized";
  UNUSED(data_len);
  UNUSED(p_core_init_rsp_params);

  ALOGD("%s", func);
  phNxpNciHal_core_initialized(p_core_init_rsp_params);
}

static void setHalFunctionEntries(tHAL_NFC_ENTRY* halFuncEntries) {
  halFuncEntries->initialize = NULL;
  halFuncEntries->terminate = NULL;
  halFuncEntries->open = HalOpen;
  halFuncEntries->close = HalClose;
  halFuncEntries->core_initialized = HalCoreInitialized;
  halFuncEntries->write = HalWrite;
  halFuncEntries->get_max_ee = NULL;
  return;
}

void nfaHciCallback(tNFA_HCI_EVT event, tNFA_HCI_EVT_DATA* eventData) {
  static const char fn[] = "nfaHciCallback";
  ALOGD("%s: event=0x%X", fn, event);
  ALOGE("nfaHciCallback");
  switch (event) {
    case NFA_HCI_REGISTER_EVT: {
      ALOGD("%s: NFA_HCI_REGISTER_EVT; status=0x%X; handle=0x%X", fn,
            eventData->hci_register.status, eventData->hci_register.hci_handle);
      DwpSeChannelCallback::mNfaHciHandle = eventData->hci_register.hci_handle;
      SEM_POST(&cb_data);

    } break;

    case NFA_HCI_EVENT_RCVD_EVT: {
      ALOGE("%s: NFA_HCI_EVENT_RCVD_EVT; code: 0x%X; pipe: 0x%X; data len: %u",
            fn, eventData->rcvd_evt.evt_code, eventData->rcvd_evt.pipe,
            eventData->rcvd_evt.evt_len);
      SyncEventGuard guard(DwpSeChannelCallback::mTransEvt);
      DwpSeChannelCallback::mTransEvt.notifyOne();
    }
    case NFA_HCI_RSP_APDU_RCVD_EVT: {
      ALOGD("%s: NFA_HCI_RSP_APDU_RCVD_EVT", fn);
      if (eventData->apdu_rcvd.apdu_len > 0) {
        DwpSeChannelCallback::mActualResponseSize =
            (eventData->apdu_rcvd.apdu_len > MAX_RESPONSE_SIZE)
                ? MAX_RESPONSE_SIZE
                : eventData->apdu_rcvd.apdu_len;
      }
      SyncEventGuard guard(DwpSeChannelCallback::mTransEvt);
      DwpSeChannelCallback::mTransEvt.notifyOne();
    } break;
  }
}
#endif
