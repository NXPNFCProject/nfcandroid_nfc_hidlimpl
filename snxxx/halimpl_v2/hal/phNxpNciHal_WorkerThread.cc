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

#include "phNxpNciHal_WorkerThread.h"
#include <android-base/stringprintf.h>
#include <log/log.h>
#include <phDal4Nfc_messageQueueLib.h>
#include <phNfcNciConstants.h>
#include <phNxpConfig.h>
#include <phNxpEventLogger.h>
#include <phNxpLog.h>
#include <phNxpNciHal.h>
#include <phNxpNciHal_ext.h>
#include <phTmlNfc.h>
#include "NfcExtension.h"

extern phNxpNciHal_Control_t nxpncihal_ctrl;

phNxpNciHal_WorkerThread::phNxpNciHal_WorkerThread() : thread_running(false) {worker_thread = 0;}

phNxpNciHal_WorkerThread::~phNxpNciHal_WorkerThread() { Stop(); }

phNxpNciHal_WorkerThread& phNxpNciHal_WorkerThread::getInstance() {
  static phNxpNciHal_WorkerThread instance;
  return instance;
}

bool phNxpNciHal_WorkerThread::Start() {
  /* The thread_running.load() and thread_running.store() methods are
     part of the C++11 std::atomic class. These methods are used to
     perform atomic operations on variables, which ensures that the
     operations are thread-safe without needing explicit locks or mutexes */
  if (!thread_running.load()) {
    thread_running.store(true);
    int val = pthread_create(&worker_thread, NULL,
                             phNxpNciHal_WorkerThread::WorkerThread, this);
    if (val != 0) {
      thread_running.store(false);
      NXPLOG_NCIHAL_E("pthread_create failed");
      return false;
    }
  }
  return true;
}

bool phNxpNciHal_WorkerThread::Stop() {
  nxpncihal_ctrl.p_nfc_stack_cback = NULL;
  nxpncihal_ctrl.p_nfc_stack_data_cback = NULL;
  thread_running.store(false);

  if ((thread_running.load()) &&
      (pthread_join(worker_thread, (void**)NULL) != 0)) {
    NXPLOG_NCIHAL_E("pthread_join failed");
    return false;
  }
  return true;
}

void* phNxpNciHal_WorkerThread::WorkerThread(void* arg) {
  phNxpNciHal_WorkerThread* worker =
      static_cast<phNxpNciHal_WorkerThread*>(arg);
  worker->Run();
  return nullptr;
}

void phNxpNciHal_WorkerThread::Run() {
  phLibNfc_Message_t msg;
  NXPLOG_NCIHAL_D("HAL Worker thread started");

  while (thread_running.load()) {
    memset(&msg, 0x00, sizeof(phLibNfc_Message_t));
    if (phDal4Nfc_msgrcv(nxpncihal_ctrl.gDrvCfg.nClientId, &msg, 0, 0) == -1) {
      NXPLOG_NCIHAL_E("NFC worker received bad message");
      continue;
    }

    if (!thread_running.load()) {
      break;
    }

    switch (msg.eMsgType) {
      case NCI_HAL_TML_WRITE_MSG: {
        REENTRANCE_LOCK();
        phLibNfc_DeferredCall_t* deferCall =
            (phLibNfc_DeferredCall_t*)(msg.pMsgData);
        phTmlNfc_TransactInfo_t* pInfo =
            (phTmlNfc_TransactInfo_t*)deferCall->pParameter;
        int bytesWritten = phNxpNciHal_write_unlocked(
            (uint16_t)pInfo->oem_cmd_len, (uint8_t*)pInfo->p_oem_cmd_data,
            ORIG_EXTNS);
        if (bytesWritten == pInfo->oem_cmd_len) {
          phNxpExtn_WriteCompleteStatusUpdate(NFCSTATUS_SUCCESS);
        } else {
          phNxpExtn_WriteCompleteStatusUpdate(NFCSTATUS_FAILED);
        }
        REENTRANCE_UNLOCK();
        break;
      }
      case NCI_HAL_OEM_RSP_NTF_MSG: {
        REENTRANCE_LOCK();
        phLibNfc_DeferredCall_t* deferCall =
            (phLibNfc_DeferredCall_t*)(msg.pMsgData);
        phTmlNfc_TransactInfo_t* pInfo =
            (phTmlNfc_TransactInfo_t*)deferCall->pParameter;
        if (nxpncihal_ctrl.p_nfc_stack_data_cback != NULL) {
          (*nxpncihal_ctrl.p_nfc_stack_data_cback)(pInfo->oem_rsp_ntf_len,
                                                   pInfo->p_oem_rsp_ntf_data);
        }
        REENTRANCE_UNLOCK();
        break;
      }
      case HAL_CTRL_GRANTED_MSG: {
        NXPLOG_NCIHAL_D("Processing HAL_CTRL_GRANTED_MSG");
        REENTRANCE_LOCK();
        phNxpExtn_NfcHalControlGranted();
        REENTRANCE_UNLOCK();
        break;
      }
      case PH_LIBNFC_DEFERREDCALL_MSG: {
        REENTRANCE_LOCK();
        phLibNfc_DeferredCall_t* deferCall =
            (phLibNfc_DeferredCall_t*)(msg.pMsgData);

        phTmlNfc_TransactInfo_t transact_info;
        phTmlNfc_TransactInfo_t* ptransact_info = &transact_info;
        if (ptransact_info != NULL && msg.Size > 0) {
          memcpy(nxpncihal_ctrl.p_rsp_data, msg.data, msg.Size);
          ptransact_info->pBuff = nxpncihal_ctrl.p_rsp_data;
          ptransact_info->wLength = msg.Size;
          ptransact_info->wStatus = msg.w_status;
          deferCall->pCallback(ptransact_info);
        } else {
          NXPLOG_NCIHAL_D("Processing the response timeout");
          deferCall->pCallback(deferCall->pParameter);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case NCI_HAL_OPEN_CPLT_MSG: {
        REENTRANCE_LOCK();
        phNxpExtn_HandleHalEvent(HAL_NFC_OPEN_CPLT_EVT);
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
        }
        Stop();
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

      case NCI_HAL_HCI_NETWORK_RESET_MSG: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
          /* Send the event */
          (*nxpncihal_ctrl.p_nfc_stack_cback)(
              (uint32_t)HAL_HCI_NETWORK_RESET_EVT, HAL_NFC_STATUS_OK);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case NCI_HAL_ERROR_MSG: {
        REENTRANCE_LOCK();
        phNxpExtn_HandleHalEvent(NFCC_HAL_TRANS_ERR_CODE);
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
      case NCI_HAL_VENDOR_MSG: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_data_cback != NULL) {
          (*nxpncihal_ctrl.p_nfc_stack_data_cback)(
              nxpncihal_ctrl.vendor_msg_len, nxpncihal_ctrl.vendor_msg);
        }
        REENTRANCE_UNLOCK();
        break;
      }
      case HAL_NFC_FW_UPDATE_STATUS_EVT: {
        REENTRANCE_LOCK();
        if (nxpncihal_ctrl.p_nfc_stack_cback != NULL) {
          /* Send the event */
          (*nxpncihal_ctrl.p_nfc_stack_cback)(msg.eMsgType,
                                              *((uint8_t*)msg.pMsgData));
        }
        REENTRANCE_UNLOCK();
        break;
      }
    }
  }
  return;
}
