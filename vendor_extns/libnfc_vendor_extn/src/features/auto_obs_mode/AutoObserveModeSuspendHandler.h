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
#ifndef NFC_AUTO_OBSERVE_MODE_H
#define NFC_AUTO_OBSERVE_MODE_H

#include "PlatformAbstractionLayer.h"
#include "phNfcStatus.h"

/**
 * @brief Manager class to handle the Auto Observe Mode.
 */
class AutoObserveModeSuspendHandler {
 public:
  /**
   * @brief Get the singleton instance of AutoObserveModeSuspendHandler.
   * @return AutoObserveModeSuspendHandler* Pointer to the instance.
   */
  static AutoObserveModeSuspendHandler* getInstance();

  /**
   * @brief Configure Auto ObserveMode
   * feature .
   */
  void phNxpNciHal_configureAutoObserveModeSuspendHandler();

  /**
   * @brief Releases all the resources
   * @return None
   *
   */
  static inline void finalize() {
    if (sAutoObserveModeSuspendHandler != nullptr) {
      sAutoObserveModeSuspendHandler.reset();
    }
  }

 private:
  static std::unique_ptr<AutoObserveModeSuspendHandler>
      sAutoObserveModeSuspendHandler; /* Singleton instance of
                                         AutoObserveModeSuspendHandler */
  constexpr static uint8_t AUTO_OBSERVE_MODE_CONFIG_SIZE = 0x03;
  constexpr static uint8_t AUTO_OBSERVE_MODE_OID = 0x43;
  constexpr static uint8_t AUTO_OBSERVE_MODE_SET_OP_ID = 0x20;
  constexpr static uint8_t AUTO_OBSERVE_MODE_CMD_PAYLOAD_LENGTH = 0x04;

  AutoObserveModeSuspendHandler();
  ~AutoObserveModeSuspendHandler();
  friend struct std::default_delete<AutoObserveModeSuspendHandler>;
};

#endif  // NFC_AUTO_OBSERVE_MODE_H