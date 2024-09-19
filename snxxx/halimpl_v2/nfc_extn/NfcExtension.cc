/*
 * Copyright 2024 NXP
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
#include <phTmlNfc.h>
#include "NxpNfcThreadMutex.h"

extern phNxpNciHal_Control_t nxpncihal_ctrl;
extern phTmlNfc_Context_t* gpphTmlNfc_Context;
fp_extn_init_t fp_extn_init = NULL;
fp_extn_deinit_t fp_extn_deinit = NULL;
fp_extn_handle_nfc_event_t fp_extn_handle_nfc_event = NULL;

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

/**
 * TODO: Will be removed once fp_extn_deinit() is functional
 * @brief global flag to initialize extention lib only once
 */
static bool initalized = false;

void phNxpExtn_LibSetup() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  p_oem_extn_handle =
      dlopen("/system/vendor/lib64/libnxp_nfc_gen_ext.so", RTLD_NOW);
  if (p_oem_extn_handle == NULL) {
    NXPLOG_NCIHAL_E(
        "%s Error : opening (/system/vendor/lib64/libnxp_nfc_gen_ext.so) !!",
        __func__);
    return;
  }

  if ((fp_extn_init = (fp_extn_init_t)dlsym(p_oem_extn_handle,
                                            "phNxpExtn_LibInit")) == NULL) {
    NXPLOG_NCIHAL_E("%s Failed to find phNxpExtn_LibInit !!", __func__);
  }

  if ((fp_extn_deinit = (fp_extn_deinit_t)dlsym(
           p_oem_extn_handle, "phNxpExtn_LibDeInit")) == NULL) {
    NXPLOG_NCIHAL_E("%s Failed to find phNxpExtn_LibDeInit !!", __func__);
  }

  if ((fp_extn_handle_nfc_event = (fp_extn_handle_nfc_event_t)dlsym(
           p_oem_extn_handle, "phNxpExtn_HandleNfcEvent")) == NULL) {
    NXPLOG_NCIHAL_E("%s Failed to find phNxpExtn_HandleNfcEvent !!", __func__);
  }

  phNxpExtn_Init();
}

/* Extension feature API's Start */
void phNxpExtn_Init() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  if (fp_extn_init != NULL) {
    if (!initalized) {
      fp_extn_init();
      NXPLOG_NCIHAL_D("%s Initialized!", __func__);
      initalized = true;
    } else {
      NXPLOG_NCIHAL_D("%s Already Initialized!", __func__);
    }
  }
}

void phNxpExtn_LibClose() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  if (fp_extn_deinit != NULL) {
    fp_extn_deinit();
  }
  if (p_oem_extn_handle != NULL) {
    NXPLOG_NCIHAL_D("%s Closing libnxp_nfc_gen_ext.so lib", __func__);
    dlclose(p_oem_extn_handle);
    p_oem_extn_handle = NULL;
  }
}

NFCSTATUS phNxpExtn_HandleNciMsg(uint16_t dataLen, const uint8_t* pData) {
  NXPLOG_NCIHAL_D("%s Enter dataLen:%d", __func__, dataLen);
  nci_data_t nci_data;
  nci_data.data_len = dataLen;
  nci_data.p_data = (uint8_t*)pData;
  nfc_ext_event_data.nci_msg = nci_data;

  if (fp_extn_handle_nfc_event != NULL) {
    return fp_extn_handle_nfc_event(HANDLE_VENDOR_NCI_MSG, nfc_ext_event_data);
  } else {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }
}

void phNxpExtn_WriteCompleteStatusUpdate(NFCSTATUS status) {
  NXPLOG_NCIHAL_D("%s Enter status:%d", __func__, status);
  nfc_ext_event_data.write_status = status;
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_WRITE_COMPLETE_STATUS, nfc_ext_event_data);
  }
}

NFCSTATUS phNxpExtn_HandleNciRspNtf(uint16_t dataLen, const uint8_t* pData) {
  NXPLOG_NCIHAL_D("%s Enter dataLen:%d", __func__, dataLen);
  nci_data_t nci_data;
  nci_data.data_len = dataLen;
  nci_data.p_data = (uint8_t*)pData;
  nfc_ext_event_data.nci_rsp_ntf = nci_data;

  if (fp_extn_handle_nfc_event != NULL) {
    return fp_extn_handle_nfc_event(HANDLE_VENDOR_NCI_RSP_NTF,
                                    nfc_ext_event_data);
  } else {
    return NFCSTATUS_EXTN_FEATURE_FAILURE;
  }
}

// TODO: Shall it be directly maintained in Extension library
void phNxpExtn_NfcRfStateUpdate(uint8_t state) {
  NXPLOG_NCIHAL_D("%s Enter state:%d", __func__, state);
  nfc_ext_event_data.rf_state = state;
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_RF_HAL_STATE_UPDATE, nfc_ext_event_data);
  }
}
void phNxpExtn_NfcHalStateUpdate(uint8_t state) {
  NXPLOG_NCIHAL_D("%s Enter state:%d", __func__, state);
  nfc_ext_event_data.hal_state = state;
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_NFC_HAL_STATE_UPDATE, nfc_ext_event_data);
  }
}

void phNxpExtn_NfcHalControlGranted() {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  if (fp_extn_handle_nfc_event != NULL) {
    fp_extn_handle_nfc_event(HANDLE_HAL_CONTROL_GRANTED, nfc_ext_event_data);
  }
}
/* Extension feature API's End */

/* HAL API's Start */
NFCSTATUS phNxpHal_EnqueueWrite(uint8_t* pBuffer, uint16_t wLength) {
  NXPLOG_NCIHAL_D("%s Enter wLength:%d", __func__, wLength);
  phLibNfc_DeferredCall_t tDeferredInfo;
  phLibNfc_Message_t tMsg;
  phTmlNfc_TransactInfo_t tTransactionInfo;

  tTransactionInfo.wStatus = NFCSTATUS_SUCCESS;
  tTransactionInfo.oem_cmd_len = wLength;
  phNxpNciHal_Memcpy(tTransactionInfo.p_oem_cmd_data, wLength, pBuffer,
                     wLength);
  tDeferredInfo.pParameter = &tTransactionInfo;
  tMsg.pMsgData = &tDeferredInfo;
  tMsg.Size = sizeof(tDeferredInfo);
  tMsg.eMsgType = NCI_HAL_TML_WRITE_MSG;
  phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId, &tMsg);

  return NFCSTATUS_SUCCESS;
}

NFCSTATUS phNxpHal_EnqueueRsp(uint8_t* pBuffer, uint16_t wLength) {
  NXPLOG_NCIHAL_D("%s Enter wLength:%d", __func__, wLength);
  phLibNfc_DeferredCall_t tDeferredInfo;
  phLibNfc_Message_t tMsg;
  phTmlNfc_TransactInfo_t tTransactionInfo;

  tTransactionInfo.wStatus = NFCSTATUS_SUCCESS;
  tTransactionInfo.oem_rsp_ntf_len = wLength;
  phNxpNciHal_Memcpy(tTransactionInfo.p_oem_rsp_ntf_data, wLength, pBuffer,
                     wLength);
  tDeferredInfo.pParameter = &tTransactionInfo;
  tMsg.pMsgData = &tDeferredInfo;
  tMsg.Size = sizeof(tDeferredInfo);
  tMsg.eMsgType = NCI_HAL_OEM_RSP_NTF_MSG;
  phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId, &tMsg);
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
