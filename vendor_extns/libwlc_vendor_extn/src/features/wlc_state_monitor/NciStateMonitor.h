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

#ifndef NCI_STATE_MONITOR_H
#define NCI_STATE_MONITOR_H

#include "IEventHandler.h"
#include "WlcExtensionWriter.h"
#include "PalThreadMutex.h"
#include "PlatformAbstractionLayer.h"
#include <cstdint>
#include <map>
#include <vector>

/** \addtogroup STATEMONITOR_EVENT_HANDLER_API_INTERFACE
 *  @brief  interface to perform the NciStateMonitor feature functionality.
 *  @{
 */
class NciStateMonitor : public IEventHandler {
public:
  static NciStateMonitor *getInstance();
  /**
   * @brief handles the vendor NCI message by delegating the message to
   *        current active event handler
   * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled by extension library otherwise
   * WLCSTATUS_EXTN_FEATURE_FAILURE.
   *
   */
  WLCSTATUS handleVendorNciMessage(uint16_t dataLen, const uint8_t *pData);

  /**
   * @brief handles the vendor NCI response/Notification by delegating the
   * message to current active event handler
   * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled by extension library otherwise
   * WLCSTATUS_EXTN_FEATURE_FAILURE.
   *
   */
  WLCSTATUS handleVendorNciRspNtf(uint16_t dataLen, uint8_t *pData);

  /**
   * @brief This API monitor CORE_RESET_NTF to extract information like
   * firmware version.
   * @param CORE_RESET_NTF
   * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled by extension library otherwise
   * WLCSTATUS_EXTN_FEATURE_FAILURE.
   */
  WLCSTATUS processCoreResetNtfReceived(std::vector<uint8_t> coreResetNtf);

  /**
   * @brief on receiving NFCEE_MODE_SET_CMD for ESE, setting eseModeSetCmd to
   * true.
   *
   * @param dataLen
   * @param pData
   * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled by extension library otherwise
   * WLCSTATUS_EXTN_FEATURE_FAILURE.
   */
  WLCSTATUS processNciCmd(uint16_t dataLen, const uint8_t *pData);

  /**
   * @brief this callback will be invoked by controller when HAL
   *        updates the error or failure. Respective feature
   *        Handler have to clean up or handle this error gracefully
   *
   * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is consumed
   * by extension library otherwise WLCSTATUS_EXTN_FEATURE_FAILURE.
   *
   */
  WLCSTATUS handleHalEvent(uint8_t errorCode) override;

  /**
   * @brief Get the Fw Version
   *
   * @return firmwareVersion
   */
  std::vector<uint8_t> getFwVersion();

  static inline void finalize() {
    if (instance != nullptr) {
      instance.reset();
    }
  }

 private:
   static std::unique_ptr<NciStateMonitor> instance;
   std::vector<uint8_t> firmwareVersion;
   std::vector<uint8_t> nciCmd;
   NciStateMonitor();
   ~NciStateMonitor();
   friend struct std::default_delete<NciStateMonitor>;
};
/** @}*/
#endif // NCI_STATE_MONITOR_H
