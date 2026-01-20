/**
 *
 *  Copyright 2025 NXP
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
 **/
#ifndef RF_STATE_MONITOR_H
#define RF_STATE_MONITOR_H

#include "NciStateMonitor.h"

class RfStateMonitor {
public:
  static const uint8_t NXP_NON_STD_SAK_VALUE = 0x13;
  static const uint8_t NXP_STD_SAK_VALUE = 0x53;
  static const uint8_t T2T_RF_PROTOCOL = 0x02;
  static const uint8_t ISO_DEP_RF_PROTOCOL = 0x04;

  friend class NciStateMonitor;
  static RfStateMonitor *getInstance();

  RfStateMonitor(const RfStateMonitor &) = delete;
  RfStateMonitor &operator=(const RfStateMonitor &) = delete;
  NFCSTATUS processRfDiscRspNtf(std::vector<uint8_t> &rfResNtf);
  NfcRfState getNfcRfState();
  /**
   * @brief Releases all the resources
   * @return None
   *
   */
  static inline void finalize() {
    if (instance != nullptr) {
      instance.reset();
    }
  }

private:
  /**
   * @brief This API handle the RF_INTF_ACTIVATED_NTF for Non-Nfc Forum T2T.
   *
   * @param RF_INTF_ACTIVATED_NTF
   */
  NFCSTATUS processRfIntfActdNtf(std::vector<uint8_t> &rfIntfActdNtf);

  /**
   * @brief This API handle the RF_DISC_NTF for Non-Nfc Forum T2T.
   *
   * @param RF_DISC_NTF
   */
  NFCSTATUS processRfDiscNtf(std::vector<uint8_t> &rfDiscNtf);
  void updateNfcRfState(NfcRfState state);
  static std::unique_ptr<RfStateMonitor> instance;
  NfcRfState nfcRfState;

  RfStateMonitor();
  ~RfStateMonitor();
  friend struct std::default_delete<RfStateMonitor>;
};
#endif // RF_STATE_MONITOR_H
