/******************************************************************************
 *
 *  Copyright 2018 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include "eSEClient.h"
#include <cutils/log.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <IChannel.h>
#include <JcDnld.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "nfa_hci_api.h"
#include "nfa_api.h"
#include <phNxpNfc_IntfApi.h>
#include <phNxpNciHal.h>
#include <phNxpConfig.h>
#include "eSEClientIntf.h"
#include <LsClient.h>
#include "hal_nxpese.h"
#include <vendor/nxp/nxpese/1.0/INxpEse.h>

using vendor::nxp::nxpese::V1_0::INxpEse;
using android::hardware::hidl_vec;
using android::sp;
using android::hardware::Return;
using android::hardware::Void;

android::sp<INxpEse> mHalNxpEse;
uint8_t datahex(char c);
IChannel_t Ch;
se_extns_entry se_intf;
void* eSEClientUpdate_ThreadHandler(void* data);
void* eSEClientUpdate_Thread(void* data);
void eSEClientUpdate_Thread();
void* eSEClientUpdate_NFC_ThreadHandler(void* data);
SESTATUS ESE_ChannelInit(IChannel *ch);
uint8_t handleJcopOsDownload();
uint8_t performLSUpdate();
void* LSUpdate_Thread(void* data);
extern phNxpNciHal_Control_t nxpncihal_ctrl;
void seteSEClientState(uint8_t state);
SESTATUS eSEUpdate_SeqHandler();
int16_t SE_Open()
{
  if(phNxpNfc_openEse() == SESTATUS_SUCCESS)
  {
    ALOGD("%s enter: success ", __func__);
    return SESTATUS_SUCCESS;
  }
  else
  {
    ALOGD("%s enter: failed ", __func__);
    return SESTATUS_FAILED;
  }
}

void SE_Reset()
{
    //SESTATUS_SUCCESS;
}

bool SE_Transmit(uint8_t* xmitBuffer, int32_t xmitBufferSize, uint8_t* recvBuffer,
                     int32_t recvBufferMaxSize, int32_t& recvBufferActualSize, int32_t timeoutMillisec)
{
   return phNxpNfc_EseTransceive(xmitBuffer, xmitBufferSize, recvBuffer,
                    recvBufferMaxSize, recvBufferActualSize, timeoutMillisec);
}

void SE_JcopDownLoadReset()
{

    phNxpNfc_ResetEseJcopUpdate();

}

bool SE_Close(int16_t mHandle)
{
    if(mHandle != 0)
    {
      phNxpNfc_closeEse();
      return true;
    }
    else
      return false;
}
uint8_t SE_getInterfaceInfo()
{
  return INTF_NFC;
}
/***************************************************************************
**
** Function:        checkEseClientUpdate
**
** Description:     Check the initial condition
                    and interafce for eSE Client update for LS and JCOP download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void checkEseClientUpdate()
{
  ALOGD("%s enter:  ", __func__);
  bool nfcSEIntfPresent = false;
  char nfcterminal[5];
  checkeSEClientRequired(ESE_INTF_NFC);
  se_intf.isJcopUpdateRequired = getJcopUpdateRequired();
  se_intf.isLSUpdateRequired = getLsUpdateRequired();
  se_intf.sJcopUpdateIntferface = getJcopUpdateIntf();
  se_intf.sLsUpdateIntferface = getLsUpdateIntf();
  if(getNfcSeTerminalId(nfcterminal)) {
    nfcSEIntfPresent = true;
    ALOGD("%s SMB intf  is present  ", __func__);
  }
  if(((se_intf.isJcopUpdateRequired && se_intf.sJcopUpdateIntferface)||
   (se_intf.isLSUpdateRequired && se_intf.sLsUpdateIntferface))&& (nfcSEIntfPresent)) {
    seteSEClientState(ESE_UPDATE_STARTED);
  }
  else
  {
     se_intf.isJcopUpdateRequired = false;
     se_intf.isLSUpdateRequired = false;
     setJcopUpdateRequired(0);
     setLsUpdateRequired(0);
     ALOGD("%s LS and JCOP download not required ", __func__);
  }
}

/***************************************************************************
**
** Function:        IoctlCallback
**
** Description:     Callback for IOCTL API
**
** Returns:         void
**
*******************************************************************************/

void IoctlCallback(hidl_vec<uint8_t> outputData) {
  const char* func = "IoctlCallback";
  ese_nxp_ExtnOutputData_t* pOutData =
      (ese_nxp_ExtnOutputData_t*)&outputData[0];
  ALOGD_IF(nfc_debug_enabled, "%s Ioctl Type=%lu", func,
           (unsigned long)pOutData->ioctlType);
  NfcAdaptation* pAdaptation = (NfcAdaptation*)pOutData->context;
  /*Output Data from stub->Proxy is copied back to output data
   * This data will be sent back to libese*/
  memcpy(&pAdaptation->mCurrentIoctlData->out, &outputData[0],
         sizeof(ese_nxp_ExtnOutputData_t));
}

/***************************************************************************
**
** Function:        perform_eSEClientUpdate
**
** Description:     Perform LS and JCOP download during hal service init
**
** Returns:         SUCCESS / SESTATUS_FAILED
**
*******************************************************************************/
SESTATUS perform_eSEClientUpdate() {

  ALOGD("%s enter:  ", __func__);

  eSEClientUpdate_Thread();
  return SESTATUS_SUCCESS;
}

SESTATUS ESE_ChannelInit(IChannel *ch)
{
    ch->open = SE_Open;
    ch->close = SE_Close;
    ch->transceive = SE_Transmit;
    ch->doeSE_Reset = SE_Reset;
    ch->doeSE_JcopDownLoadReset = SE_JcopDownLoadReset;
    ch->getInterfaceInfo = SE_getInterfaceInfo;
    return SESTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function:        eSEClientUpdate_Thread
**
** Description:     Perform eSE update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void eSEClientUpdate_Thread()
{
  SESTATUS status = SESTATUS_FAILED;
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&thread, &attr, &eSEClientUpdate_ThreadHandler, NULL) != 0) {
    ALOGD("Thread creation failed");
    status = SESTATUS_FAILED;
  } else {
    status = SESTATUS_SUCCESS;
    ALOGD("Thread creation success");
  }
    pthread_attr_destroy(&attr);
}

/*******************************************************************************
**
** Function:        eSEClientUpdate_NFC_Thread
**
** Description:     Perform eSE update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void eSEClientUpdate_NFC_Thread()
{
  SESTATUS status = SESTATUS_FAILED;
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&thread, &attr, &eSEClientUpdate_NFC_ThreadHandler, NULL) != 0) {
    ALOGD("Thread creation failed");
    status = SESTATUS_FAILED;
  } else {
    status = SESTATUS_SUCCESS;
    ALOGD("Thread creation success");
  }
    pthread_attr_destroy(&attr);
}
/*******************************************************************************
**
** Function:        eSEClientUpdate_NFC_ThreadHandler
**
** Description:     Perform JCOP Download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void* eSEClientUpdate_NFC_ThreadHandler(void* data) {
  (void)data;
  ALOGD("%s Enter\n", __func__);
  eSEUpdate_SeqHandler();
  ALOGD("%s Exit eSEClientUpdate_Thread\n", __func__);
  return NULL;

}
/*******************************************************************************
**
** Function:        eSEClientUpdate_ThreadHandler
**
** Description:     Perform JCOP Download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void* eSEClientUpdate_ThreadHandler(void* data) {
  (void)data;
  nfc_nci_IoctlInOutData_t inpOutData;
  int state, cnt = 0;

  ALOGD("%s Enter\n", __func__);
  while(((mHalNxpEse == nullptr) && (cnt < 3)))
  {
    mHalNxpEse = INxpEse::tryGetService();
    ALOGD_IF(mHalNxpEse == nullptr, ": Failed to retrieve the NXP ESE HAL!");
    if(mHalNxpEse != nullptr) {
      ALOGD_IF(nfc_debug_enabled, ": INxpEse::getService() returned %p (%s)",
                mHalNxpEse.get(),
               (mHalNxpEse->isRemote() ? "remote" : "local"));
    }
    usleep(100*1000);
    cnt++;
  }
  if(mHalNxpEse != nullptr)
  {
    memset(&inpOutData, 0x00, sizeof(nfc_nci_IoctlInOutData_t));
    inpOutData.inp.data.nciCmd.cmd_len = sizeof(state);
    memcpy(inpOutData.inp.data.nciCmd.p_cmd, &state,sizeof(state));

    hidl_vec<uint8_t> data;

    nfc_nci_IoctlInOutData_t* pInpOutData = &inpOutData;
    //ALOGD_IF(nfc_debug_enabled, "%s arg=%ld", func, arg);
    pInpOutData->inp.context = &NfcAdaptation::GetInstance();
    NfcAdaptation::GetInstance().mCurrentIoctlData = pInpOutData;
    data.setToExternal((uint8_t*)pInpOutData, sizeof(nfc_nci_IoctlInOutData_t));

    mHalNxpEse->ioctl(HAL_ESE_IOCTL_GET_ESE_UPDATE_STATE, data, IoctlCallback);

    ALOGD_IF(nfc_debug_enabled, "Ioctl Completed for Type result = %d", pInpOutData->out.data.status);
    if(!se_intf.isJcopUpdateRequired && (pInpOutData->out.data.status & 0xFF))
    {
	  se_intf.isJcopUpdateRequired = true;
    }
    if(!se_intf.isLSUpdateRequired && ((pInpOutData->out.data.status >> 8) & 0xFF))
    {
	  se_intf.isLSUpdateRequired = true;
    }

  }

  if(se_intf.isJcopUpdateRequired) {
    if(se_intf.sJcopUpdateIntferface == ESE_INTF_SPI) {
      seteSEClientState(ESE_JCOP_UPDATE_REQUIRED);
      return NULL;
    }
    else if(se_intf.sJcopUpdateIntferface == ESE_INTF_NFC) {
      seteSEClientState(ESE_JCOP_UPDATE_REQUIRED);
    }
  }

  if((ESE_JCOP_UPDATE_REQUIRED != ese_update) && (se_intf.isLSUpdateRequired)) {
    if(se_intf.sLsUpdateIntferface == ESE_INTF_SPI) {
      seteSEClientState(ESE_LS_UPDATE_REQUIRED);
      return NULL;
    }
    else if(se_intf.sLsUpdateIntferface == ESE_INTF_NFC) {
      seteSEClientState(ESE_LS_UPDATE_REQUIRED);
    }
  }

  if((ese_update == ESE_JCOP_UPDATE_REQUIRED) ||
    (ese_update == ESE_LS_UPDATE_REQUIRED))

  eSEUpdate_SeqHandler();
  ALOGD("%s Exit eSEClientUpdate_Thread\n", __func__);
  return NULL;
}

/*******************************************************************************
**
** Function:        handleJcopOsDownload
**
** Description:     Perform JCOP update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
uint8_t handleJcopOsDownload() {
  ALOGD("%s enter:  ", __func__);

  uint8_t status;

  phNxpNfc_InitLib();

  usleep(50 * 1000);
  ALOGE("%s: after init", __FUNCTION__);
  nfc_debug_enabled = true;
  ESE_ChannelInit(&Ch);
  ALOGE("%s: ESE_ChannelInit", __FUNCTION__);

  status = JCDNLD_Init(&Ch);
  ALOGE("%s: JCDNLD_Init", __FUNCTION__);

  if(status != STATUS_SUCCESS)
  {
      ALOGE("%s: JCDND initialization failed", __FUNCTION__);
  }else
  {
      status = JCDNLD_StartDownload();
      ALOGE("%s: JCDNLD_StartDownload", __FUNCTION__);
      if(status != SESTATUS_SUCCESS)
      {
          ALOGE("%s: JCDNLD_StartDownload failed", __FUNCTION__);
      }
  }
  JCDNLD_DeInit();
  if(!(se_intf.isLSUpdateRequired && se_intf.sLsUpdateIntferface == ESE_INTF_NFC ))
    phNxpNfc_DeInitLib();
  sleep(1);

  ALOGD("%s pthread_exit\n", __func__);
  return status;
}

/*******************************************************************************
**
** Function:        performLSUpdate
**
** Description:     Perform LS update
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
uint8_t performLSUpdate()
{
  uint8_t status = SESTATUS_SUCCESS;

  phNxpNfc_InitLib();

  usleep(50 * 1000);
  ALOGE("%s: after init", __FUNCTION__);
  nfc_debug_enabled = true;
  ESE_ChannelInit(&Ch);
  ALOGE("%s: ESE_ChannelInit", __FUNCTION__);

  status = performLSDownload(&Ch);

  phNxpNfc_DeInitLib();
  sleep(1);

  return status;
}

/*******************************************************************************
**
** Function:        seteSEClientState
**
** Description:     Function to set the eSEUpdate state
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void seteSEClientState(uint8_t state)
{
  ALOGE("%s: State = %d", __FUNCTION__, state);
  ese_update = (ese_update_state_t)state;
}

/*******************************************************************************
**
** Function:        sendeSEUpdateState
**
** Description:     Notify NFC HAL LS / JCOP download state
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void sendeSEUpdateState(uint8_t state)
{
  nfc_nci_IoctlInOutData_t inpOutData;
  ALOGE("%s: State = %d", __FUNCTION__, state);
  memset(&inpOutData, 0x00, sizeof(nfc_nci_IoctlInOutData_t));
  inpOutData.inp.data.nciCmd.cmd_len = sizeof(state);
  memcpy(inpOutData.inp.data.nciCmd.p_cmd, &state,sizeof(state));
  inpOutData.inp.data_source = 2;
  phNxpNciHal_ioctl(HAL_ESE_IOCTL_NFC_JCOP_DWNLD, &inpOutData);
}

/*******************************************************************************
**
** Function:        eSEUpdate_SeqHandler
**
** Description:     ESE client update handler
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
SESTATUS eSEUpdate_SeqHandler()
{
    switch(ese_update)
    {
      case ESE_UPDATE_STARTED:
        break;
      case ESE_JCOP_UPDATE_REQUIRED:
        ALOGE("%s: ESE_JCOP_UPDATE_REQUIRED", __FUNCTION__);
        if(se_intf.isJcopUpdateRequired) {
          if(se_intf.sJcopUpdateIntferface == ESE_INTF_NFC) {
            handleJcopOsDownload();
            sendeSEUpdateState(ESE_JCOP_UPDATE_COMPLETED);
          }
          else if(se_intf.sJcopUpdateIntferface == ESE_INTF_SPI) {
            return SESTATUS_SUCCESS;
          }
        }
      case ESE_JCOP_UPDATE_COMPLETED:
      case ESE_LS_UPDATE_REQUIRED:
        if(se_intf.isLSUpdateRequired) {
          if(se_intf.sLsUpdateIntferface == ESE_INTF_NFC) {
            performLSUpdate();
            sendeSEUpdateState(ESE_LS_UPDATE_COMPLETED);
          }
          else if(se_intf.sLsUpdateIntferface == ESE_INTF_SPI)
          {
            seteSEClientState(ESE_LS_UPDATE_REQUIRED);
            return SESTATUS_SUCCESS;
          }
        }
      case ESE_LS_UPDATE_COMPLETED:
      case ESE_UPDATE_COMPLETED:

      {
        nfc_nci_IoctlInOutData_t inpOutData;
        seteSEClientState(ESE_UPDATE_COMPLETED);
        ALOGD("LSUpdate Thread not required inform NFC to restart");
        memset(&inpOutData, 0x00, sizeof(nfc_nci_IoctlInOutData_t));
        inpOutData.inp.data_source = 2;
        usleep(50 * 1000);
        phNxpNciHal_ioctl(HAL_NFC_IOCTL_ESE_UPDATE_COMPLETE, &inpOutData);
      }
      break;
    }
    return SESTATUS_SUCCESS;
}
