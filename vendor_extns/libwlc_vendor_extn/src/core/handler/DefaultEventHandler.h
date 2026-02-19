/**
 *
 *  Copyright 2024-2026 NXP
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

#ifndef DEFAULT_EVENT_HANDLER_GEN_H
#define DEFAULT_EVENT_HANDLER_GEN_H

#include "IEventHandler.h"
#include <cstdint>

/** \addtogroup DEFAULT_EVENT_HANDLER_API_INTERFACE
 *  @brief  interface to perform the default event handler functionality.
 *  @{
 */

class DefaultEventHandler : public IEventHandler {
public:
  /**
   * @brief handles the vendor NCI message
   * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled by extension library otherwise
   * WLCSTATUS_EXTN_FEATURE_FAILURE.
   *
   */
  WLCSTATUS handleVendorNciMessage(uint16_t dataLen,
                                   const uint8_t *pData) override;

  /**
   * @brief handles the vendor NCI response and notification
   * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
   * feature and handled by extension library otherwise
   * WLCSTATUS_EXTN_FEATURE_FAILURE.
   *
   */
  WLCSTATUS handleVendorNciRspNtf(uint16_t dataLen, uint8_t *pData) override;

private:
};
/** @}*/
#endif // DEFAULT_EVENT_HANDLER_GEN_H
