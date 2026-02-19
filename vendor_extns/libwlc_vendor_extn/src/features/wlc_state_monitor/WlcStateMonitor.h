/**
 *
 *  Copyright 2025-2026 NXP
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
#ifndef WLC_STATE_MONITOR_H
#define WLC_STATE_MONITOR_H

#include "IEventHandler.h"
#include "NciStateMonitor.h"
#include <map>
#include <phNfcStatus.h>
#include <stdint.h>
#include <vector>

class WlcStateMonitor {
public:
  friend class NciStateMonitor;
  static WlcStateMonitor *getInstance();

  WlcStateMonitor(const WlcStateMonitor &) = delete;
  WlcStateMonitor &operator=(const WlcStateMonitor &) = delete;
  /**
   * @brief Releases all the resources
   * @return None
   *
   */
  static inline void finalize() {
    if (instance != nullptr) {
      instance.reset();
    }
  };

/**
 * @brief Processes the Discovery Map Command & update according to NCI2.3.
 *
 * @param dataLen Length of the command data in bytes.
 * @param pData Pointer to the command payload.
 *
 * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
 * feature and handled by extension library otherwise
 * WLCSTATUS_EXTN_FEATURE_FAILURE.
 */
  WLCSTATUS processDiscMapCmd(uint16_t dataLen, const uint8_t *pData);

/**
 * @brief Processes the RF Discover Command & update according to NCI2.3.
 *
 * @param dataLen Length of the command data in bytes.
 * @param pData Pointer to the command payload.
 *
 * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
 * feature and handled by extension library otherwise
 * WLCSTATUS_EXTN_FEATURE_FAILURE.
 */
  WLCSTATUS processRfDiscCmd(uint16_t dataLen, const uint8_t *pData);

/**
 * @brief Handles NCI Responses and Notifications related to WLC.
 *
 * This API parses incoming NCI messages (response or notification) and
 * triggers appropriate actions or callbacks for WLC status and data.
 *
 * @param dataLen Length of the NCI message in bytes.
 * @param pData Pointer to the NCI message payload containing WLC-specific data.
 *
 * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
 * feature and handled by extension library otherwise
 * WLCSTATUS_EXTN_FEATURE_FAILURE.
 */
  WLCSTATUS handleWlcNciRspNtf(uint16_t dataLen, const uint8_t *pData);

private:
  static std::unique_ptr<WlcStateMonitor> instance;
  bool mIsAutonomousModeEn = false;
  WlcStateMonitor();
  ~WlcStateMonitor();
  friend struct std::default_delete<WlcStateMonitor>;
};
#endif // WLC_STATE_MONITOR_H
