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
#pragma once

#include <fstream>
#include <mutex>

using namespace std;

class phNxpTempMgr {
 public:
  // mark copy constructor deleted
  phNxpTempMgr(const phNxpTempMgr&) = delete;

  /**
   * Get singleton instance of phNxpTempMgr.
   */
  static phNxpTempMgr& GetInstance();

  /**
   * Parse temperature status notification.
   */
  void ParseResponse(uint8_t* p_ntf, uint16_t p_len);

  /**
   * Apply delay if temp of any of NFCC module is NOK.
   */
  void CheckAndWait();

  /**
   * Reset the state to default.
   */
  void Reset(bool reset_timer = true);

 private:
  // constructor
  phNxpTempMgr();

  std::mutex ic_temp_mutex_;  // Mutex for protecting shared resources

  // tracks number of temp ntf
  // value is incremented for each NTF with temp status as NOK
  // decremented for each NTF with temp status as OK
  // callabck timer is stopped when num_of_ntf reaches 0
  uint8_t num_of_ntf_;

  // delay(in ms) before sending the next nci cmd to NFCC
  uint32_t write_delay_;

  uint32_t timeout_;           // timeout for the tempNTF timeout callback
  uint32_t timeout_timer_id_;  // ID of the tempNTF timeout callback timer
};
