/*******************************************************************************
 *
 *  Copyright 2019 NXP
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
#include <stdlib.h>
#include <cutils/log.h>
#include "DwpSeChannelCallback.h"
#include "phNxpEse_Api.h"
#include "nfa_ee_api.h"
#include "nfa_hci_api.h"
/** abstract class having pure virtual functions to be implemented be each
 * client  - spi, nfc etc**/
  SyncEvent DwpSeChannelCallback::mModeSetEvt;
  SyncEvent DwpSeChannelCallback::mPowerLinkEvt;
  SyncEvent DwpSeChannelCallback::mTransEvt;

  tNFA_HANDLE DwpSeChannelCallback::mNfaHciHandle;
  int DwpSeChannelCallback::mActualResponseSize;
/*******************************************************************************
**
** Function:        Open
**
** Description:     Initialize the channel.
**
** Returns:         True if ok.
**
*******************************************************************************/
int16_t DwpSeChannelCallback::open() {
  tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
  ALOGE("phNxpNfc_openEse enter");
  {
    SyncEventGuard guard(mPowerLinkEvt);
    nfaStat = NFA_SendPowerLinkCommand((uint8_t)ESE_HANDLE, 0x03);
    if (nfaStat == NFA_STATUS_OK) {
      mPowerLinkEvt.wait(500);
    }
  }
  {
    ALOGE("phNxpNfc_openEse mode set");
    SyncEventGuard guard(mModeSetEvt);
    nfaStat = NFA_EeModeSet((uint8_t)ESE_HANDLE, NFA_EE_MD_ACTIVATE);
    if (nfaStat == NFA_STATUS_OK) {
      mModeSetEvt.wait(500);
    }
  }
  ALOGE("phNxpNfc_openEse exit: status : %d", nfaStat);

  return nfaStat;
}

/*******************************************************************************
**
** Function:        close
**
** Description:     Close the channel.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool DwpSeChannelCallback::close(int16_t mHandle) {
  if (mHandle != 0)
    return true;
  else
    return false;
}

/*******************************************************************************
**
** Function:        transceive
**
** Description:     Send data to the secure element; read it's response.
**                  xmitBuffer: Data to transmit.
**                  xmitBufferSize: Length of data.
**                  recvBuffer: Buffer to receive response.
**                  recvBufferMaxSize: Maximum size of buffer.
**                  recvBufferActualSize: Actual length of response.
**                  timeoutMillisec: timeout in millisecond
**
** Returns:         True if ok.
**
*******************************************************************************/
bool DwpSeChannelCallback::transceive(
    uint8_t* xmitBuffer, int32_t xmitBufferSize,
    __attribute__((unused)) uint8_t* recvBuffer, int32_t recvBufferMaxSize,
    __attribute__((unused)) int32_t& recvBufferActualSize,
    int32_t timeoutMillisec) {
  (void)xmitBuffer;
  (void)xmitBufferSize;
  (void)timeoutMillisec;
  static const char fn[] = "SE_Transmit";
  tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
  bool isSuccess = false;

  ALOGE("phNxpNfc_EseTransceive enter");
  {
    mActualResponseSize = 0;
    memset(mResponseData, 0, sizeof(mResponseData));
    SyncEventGuard guard(mTransEvt);
    nfaStat = NFA_HciSendApdu(mNfaHciHandle, mActiveEeHandle, xmitBufferSize,
                              xmitBuffer, sizeof(mResponseData), mResponseData,
                              timeoutMillisec);
    ALOGE("%s: status code; nfaStat=0x%X", fn, nfaStat);
    if (nfaStat == NFA_STATUS_OK) {
      ALOGE("phNxpNfc_EseTransceive before waiting");
        mTransEvt.wait();
      ALOGE("phNxpNfc_EseTransceive after waiting");
    } else {
      ALOGE("%s: fail send data; error=0x%X", fn, nfaStat);
      goto TheEnd;
    }
  }
  if (mActualResponseSize > recvBufferMaxSize)
    recvBufferActualSize = recvBufferMaxSize;
  else
    recvBufferActualSize = mActualResponseSize;

  memcpy(recvBuffer, mResponseData, recvBufferActualSize);
  isSuccess = true;
TheEnd:
  ALOGE("phNxpNfc_EseTransceive exit");
  return (isSuccess);
}

/*******************************************************************************
**
** Function:        doEseHardReset
**
** Description:     Power OFF and ON to eSE during JCOP Update
**
** Returns:         None.
**
*******************************************************************************/
void DwpSeChannelCallback::doEseHardReset() {
  ALOGE("phNxpNfc_ResetEseJcopUpdate enter");
  tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
  {
    SyncEventGuard guard(mPowerLinkEvt);
    nfaStat = NFA_SendPowerLinkCommand((uint8_t)ESE_HANDLE, 0x00);
    if (nfaStat == NFA_STATUS_OK) {
      mPowerLinkEvt.wait(500);
    }
  }
  {
    ALOGE("phNxpNfc_ResetEseJcopUpdate mode set");
    SyncEventGuard guard(mModeSetEvt);
    nfaStat = NFA_EeModeSet((uint8_t)ESE_HANDLE, NFA_EE_MD_DEACTIVATE);
    if (nfaStat == NFA_STATUS_OK) {
      mModeSetEvt.wait(500);
    }
  }
  ALOGE("phNxpNfc_ResetEseJcopUpdate power link");
  {
    SyncEventGuard guard(mPowerLinkEvt);
    nfaStat = NFA_SendPowerLinkCommand((uint8_t)ESE_HANDLE, 0x03);
    if (nfaStat == NFA_STATUS_OK) {
      mPowerLinkEvt.wait(500);
    }
  }
  {
    ALOGE("phNxpNfc_ResetEseJcopUpdate mode set");
    SyncEventGuard guard(mModeSetEvt);
    nfaStat = NFA_EeModeSet((uint8_t)ESE_HANDLE, NFA_EE_MD_ACTIVATE);
    if (nfaStat == NFA_STATUS_OK) {
      mModeSetEvt.wait(500);
    }
  }
  ALOGE("phNxpNfc_ResetEseJcopUpdate exit");
}

/*******************************************************************************
**
** Function:        getInterfaceInfo
**
** Description:     NFCC and eSE feature flags
**
** Returns:         None.
**
*******************************************************************************/
uint8_t DwpSeChannelCallback::getInterfaceInfo() { return INTF_NFC; }
