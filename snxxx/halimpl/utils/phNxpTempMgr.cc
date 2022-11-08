/*
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 ** http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 **
 ** Copyright 2023 NXP
 **
 */
#include "phNxpTempMgr.h"
#include <unistd.h>
#include <mutex>

#include <phNxpConfig.h>
#include <phNxpLog.h>
#include <phOsalNfc_Timer.h>

using namespace std;

static void tempNTf_timeout_cb(uint32_t TimerId, void* pContext) {
  (void)TimerId;
  NXPLOG_NCIHAL_D("phNxpTempMgr::tempNTf_timeout_cb timedout");
  phNxpTempMgr* tMgr = (phNxpTempMgr*)pContext;
  tMgr->Reset(false /*reset timer*/);
}

phNxpTempMgr::phNxpTempMgr() {
  timeout_timer_id_ = 0;
  num_of_ntf_ = 0;
  write_delay_ = 1000;   // 1 sec
  timeout_ = 10 * 1000;  // 10 secs
}

phNxpTempMgr& phNxpTempMgr::GetInstance() {
  static phNxpTempMgr _nxpTempmgr;
  return _nxpTempmgr;
}

void phNxpTempMgr::ParseResponse(uint8_t* p_ntf, uint16_t p_len) {
  (void)p_len;
  NFCSTATUS status = NFCSTATUS_FAILED;

  std::lock_guard<std::mutex> lock(ic_temp_mutex_);
  bool is_ic_temp_ok = p_ntf[03] == 0x00 ? true : false;

  NXPLOG_NCIHAL_D("phNxpTempMgr: IC temp state is %d", is_ic_temp_ok);
  if (!is_ic_temp_ok) {
    num_of_ntf_++;
    if (timeout_timer_id_ == 0) timeout_timer_id_ = phOsalNfc_Timer_Create();
    /* Start timer */
    status = phOsalNfc_Timer_Start(timeout_timer_id_, timeout_,
                                   &tempNTf_timeout_cb, this);
    if (NFCSTATUS_SUCCESS != status) {
      NXPLOG_NCIHAL_D("tempNtf timer not started!!!");
    }
  } else {
    if (num_of_ntf_ > 0) num_of_ntf_--;
    if (num_of_ntf_ == 0) {
      /* Stop Timer */
      status = phOsalNfc_Timer_Stop(timeout_timer_id_);
      if (NFCSTATUS_SUCCESS != status) {
        NXPLOG_NCIHAL_D("tempNtf timer stop ERROR!!!");
      }
    }
  }
}

void phNxpTempMgr::CheckAndWait() {
  if (num_of_ntf_ > 0) {
    NXPLOG_NCIHAL_D("Wait for %d seconds", write_delay_ / 1000);
    usleep(write_delay_ * 1000);
  }
}
void phNxpTempMgr::Reset(bool reset_timer) {
  NXPLOG_NCIHAL_D("phNxpTempMgr::Reset ");
  std::lock_guard<std::mutex> lock(ic_temp_mutex_);
  num_of_ntf_ = 0;
  if (reset_timer) {
    timeout_timer_id_ = 0;
  }
}
