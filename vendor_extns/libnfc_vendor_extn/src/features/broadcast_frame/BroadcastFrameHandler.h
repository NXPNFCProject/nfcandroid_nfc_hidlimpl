/******************************************************************************
 *
 *  Copyright 2026 NXP
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
 ******************************************************************************/
#ifndef NFC_BROADCAST_FRAME_HANDLER_H
#define NFC_BROADCAST_FRAME_HANDLER_H

#include <mutex>
#include "NfcExtensionConstants.h"
#include "PlatformAbstractionLayer.h"
#include "phNfcStatus.h"

/**
 * @brief Manager class to handle the Broadcast frame action.
 */
class BroadcastFrameHandler {
 public:
  /**
   * @brief Get the singleton instance of BroadcastFrameHandler.
   * @return BroadcastFrameHandler* Pointer to the instance.
   */
  static BroadcastFrameHandler* getInstance();

  /**
   * @brief Configure Broadcast frame action feature .
   */
  void phNxpNciHal_configureBroadcastFrameHandler();

  /**
   * @brief Process NCI response/notification for Brodcast frame.
   * @param dataLen Length of the data
   * @param pData Pointer to the data
   * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled it internally otherwise NFCSTATUS_EXTN_FEATURE_FAILURE.
   */
  NFCSTATUS handleVendorNciRspNtf(uint16_t dataLen, uint8_t* pData);

  /**
   * @brief Releases all the resources
   * @return None
   *
   */
  static inline void finalize() {
    std::lock_guard<std::mutex> lock(sBroadcastFrameHandlerMutex);
    if (sBroadcastFrameHandler != nullptr) {
      sBroadcastFrameHandler.reset();
    }
  }

 private:
  static std::unique_ptr<BroadcastFrameHandler>
      sBroadcastFrameHandler; /* Singleton instance of BroadcastFrameHandler */
  static std::mutex sBroadcastFrameHandlerMutex;
  constexpr static uint8_t BROADCAST_FRAME_CONFIG_SIZE = 0x13;
  constexpr static uint8_t BROADCAST_ACTION_NTF_GID = 0x61;
  constexpr static uint8_t BROADCAST_ACTION_NTF_OID = 0x16;
  constexpr static uint8_t BROADCAST_ACTION_NTF_SUB_GID = 0x01;
  constexpr static uint8_t BROADCAST_ACTION_NTF_SUB_OID = 0x03;

  BroadcastFrameHandler();
  ~BroadcastFrameHandler();
  friend struct std::default_delete<BroadcastFrameHandler>;
};

#endif  // NFC_BROADCAST_FRAME_HANDLER_H
