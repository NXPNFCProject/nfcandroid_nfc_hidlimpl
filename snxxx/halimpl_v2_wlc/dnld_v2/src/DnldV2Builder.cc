/*
 * Copyright 2025-2026 NXP
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

#include "DnldV2Builder.h"

DnldV2Builder& DnldV2Builder::getInstance() {
  static DnldV2Builder msDnldV2Builder;
  return msDnldV2Builder;
}

void DnldV2Builder::setParams(uint16_t fragmentLen, const uint8_t* fwImgPtr,
                              uint32_t fwImgLen, uint16_t fwVer) {
  SecureDwnLoadAgent::getInstance().setParams(fragmentLen, fwImgPtr, fwImgLen,
                                              fwVer);
}

NFCSTATUS DnldV2Builder::doFwDnld() {
  return SecureDwnLoadAgent::getInstance().performFWDownload();
}

NFCSTATUS DnldV2Builder::getInfo() {
  return SecureDwnLoadAgent::getInstance().getInfo();
}

void DnldV2Builder::setChipDetected(bool isChipDetected) {
  this->isChipDetected = isChipDetected;
}
bool DnldV2Builder::getChipDetected() { return isChipDetected; }

uint8_t DnldV2Builder::getFwRomVersion() {
  return SecureDwnLoadAgent::getInstance().getFwRomVersion();
}
uint8_t DnldV2Builder::getFwMajorVersion() {
  return SecureDwnLoadAgent::getInstance().getFwMajorVersion();
}
uint8_t DnldV2Builder::getHwVersion() {
  return SecureDwnLoadAgent::getInstance().getHwVersion();
}

SecureDwnLoadAgent::SecureDwnLoadAgent() {
  status = NFCSTATUS_OK;
  mDnldV2State = DnldV2State_t::INIT;
  memset(&sem_cb_data, 0, sizeof(phNxpNciHal_Sem_t));
  mFragmentLen = 0;
  pFwImgPtr = nullptr;
  mFwImgLen = 0;
  mMaxFragmentLen = 0;
  mCurrIdx = 0;
  mFwVer = 0;
  memset(&mGetInfoResp, 0xFF, sizeof(getInfoResp));
  /* set true when in degraded fw dnld*/
  fwDnldForm = false;
}

SecureDwnLoadAgent::~SecureDwnLoadAgent() {}

void SecureDwnLoadAgent::setParams(uint16_t fragmentLen,
                                   const uint8_t* fwImgPtr, uint32_t fwImgLen,
                                   uint16_t fwVer) {
  this->mFragmentLen = fragmentLen; /* Max packet data len set by platform   */
  this->pFwImgPtr = fwImgPtr;       /* Pointer to the fw file                */
  this->mFwImgLen = fwImgLen;       /* Length of the fw file                 */
  this->mFwVer = fwVer;             /* Fw version extracted from the fw file */
  mMaxFragmentLen = DNLDV2_PCKT_FRAGMENT_LEN;
  mCurrIdx = DNLDV2_PCKT_START_IDX; /*Starting location of commands to be sent*/
  fwDnldForm = false; /*Should be set to true in degraded fw dnld*/
}

NFCSTATUS SecureDwnLoadAgent::performFWDownload() {
  NXPLOG_NCIHAL_D("%s LUNA fw download begin", __func__);
  bool flag = true;
  switch (mDnldV2State) {
    case DnldV2State_t::INIT:
      if (phNxpNciHal_init_cb_data(&sem_cb_data, NULL) != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Create sem cb failed");
        status = NFCSTATUS_FAILED;
      }
      sendEdlCmd();
      /* if in tearing mode*/
      if (DnldV2BuilderInstance.getChipDetected()) {
        DnldV2BuilderInstance.setChipDetected(false);
      }
      NXPLOG_NCIHAL_D("%s Fw download state: INIT", __func__);
      // keep read pending to recieve EDL Download mode notification
      status = mPacketFrame.read((pphTmlNfc_TransactCompletionCb_t) &
                                     (SecureDwnLoadAgent::EDLDownloadCallbck),
                                 (void*)this);
      if (status != NFCSTATUS_PENDING) {
        NXPLOG_NCIHAL_E("EDL TML enter download mode notification read error");
        return status;
      }
      if (SEM_WAIT(sem_cb_data)) {
        NXPLOG_NCIHAL_E("%s semaphore wait error", __func__);
        status = NFCSTATUS_FAILED;
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        return status;
      }

      status = mPacketFrame.parseHdllResponse();
      if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E(
            "EDL HDLL enter download mode notification parsing failed");
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        return status;
      }
      NXPLOG_NCIHAL_D(
          "Received Enter download mode notification with status = %d", status);
      mDnldV2State = DnldV2State_t::EDL_CERT;
      [[fallthrough]];

    case DnldV2State_t::EDL_CERT:
      NXPLOG_NCIHAL_D("%s Fw download state: send cert frame", __func__);
      status = EDLDownload();
      if (status != NFCSTATUS_SUCCESS) {
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        NXPLOG_NCIHAL_E("EDL enter commit mode notification parsing error");
        return status;
      }
      mDnldV2State = DnldV2State_t::WRITE_FIRST;
      [[fallthrough]];

    case DnldV2State_t::WRITE_FIRST:
      NXPLOG_NCIHAL_D("%s Fw download state: send write first nvm frame",
                      __func__);
      status = EDLDownload();
      if (status != NFCSTATUS_SUCCESS) {
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        NXPLOG_NCIHAL_E("EDL enter commit mode notification parsing error");
        return status;
      }
      mDnldV2State = DnldV2State_t::NVM_WRITE;
      [[fallthrough]];

    case DnldV2State_t::NVM_WRITE:
      NXPLOG_NCIHAL_D("%s Fw download state: send consecutive write nvm frames",
                      __func__);
      do {
        status = EDLDownload();
        if (status != NFCSTATUS_SUCCESS) {
          phNxpNciHal_cleanup_cb_data(&sem_cb_data);
          NXPLOG_NCIHAL_E("EDL enter commit mode notification parsing error");
          return status;
        }
        flag = pFwImgPtr[mCurrIdx + DNLDV2_HDLL_OPCODE] ==
                       (int)SecureDwnldHcpEdl_t::
                           GROUP_EDL_DOWNLOAD_NVM_WRITE_LAST_NXP
                   ? false
                   : true;
        if (mCurrIdx > mFwImgLen) flag = false;
      } while (flag);
      mDnldV2State = DnldV2State_t::NVM_WRITE_LAST;
      [[fallthrough]];

    case DnldV2State_t::NVM_WRITE_LAST:
      NXPLOG_NCIHAL_D("%s Fw download state: send last nvm frame", __func__);
      status = EDLDownload();
      if (status != NFCSTATUS_SUCCESS) {
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        NXPLOG_NCIHAL_E("EDL enter commit mode notification parsing error");
        return status;
      }
      mDnldV2State = DnldV2State_t::COMMIT_MODE;
      [[fallthrough]];

    case DnldV2State_t::COMMIT_MODE:
      NXPLOG_NCIHAL_D("%s LUNA fw download state: commit mode", __func__);
      status = mPacketFrame.frameHdllMessage(
          false, SecureDwnldHcpType_t::HCP_CMD,
          SecureDwnldHcpGroup_t::HCP_GENERIC,
          SecureDwnldHcpGroupGeneric_t::GROUP_GENERIC_ENTER_COMMIT_MODE, {});

      status = writeSync();
      if (status != NFCSTATUS_SUCCESS) {
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        NXPLOG_NCIHAL_E("EDL enter commit mode response parsing error");
        return status;
      }

      status = mPacketFrame.parseHdllResponse();
      if (status != NFCSTATUS_SUCCESS) {
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        NXPLOG_NCIHAL_D("EDL enter commit mode response parsing error");
        return status;
      }

      NXPLOG_NCIHAL_D("Received enter commit mode response with status = %d",
                      status);
      status = mPacketFrame.read((pphTmlNfc_TransactCompletionCb_t) &
                                     (SecureDwnLoadAgent::EDLDownloadCallbck),
                                 (void*)this);

      if (status != NFCSTATUS_PENDING) {
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        NXPLOG_NCIHAL_E("EDL TML enter commit mode notification read error");
        return status;
      }

      if (SEM_WAIT(sem_cb_data)) {
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        NXPLOG_NCIHAL_E("%s semaphore wait error", __func__);
        status = NFCSTATUS_FAILED;
        return status;
      }

      status = mPacketFrame.parseHdllResponse();
      if (status != NFCSTATUS_SUCCESS) {
        phNxpNciHal_cleanup_cb_data(&sem_cb_data);
        NXPLOG_NCIHAL_E("EDL enter commit mode notification parsing error");
        return status;
      }
      NXPLOG_NCIHAL_D(
          "Received Enter commit mode notification with status = %d", status);
      mDnldV2State = DnldV2State_t::DOWNLOAD_COMPLETE;

      /* Required for performing normal fw dnld after degraded fw dnld */
      if (fwDnldForm) {
        fwDnldForm = false;
        NXPLOG_NCIHAL_D("%s Invoking normal fw dnld", __func__);
        mDnldV2State = DnldV2State_t::INIT;
        status = performFWDownload();
        if (status != NFCSTATUS_SUCCESS) {
          phNxpNciHal_cleanup_cb_data(&sem_cb_data);
          NXPLOG_NCIHAL_E("EDL enter commit mode notification parsing error");
          return status;
        }
        NXPLOG_NCIHAL_D("%s Normal fw dnld complete", __func__);
      }
      [[fallthrough]];

    case DnldV2State_t::DOWNLOAD_COMPLETE:
      mDnldV2State = DnldV2State_t::INIT;
      phNxpNciHal_cleanup_cb_data(&sem_cb_data);
      NXPLOG_NCIHAL_D("%s LUNA fw download state: download complete", __func__);
      break;

    default:
      NXPLOG_NCIHAL_D("%s LUNA fw download failed", __func__);
      status = NFCSTATUS_FAILED;
  }
  return status;
}

/*
Recursive function of reading the file and sending EDL data for transceive.
The process would stop we get length as 0, then the state would be changed
to commit mode.
*/
NFCSTATUS SecureDwnLoadAgent::EDLDownload() {
  uint32_t length = 0;
  NXPLOG_NCIHAL_D("%s sending from img file @ idx = 0x%2x", __func__, mCurrIdx);

  status = NFCSTATUS_FAILED;
  if (pFwImgPtr == nullptr) {
    return status;
  }
  if (mCurrIdx > mFwImgLen) {
    return status;
  }

  /* extract the length */
  length |= pFwImgPtr[mCurrIdx] << EDL_SHIFT_BYTE;
  length |= pFwImgPtr[mCurrIdx + EDL_FIRST_OFFSET];
  NXPLOG_NCIHAL_D("%s sending from idx = 0x%2X of len = 0x%2X", __func__,
                  mCurrIdx, length);

  length += DNLDV2_HEADER_CRC_LEN; /* HDLL header */
  status = mPacketFrame.frameHdllMessage((const uint8_t*)(&pFwImgPtr[mCurrIdx]),
                                         length);

  status = writeSync();

  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("EDL enter commit mode response parsing error");
    return status;
  }

  status = mPacketFrame.parseHdllResponse();
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("%s EDL response parsing error", __func__);
    return status;
  }
  mCurrIdx += length;
  mCurrIdx +=
      static_cast<uint8_t> DNLDV2_HEADER_CRC_LEN; /* need to skip 2 bytes of CRC
                                 as it's calculated in code*/

  NXPLOG_NCIHAL_D("Recursive call to %s: @ idx = 0x%2x", __func__, mCurrIdx);
  return status;
}

void SecureDwnLoadAgent::EDLDownloadCallbck(void* pContext,
                                            phTmlNfc_TransactInfo_t* pInfo) {
  SecureDwnLoadAgent* LUNADnldAgent = (SecureDwnLoadAgent*)pContext;
  NXPLOG_NCIHAL_D("%s received EDL response, stopping timer", __func__);
  LUNADnldAgent->mDnldV2Timer.kill();
  LUNADnldAgent->mPacketFrame.setOutData(pInfo->pBuff, pInfo->wLength);
  LUNADnldAgent->status = NFCSTATUS_SUCCESS;
  SEM_POST(&(LUNADnldAgent->sem_cb_data));
  switch (LUNADnldAgent->mDnldV2State) {
    case DnldV2State_t::INIT:
      NXPLOG_NCIHAL_D("%s Received EDL notification received", __func__);
      break;

    case DnldV2State_t::EDL_CERT:
      NXPLOG_NCIHAL_D("%s Received EDL Certificate frame response received",
                      __func__);
      break;

    case DnldV2State_t::WRITE_FIRST:
      NXPLOG_NCIHAL_D("%s Received Write first frame response received",
                      __func__);
      break;

    case DnldV2State_t::NVM_WRITE:
      NXPLOG_NCIHAL_D("%s Received NVM write frame response received",
                      __func__);
      break;

    case DnldV2State_t::NVM_WRITE_LAST:
      NXPLOG_NCIHAL_D("%s Received NVM write last frame response received",
                      __func__);
      break;

    case DnldV2State_t::COMMIT_MODE:
      NXPLOG_NCIHAL_D("%s Received Enter commit mode response received",
                      __func__);
      break;

    case DnldV2State_t::DOWNLOAD_COMPLETE:
      NXPLOG_NCIHAL_D("%s LUNA fw Dnld complete", __func__);
      break;

    case DnldV2State_t::GET_INFO:
      NXPLOG_NCIHAL_D("%s LUNA get info response received", __func__);
      break;
    default:
      break;
  }
}

void SecureDwnLoadAgent::EDLTimerExpired(union sigval val) {  // used for linux
  (void)val;
  NXPLOG_NCIHAL_D("%s EDL Timer Expired", __func__);
  SEM_POST(SecureDwnLoadAgent::getInstance().getSemHandle());
}

void SecureDwnLoadAgent::printGetInfo() {
  NXPLOG_NCIHAL_D("bootStatus                                        :%02X\n",
                  mGetInfoResp.bootStatus);
  NXPLOG_NCIHAL_D("romVersion                                        :%02X\n",
                  mGetInfoResp.romVersion);
  NXPLOG_NCIHAL_D("protectedPageStatus                               :%02X\n",
                  mGetInfoResp.protectedPageStatus);
  NXPLOG_NCIHAL_D(
      "lifecycleVersionScratch                           :%02X%02X\n",
      mGetInfoResp.lifecycleVersionScratch[0],
      mGetInfoResp.lifecycleVersionScratch[1]);
  NXPLOG_NCIHAL_D(
      "lifecycleVersionProtected                         :%02X%02X\n",
      mGetInfoResp.lifecycleVersionProtected[0],
      mGetInfoResp.lifecycleVersionProtected[1]);
  NXPLOG_NCIHAL_D(
      "chipVersion                                       :%02X%02X\n",
      mGetInfoResp.chipVersion[0], mGetInfoResp.chipVersion[1]);
  NXPLOG_NCIHAL_D(
      "firmwareVersionNxpScratch                         :%02X%02X\n",
      mGetInfoResp.firmwareVersionNxpScratch[0],
      mGetInfoResp.firmwareVersionNxpScratch[1]);
  NXPLOG_NCIHAL_D(
      "firmwareVersionNxpProtected                       :%02X%02X\n",
      mGetInfoResp.firmwareVersionNxpProtected[0],
      mGetInfoResp.firmwareVersionNxpProtected[1]);
  NXPLOG_NCIHAL_D(
      "chipVariant                                       :%02X%02X\n",
      mGetInfoResp.chipVariant[0], mGetInfoResp.chipVariant[1]);
  NXPLOG_NCIHAL_D(
      "hwVersion                                         :%02X%02X\n",
      mGetInfoResp.hwVersion[0], mGetInfoResp.hwVersion[1]);
  NXPLOG_NCIHAL_D(
      "deviceLifecycle                                   :%02X%02X%02X%02X\n",
      mGetInfoResp.deviceLifecycle[0], mGetInfoResp.deviceLifecycle[1],
      mGetInfoResp.deviceLifecycle[2], mGetInfoResp.deviceLifecycle[3]);
  NXPLOG_NCIHAL_D(
      "deviceLifecycleScratch                            :%02X%02X%02X%02X\n",
      mGetInfoResp.deviceLifecycleScratch[0],
      mGetInfoResp.deviceLifecycleScratch[1],
      mGetInfoResp.deviceLifecycleScratch[2],
      mGetInfoResp.deviceLifecycleScratch[3]);
  NXPLOG_NCIHAL_D(
      "deviceLifecycleProtected                          :%02X%02X%02X%02X\n",
      mGetInfoResp.deviceLifecycleProtected[0],
      mGetInfoResp.deviceLifecycleProtected[1],
      mGetInfoResp.deviceLifecycleProtected[2],
      mGetInfoResp.deviceLifecycleProtected[3]);
  NXPLOG_NCIHAL_D("dieId                                             :");
  for (int i = 0; i < 16; i++) NXPLOG_NCIHAL_D("%02X ", mGetInfoResp.dieId[i]);
  NXPLOG_NCIHAL_D(
      "dieIdCrc                                          :%02X%02X%02X%02X",
      mGetInfoResp.dieIdCrc[0], mGetInfoResp.dieIdCrc[1],
      mGetInfoResp.dieIdCrc[2], mGetInfoResp.dieIdCrc[3]);
  NXPLOG_NCIHAL_D("minLifeCycleCertificateVersionScratch             :%02X%02X",
                  mGetInfoResp.minLifeCycleCertificateVersionScratch[0],
                  mGetInfoResp.minLifeCycleCertificateVersionScratch[1]);
  NXPLOG_NCIHAL_D("minLifecycleCertificateVersionProtected           :%02X%02X",
                  mGetInfoResp.minLifecycleCertificateVersionProtected[0],
                  mGetInfoResp.minLifecycleCertificateVersionProtected[1]);
  NXPLOG_NCIHAL_D("minDownloadCertificateVersionScratch              :%02X%02X",
                  mGetInfoResp.minDownloadCertificateVersionScratch[0],
                  mGetInfoResp.minDownloadCertificateVersionScratch[1]);
  NXPLOG_NCIHAL_D("minDownloadCertificateVersionProtected            :%02X%02X",
                  mGetInfoResp.minDownloadCertificateVersionProtected[0],
                  mGetInfoResp.minDownloadCertificateVersionProtected[1]);
  NXPLOG_NCIHAL_D("hashAppNxpStatus                                  :%02X",
                  mGetInfoResp.hashAppNxpStatus);
  NXPLOG_NCIHAL_D("hashAppCustStatus                                 :%02X",
                  mGetInfoResp.hashAppCustStatus);
  NXPLOG_NCIHAL_D("hashAppNxp                                        :");
  for (int i = 0; i < 32; i++)
    NXPLOG_NCIHAL_D("%02X ", mGetInfoResp.hashAppNxp[i]);
  NXPLOG_NCIHAL_D("hashAppCust                                       :");
  for (int i = 0; i < 32; i++)
    NXPLOG_NCIHAL_D("%02X ", mGetInfoResp.hashAppCust[i]);
  NXPLOG_NCIHAL_D("trimmings                                         :");
  for (int i = 0; i < 80; i++)
    NXPLOG_NCIHAL_D("%02X ", mGetInfoResp.trimmings[i]);
  NXPLOG_NCIHAL_D("rfu1                                              :%02X",
                  mGetInfoResp.rfu1);
  NXPLOG_NCIHAL_D("modelId                                           :%02X",
                  mGetInfoResp.modelId);
}

NFCSTATUS SecureDwnLoadAgent::getInfo() {
  NFCSTATUS status = NFCSTATUS_OK;

  /* LUNA get info transceive*/
  NXPLOG_NCIHAL_D("%s LUNA fw download sending get info cmd", __func__);
  status = mPacketFrame.frameHdllMessage(
      false, SecureDwnldHcpType_t::HCP_CMD, SecureDwnldHcpGroup_t::HCP_GENERIC,
      SecureDwnldHcpGroupGeneric_t::GROUP_GENERIC_GET_INFO, {});
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("Wrong parameters given for enter commit mode HDLL frame");
    return status;
  }

  status = writeSync();
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("EDL enter commit mode response parsing error");
    return status;
  }

  NXPLOG_NCIHAL_D("%s LUNA fw download received get info cmd response",
                  __func__);
  status = mPacketFrame.parseHdllResponse();
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("EDL get info response parsing error");
    return status;
  }
  if (status == NFCSTATUS_SUCCESS) {
    mPacketFrame.doGetInfo(&mGetInfoResp);
  }
  printGetInfo();
  mDnldV2State = DnldV2State_t::INIT;

  switch (mGetInfoResp.protectedPageStatus) {
    case DNLDV2_INTEGRITY_FAILED:
      NXPLOG_NCIHAL_D("%s Integrity check failed, normal fw dnld required",
                      __func__);
      mDnldV2State = DnldV2State_t::INIT;
      break;
    case DNLDV2_PG_STATUS_FAILED:
      NXPLOG_NCIHAL_D("%s Integrity check failed, commit required", __func__);
      mDnldV2State = DnldV2State_t::COMMIT_MODE;
      break;
    case DNLDV2_PG_STATUS_DEGRADED:
      NXPLOG_NCIHAL_D("%s Page status failed, degraded fw dnld required",
                      __func__);
      mDnldV2State = DnldV2State_t::NVM_WRITE;
      fwDnldForm = true;
      break;
    default:
      break;
  }
  if (mGetInfoResp.hashAppNxpStatus == DNLDV2_INTEGRITY_FAILED) {
    NXPLOG_NCIHAL_D("%s Integrity check failed, normal fw dnld required",
                    __func__);
    mDnldV2State = DnldV2State_t::INIT;
    status = NFCSTATUS_FAILED;
  }

  return status;
}

NFCSTATUS SecureDwnLoadAgent::writeSync() {
  status = mPacketFrame.write();
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("EDL TML write error");
    return status;
  } else {
    if (mDnldV2Timer.set(DNLDV2_TIMER_DURATION, nullptr,
                         &(SecureDwnLoadAgent::EDLTimerExpired))) {
      NXPLOG_NCIHAL_D("%s Timer initialiazation successfull ", __func__);
    } else {
      NXPLOG_NCIHAL_D("%s Timer initialiazation failed ", __func__);
      status = NFCSTATUS_FAILED;
      return status;
    }
  }
  status = mPacketFrame.read((pphTmlNfc_TransactCompletionCb_t) &
                                 (SecureDwnLoadAgent::EDLDownloadCallbck),
                             (void*)this);

  if (status != NFCSTATUS_PENDING) {
    NXPLOG_NCIHAL_D("EDL TML read error");
    return status;
  }

  if (SEM_WAIT(sem_cb_data)) {
    NXPLOG_NCIHAL_E("%s semaphore wait error", __func__);
    status = NFCSTATUS_FAILED;
  }
  status = NFCSTATUS_SUCCESS;
  return status;
}

NFCSTATUS SecureDwnLoadAgent::sendEdlCmd() {
  NFCSTATUS status = NFCSTATUS_OK;

  /* LUNA get info transceive*/
  NXPLOG_NCIHAL_D("%s LUNA fw download sending get info cmd", __func__);
  status = mPacketFrame.frameHdllMessage(
      false, SecureDwnldHcpType_t::HCP_CMD, SecureDwnldHcpGroup_t::HCP_GENERIC,
      SecureDwnldHcpGroupGeneric_t::GROUP_GENERIC_ENTER_DOWNLOAD_MODE, {});
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("Wrong parameters given for enter commit mode HDLL frame");
  }

  status = writeSync();
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("EDL download mode response parsing error");
    return status;
  }

  status = mPacketFrame.parseHdllResponse();
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("EDL download mode response parsing error");
    return status;
  }
  return status;
}

uint8_t SecureDwnLoadAgent::getFwRomVersion() {
  return mGetInfoResp.romVersion;
}
uint8_t SecureDwnLoadAgent::getFwMajorVersion() {
  return mGetInfoResp.firmwareVersionNxpProtected[0];
}
uint8_t SecureDwnLoadAgent::getHwVersion() { return mGetInfoResp.hwVersion[1]; }

SecureDwnLoadAgent& SecureDwnLoadAgent::getInstance() {
  static SecureDwnLoadAgent agent;
  return agent;
}

phNxpNciHal_Sem_t* SecureDwnLoadAgent::getSemHandle() { return &sem_cb_data; }