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

#ifndef WLC_EXTENSION_API_H
#define WLC_EXTENSION_API_H

#include "WlcExtensionConstants.h"
#include "phWlcTypes.h"
#include <cstdint>
#include <NfcExtension.h>
/** \addtogroup WLC_EXTENSION_API_INTERFACE
 *  @brief  interface to perform the NXP WLC functionality.
 *  @{
 */
extern "C" {

/**
 * @brief Init the extension library with stack and data callbacks.
 * @return true in case of success.
 *         false in case of failure.
 *
 */
bool vendor_wlc_init(VendorExtnCb *vendorExtnCb);

/**
 * @brief De init the extension library
 *        and resets all it's states
 * @return true in case of success.
 *         false in case of failure.
 *
 */
bool vendor_wlc_de_init();

/**
 * @brief get wlc vendor extention control block
 * @return void
 *
 */
VendorExtnCb *getWlcVendorExtnCb();

/**
 * @brief Handles the wlc event with extenstion feature support
 * @param eventCode event code indicating the functionality
 * @param eventData pointer to data  of the event functionality
 * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
 * feature and it have to be handled by extension library. returns
 * WLCSTATUS_EXTN_FEATURE_FAILURE, if it have to be handled by WLC HAL.
 *
 */
WLCSTATUS vendor_wlc_handle_event(NfcExtEvent_t eventCode,
                                  NfcExtEventData_t *eventData);

/**
 * @brief Before sending the NCI packet to NFCC, phNxpExtn_Write is called
 * to check if there is any extension processing is required for the NCI packet
 * being sent out.
 * @param dataLen length of NCI command
 * @param pData NCI command
 * @return returns WLCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
 * feature and handled by extension library otherwise
 * WLCSTATUS_EXTN_FEATURE_FAILURE.
 */
WLCSTATUS phNxpExtn_Write(uint16_t dataLen, const uint8_t *pData);

/**
 * @brief This method will be invoked from libnfc-nci vendor extension
 * after reading all the conf defined in AIDL/Hidl Hal.
 * required conf values can be overridden
 * @param pConfigMap pointer to conf map with name and value as key pair.
 * @return void
 *
 */
void vendor_wlc_on_config_update(std::map<std::string, ConfigValue>* pConfigMap);
}
/** @}*/
#endif // WLC_EXTENSION_API_H
