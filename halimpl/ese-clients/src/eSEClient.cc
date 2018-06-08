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
#if(NXP_EXTNS == TRUE)
/*static char gethex(const char *s, char **endptr);
char *convert(const char *s, int *length);*/
uint8_t datahex(char c);
//static android::sp<ISecureElementHalCallback> cCallback;
void* performJCOS_Download_thread(void* data);
IChannel_t Ch;
extern phNxpNciHal_Control_t nxpncihal_ctrl;
static const char *path[3] = {"/data/vendor/nfc/JcopOs_Update1.apdu",
                             "/data/vendor/nfc/JcopOs_Update2.apdu",
                             "/data/vendor/nfc/JcopOs_Update3.apdu"};

static const char *uai_path[2] = {"/data/vendor/nfc/cci.jcsh",
                                  "/data/vendor/nfc/jci.jcsh"};

int16_t SE_Open()
{
    return SESTATUS_SUCCESS;
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
      return true;
    else
      return false;
}

SESTATUS ESE_ChannelInit(IChannel *ch)
{
    ch->open = SE_Open;
    ch->close = SE_Close;
    ch->transceive = SE_Transmit;
    ch->doeSE_Reset = SE_Reset;
    ch->doeSE_JcopDownLoadReset = SE_JcopDownLoadReset;
    return SESTATUS_SUCCESS;
}
#endif
/*******************************************************************************
**
** Function:        LSC_doDownload
**
** Description:     Perform LS during hal init
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
SESTATUS JCOS_doDownload( ) {
  SESTATUS status = SESTATUS_FAILED;
#if(NXP_EXTNS == TRUE)
  performJCOS_Download_thread(NULL);
#endif
  return status;
}
#if(NXP_EXTNS == TRUE)
/*******************************************************************************
**
** Function:        LSC_doDownload
**
** Description:     Perform LS during hal init
**
** Returns:         None
**
*******************************************************************************/
void* performJCOS_Download_thread(void* data) {
  ALOGD("%s enter:  ", __func__);
  (void)data;
  uint8_t status;
  uint8_t jcop_download_state = 0;
  bool stats = true;
  struct stat st;
  for (int num = 0; num < 2; num++)
  {
      if (stat(uai_path[num], &st))
      {
          stats = false;
      }
  }
  /*If UAI specific files are present*/
  if(stats == true)
  {
      for (int num = 0; num < 3; num++)
      {
          if (stat(path[num], &st))
          {
              stats = false;
          }
      }
  }
  unsigned long num = 0;
  int isfound = GetNxpNumValue(NAME_NXP_P61_JCOP_DEFAULT_INTERFACE, &num, sizeof(num));
  if (stats && isfound > 0) {
     if(num == 1) {
      stats = true;
     } else {
      stats  = false;
     }
  }
  ALOGE("%s: performJCOS_Download_thread isfound %x", __FUNCTION__,isfound);
  ALOGE("%s: performJCOS_Download_thread ESE_ChannelInit num %lx ", __FUNCTION__,num);
  if(stats)
  {
      jcop_download_state = 1;
      nfc_nci_IoctlInOutData_t inpOutData;
      memset(&inpOutData, 0x00, sizeof(nfc_nci_IoctlInOutData_t));
      inpOutData.inp.data.nciCmd.cmd_len = sizeof(jcop_download_state);
      memcpy(inpOutData.inp.data.nciCmd.p_cmd, &jcop_download_state,sizeof(jcop_download_state));
      inpOutData.inp.data_source = 2;
      phNxpNfc_InitLib();
      phNxpNciHal_ioctl(HAL_NFC_IOCTL_NFC_JCOP_DWNLD, &inpOutData);

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
      phNxpNfc_DeInitLib();
      jcop_download_state = 0;
      memset(&inpOutData, 0x00, sizeof(nfc_nci_IoctlInOutData_t));
      inpOutData.inp.data.nciCmd.cmd_len = sizeof(jcop_download_state);
      memcpy(inpOutData.inp.data.nciCmd.p_cmd, &jcop_download_state,sizeof(jcop_download_state));
      inpOutData.inp.data_source = 2;
      phNxpNciHal_ioctl(HAL_NFC_IOCTL_NFC_JCOP_DWNLD, &inpOutData);
      sleep(1);
  }
  ALOGD("%s pthread_exit\n", __func__);
  return NULL;
}

/*******************************************************************************
**
** Function:        datahex
**
** Description:     Converts char to uint8_t
**
** Returns:         uint8_t variable
**
*******************************************************************************/
uint8_t datahex(char c) {
  uint8_t value = 0;
  if (c >= '0' && c <= '9')
    value = (c - '0');
  else if (c >= 'A' && c <= 'F')
    value = (10 + (c - 'A'));
  else if (c >= 'a' && c <= 'f')
    value = (10 + (c - 'a'));
  return value;
}
#endif
