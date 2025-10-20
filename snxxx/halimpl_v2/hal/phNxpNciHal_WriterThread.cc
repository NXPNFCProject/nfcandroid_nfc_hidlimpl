/*
 * Copyright 2025 NXP
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

#include "phNxpNciHal_WriterThread.h"

#include <phDal4Nfc_messageQueueLib.h>
#include <phNxpNciHal_ext.h>

#include "NfcExtension.h"

phNxpNciHal_WriterThread::phNxpNciHal_WriterThread() : writer_thread(0), thread_running(false) {
  writer_queue = 0;
}

phNxpNciHal_WriterThread::~phNxpNciHal_WriterThread() { Stop(); }

phNxpNciHal_WriterThread& phNxpNciHal_WriterThread::getInstance() {
  static phNxpNciHal_WriterThread instance;
  return instance;
}

bool phNxpNciHal_WriterThread::Start() {
  /* The thread_running.load() and thread_running.store() methods are
     part of the C++11 std::atomic class. These methods are used to
     perform atomic operations on variables, which ensures that the
     operations are thread-safe without needing explicit locks or mutexes */
  if (!thread_running.load()) {
    thread_running.store(true);
    writer_queue = phDal4Nfc_msgget(0, 0600);
    if (writer_queue == 0) {
      NXPLOG_NCIHAL_E("%s:Failed to create writer queue", __func__);
      return false;
    }
    int val = pthread_create(&writer_thread, NULL,
                             phNxpNciHal_WriterThread::WriterThread, this);
    if (val != 0) {
      thread_running.store(false);
      phDal4Nfc_msgdestroy(writer_queue);
      writer_queue = 0;
      NXPLOG_NCIHAL_E("%s:pthread_create failed", __func__);
      return false;
    }
  }
  return true;
}

bool phNxpNciHal_WriterThread::Post(uint8_t* data, uint16_t length) {
  phLibNfc_Message_t msg;
  if (!data || length == 0 || length > PHNCI_MAX_DATA_LEN) {
    NXPLOG_NCIHAL_E("%s:Invalid input buffer", __func__);
    return false;
  }
  msg.pMsgData = NULL;
  msg.w_status = 0;
  msg.eMsgType = NCI_HAL_TML_WRITE_MSG;
  msg.Size = length;
  phNxpNciHal_Memcpy(msg.data, length, data, length);
  if (phDal4Nfc_msgsnd(writer_queue, &msg, 0) != 0) {
    return false;
  }
  return true;
}

bool phNxpNciHal_WriterThread::Post(phLibNfc_Message_t& msg) {
  if (phDal4Nfc_msgsnd(writer_queue, &msg, 0) != 0) {
    return false;
  }
  return true;
}

bool phNxpNciHal_WriterThread::Stop() {
  if (thread_running.load()) {
    thread_running.store(false);
    phDal4Nfc_msgsempost(writer_queue);
    if (pthread_join(writer_thread, static_cast<void**>(NULL)) != 0) {
      NXPLOG_NCIHAL_E("%s:pthread_join failed", __func__);
      phDal4Nfc_msgdestroy(writer_queue);
      writer_queue = 0;
      return false;
    }
    writer_thread = 0;
    phDal4Nfc_msgdestroy(writer_queue);
    writer_queue = 0;
    NXPLOG_NCIHAL_D("WriterThread stopped");
  }
  return true;
}

void* phNxpNciHal_WriterThread::WriterThread(void* arg) {
  phNxpNciHal_WriterThread* worker =
      static_cast<phNxpNciHal_WriterThread*>(arg);
  worker->Run();
  return nullptr;
}

void phNxpNciHal_WriterThread::Run() {
  phLibNfc_Message_t msg;
  NXPLOG_NCIHAL_D("WriterThread started");

  while (thread_running.load()) {
    memset(&msg, 0x00, sizeof(phLibNfc_Message_t));
    if (phDal4Nfc_msgrcv(writer_queue, &msg, 0, 0) == -1) {
      NXPLOG_NCIHAL_E("WriterThread received bad message");
      continue;
    }
    if (!thread_running.load()) {
      break;
    }
    switch (msg.eMsgType) {
      case NCI_HAL_TML_WRITE_MSG: {
        NXPLOG_NCIHAL_D("%s: Received NCI_HAL_TML_WRITE_MSG", __func__);
        CONCURRENCY_LOCK();
        uint32_t bytesWritten = phNxpNciHal_write_unlocked(
            static_cast<uint16_t>(msg.Size), static_cast<uint8_t*>(msg.data),
            ORIG_EXTNS);
        CONCURRENCY_UNLOCK();
        if (bytesWritten == msg.Size) {
          phNxpExtn_WriteCompleteStatusUpdate(NFCSTATUS_SUCCESS);
        } else {
          phNxpExtn_WriteCompleteStatusUpdate(NFCSTATUS_FAILED);
        }
        break;
      }
      case HAL_CTRL_GRANTED_MSG: {
        NXPLOG_NCIHAL_D("Processing HAL_CTRL_GRANTED_MSG");
        phNxpExtn_NfcHalControlGranted();
        break;
      }
      default: {
        NXPLOG_NCIHAL_E("Unexpected msg type");
        break;
      }
    }
  }
  return;
}
