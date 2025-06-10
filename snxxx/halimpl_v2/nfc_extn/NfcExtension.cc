/*
 * Copyright 2024-2025 NXP
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

#include "NfcExtension.h"
#include <dlfcn.h>
#include <phNxpLog.h>
#include <phNxpNciHal.h>
#include "NfcWriter.h"
#include "NxpNfcExtension.h"
#include "NxpNfcThreadMutex.h"

extern phNxpNciHal_Control_t nxpncihal_ctrl;
extern phTmlNfc_Context_t* gpphTmlNfc_Context;
extern tNfc_featureList nfcFL;
fp_extn_init_t fp_extn_init = NULL;
fp_extn_deinit_t fp_extn_deinit = NULL;
fp_extn_handle_nfc_event_t fp_extn_handle_nfc_event = NULL;
const std::string vendor_nfc_init_name = "vendor_nfc_init";
const std::string vendor_nfc_de_init_name = "vendor_nfc_de_init";
const std::string vendor_nfc_handle_event_name = "vendor_nfc_handle_event";

void* p_oem_extn_handle = NULL;
NfcExtEventData_t nfc_ext_event_data;

/**************** local methods used in this file only ************************/
/**
 * @brief Initialize and resets up the extension
 *        feature
 * @return void
 *
 */
static void phNxpExtn_Init();

std::string mLibName = "libnfc_vendor_extn.so";
#if (defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64))
std::string mLibPathName = "/system/vendor/lib64/" + mLibName;
#else
std::string mLibPathName = "/system/vendor/lib/" + mLibName;
#endif

void phNxpExtn_LibSetup() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  p_oem_extn_handle = dlopen(mLibPathName.c_str(), RTLD_NOW);
  if (p_oem_extn_handle == NULL) {
    NXPLOG_NCIHAL_E("%s Error : opening (%s) !!", __func__,
                    mLibPathName.c_str());
    return;
  }

  if ((fp_extn_init = (fp_extn_init_t)dlsym(p_oem_extn_handle,
                                            vendor_nfc_init_name.c_str())) == NULL) {
    NXPLOG_NCIHAL_E("%s Failed to find %s !!", __func__, vendor_nfc_init_name.c_str());
  }

  if ((fp_extn_deinit = (fp_extn_deinit_t)dlsym(
           p_oem_extn_handle, vendor_nfc_de_init_name.c_str())) == NULL) {
    NXPLOG_NCIHAL_E("%s Failed to find %s !!", __func__, vendor_nfc_de_init_name.c_str());
  }

  if ((fp_extn_handle_nfc_event = (fp_extn_handle_nfc_event_t)dlsym(
           p_oem_extn_handle, vendor_nfc_handle_event_name.c_str())) == NULL) {
    NXPLOG_NCIHAL_E("%s Failed to find %s !!", __func__, vendor_nfc_handle_event_name.c_str());
  }
  // Allocate Transaction buffers
  nciMsgDeferredData.tTransactionInfo.pBuff =
      (uint8_t*)calloc(NCI_MAX_DATA_LEN, sizeof(uint8_t));
  if (nciMsgDeferredData.tTransactionInfo.pBuff == NULL) {
    NXPLOG_NCIHAL_E("%s Failed to allocate transaction buffer");
    phNxpExtn_LibClose();
  }
  nciRspNtfDeferredData.tTransactionInfo.pBuff =
      (uint8_t*)calloc(NCI_MAX_DATA_LEN, sizeof(uint8_t));
  if (nciRspNtfDeferredData.tTransactionInfo.pBuff == NULL) {
    NXPLOG_NCIHAL_E("%s Failed to allocate transaction buffer");
    phNxpExtn_LibClose();
  }

  phNxpExtn_Init();
}

/* Extension feature API's Start */
void phNxpExtn_Init() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  if (fp_extn_init != NULL) {
    if (fp_extn_init(nullptr)) {
      NXPLOG_NCIHAL_D("%s : %s Success", __func__,
                      vendor_nfc_init_name.c_str());
    } else {
      NXPLOG_NCIHAL_D("%s: %s Failed", __func__, vendor_nfc_init_name.c_str());
    }
  }
}

void phNxpExtn_LibClose() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  if (fp_extn_deinit != NULL) {
    if (!fp_extn_deinit()) {
      NXPLOG_NCIHAL_D("%s : %s Failed ",
          __func__, vendor_nfc_de_init_name.c_str());
    }
  }
  phNxpNfcExtn_deInit();
  if (p_oem_extn_handle != NULL) {
    NXPLOG_NCIHAL_D("%s Closing libnfc_vendor_extn.so lib", __func__);
    int32_t status = dlclose(p_oem_extn_handle);
    dlerror(); /* Clear any existing error */
    if (status != 0) {
      NXPLOG_NCIHAL_E("%s Free libnfc_vendor_extn.so failed", __func__);
    }
    fp_extn_init = NULL;
    fp_extn_deinit = NULL;
    fp_extn_handle_nfc_event = NULL;
    p_oem_extn_handle = NULL;
  }
  // Free transaction buffers
  if (nciMsgDeferredData.tTransactionInfo.pBuff != NULL) {
    free(nciMsgDeferredData.tTransactionInfo.pBuff);
    nciMsgDeferredData.tTransactionInfo.pBuff = NULL;
  }
  if (nciRspNtfDeferredData.tTransactionInfo.pBuff != NULL) {
    free(nciRspNtfDeferredData.tTransactionInfo.pBuff);
    nciRspNtfDeferredData.tTransactionInfo.pBuff = NULL;
  }
}

NFCSTATUS phNxpExtn_HandleNciMsg(uint16_t *dataLen, const uint8_t* pData) {
  NXPLOG_NCIHAL_D("%s Enter dataLen:%d", __func__, *dataLen);
  NciData_t nci_data;
  nci_data.data_len = *dataLen;
  nci_data.p_data = (uint8_t*)pData;
  nfc_ext_event_data.nci_msg = nci_data;

  if (NFCSTATUS_EXTN_FEATURE_SUCCESS ==
      phNxpNfcExtn_HandleNciMsg(dataLen, pData))
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;

  if (fp_extn_handle_nfc_event != NULL)
    return fp_extn_handle_nfc_event(HANDLE_VENDOR_NCI_MSG, &nfc_ext_event_data);
  else
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

NFCSTATUS phNxpExtn_HandleHalEvent(uint8_t handle_event) {
  NXPLOG_NCIHAL_D("%s Enter handle_event:%d", __func__, handle_event);
  nfc_ext_event_data.hal_event = handle_event;

  if (fp_extn_handle_nfc_event != NULL) {
    return fp_extn_handle_nfc_event(HANDLE_HAL_EVENT, &nfc_ext_event_data);
  } else {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }
}

void phNxpExtn_WriteCompleteStatusUpdate(NFCSTATUS status) {
  NXPLOG_NCIHAL_D("%s Enter status:%d", __func__, status);
  nfc_ext_event_data.write_status = status;
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_WRITE_COMPLETE_STATUS, &nfc_ext_event_data);
  }
}

NFCSTATUS phNxpExtn_HandleNciRspNtf(uint16_t *dataLen, const uint8_t* pData) {
  NXPLOG_NCIHAL_D("%s Enter dataLen:%d", __func__, *dataLen);
  NciData_t nci_data;
  nci_data.data_len = *dataLen;
  nci_data.p_data = (uint8_t*)pData;
  nfc_ext_event_data.nci_rsp_ntf = nci_data;

  if (fp_extn_handle_nfc_event != NULL) {
    if (NFCSTATUS_EXTN_FEATURE_SUCCESS !=
        fp_extn_handle_nfc_event(HANDLE_VENDOR_NCI_RSP_NTF,
                                 &nfc_ext_event_data)) {
      if (NFCSTATUS_EXTN_FEATURE_SUCCESS !=
          phNxpNfcExtn_HandleNciRspNtf(dataLen, pData))
        return NFCSTATUS_EXTN_FEATURE_FAILURE;
    }
  } else {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }
  return NFCSTATUS_EXTN_FEATURE_SUCCESS;
}

void phNxpExtn_FwDnldStatusUpdate(uint8_t status) {
  NXPLOG_NCIHAL_D("%s Enter status:%d", __func__, status);
  nfc_ext_event_data.hal_event_status = status;
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_FW_DNLD_STATUS_UPDATE, &nfc_ext_event_data);
  }
}

NfcRfState_t phNxpExtn_NfcGetRfState() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_RF_HAL_STATE_UPDATE, &nfc_ext_event_data);
  }
  return (NfcRfState_t)nfc_ext_event_data.rf_state;
}

void phNxpExtn_NfcHalStateUpdate(uint8_t state) {
  NXPLOG_NCIHAL_D("%s Enter state:%d", __func__, state);
  nfc_ext_event_data.hal_state = state;
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_NFC_HAL_STATE_UPDATE, &nfc_ext_event_data);
  }
}

void phNxpExtn_NfcHalControlGranted() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_HAL_CONTROL_GRANTED, &nfc_ext_event_data);
  }
}
/* Extension feature API's End */

/* HAL API's Start */
NFCSTATUS phNxpHal_EnqueueWrite(uint8_t* pBuffer, uint16_t wLength) {
  NXPLOG_NCIHAL_D("%s Enter wLength:%d", __func__, wLength);
  nciMsgDeferredData.tTransactionInfo.wStatus = NFCSTATUS_SUCCESS;
  nciMsgDeferredData.tTransactionInfo.wLength = wLength;
  phNxpNciHal_Memcpy(nciMsgDeferredData.tTransactionInfo.pBuff, wLength,
                     pBuffer, wLength);
  nciMsgDeferredData.tDeferredInfo.pParameter =
      &nciMsgDeferredData.tTransactionInfo;
  nciMsgDeferredData.tMsg.pMsgData = &nciMsgDeferredData.tDeferredInfo;
  nciMsgDeferredData.tMsg.Size = sizeof(nciMsgDeferredData.tDeferredInfo);
  nciMsgDeferredData.tMsg.eMsgType = NCI_HAL_TML_WRITE_MSG;
  // Check command window availability before enque packet
  // to free hal worker thread from blocking for command window
  if (NfcWriter::getInstance().check_ncicmd_write_window(wLength, pBuffer) !=
      NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("%s  CMD window  check failed", __func__);
    return NFCSTATUS_FAILED;
  }
  phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId,
                        &nciMsgDeferredData.tMsg);
  return NFCSTATUS_SUCCESS;
}

NFCSTATUS phNxpHal_EnqueueRsp(uint8_t* pBuffer, uint16_t wLength) {
  NXPLOG_NCIHAL_D("%s Enter wLength:%d", __func__, wLength);
  nciRspNtfDeferredData.tTransactionInfo.wStatus = NFCSTATUS_SUCCESS;
  nciRspNtfDeferredData.tTransactionInfo.wLength = wLength;
  phNxpNciHal_Memcpy(nciRspNtfDeferredData.tTransactionInfo.pBuff, wLength,
                     pBuffer, wLength);
  nciRspNtfDeferredData.tDeferredInfo.pParameter =
      &nciRspNtfDeferredData.tTransactionInfo;
  nciRspNtfDeferredData.tMsg.pMsgData = &nciRspNtfDeferredData.tDeferredInfo;
  nciRspNtfDeferredData.tMsg.Size = sizeof(nciRspNtfDeferredData.tDeferredInfo);
  nciRspNtfDeferredData.tMsg.eMsgType = NCI_HAL_OEM_RSP_NTF_MSG;
  phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId,
                        &nciRspNtfDeferredData.tMsg);
  return NFCSTATUS_SUCCESS;
}

void phNxpHal_RequestControl() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  phNxpNciHal_request_control();
}

void phNxpHal_ReleaseControl() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  phNxpNciHal_release_control();
}

void phNxpHal_NfcDataCallback(uint16_t dataLen, const uint8_t* pData) {
  NXPLOG_NCIHAL_D("%s Enter dataLen:%d", __func__, dataLen);
  if (nxpncihal_ctrl.p_nfc_stack_data_cback != NULL) {
    (*nxpncihal_ctrl.p_nfc_stack_data_cback)(dataLen, (uint8_t*)pData);
  }
}

uint8_t phNxpHal_GetNxpByteArrayValue(const char* name, char* pValue,
                                      long bufflen, long* len) {
  return GetNxpByteArrayValue(name, pValue, bufflen, len);
}

uint8_t phNxpHal_GetNxpNumValue(const char* name, void* pValue,
                                unsigned long len) {
  return GetNxpNumValue(name, pValue, len);
}

/* HAL API's End */
