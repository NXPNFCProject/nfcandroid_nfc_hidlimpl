/*
 * Copyright 2024 NXP
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
#include <ObserveMode.h>
#include <phNfcNciConstants.h>
#include "phNxpNciHal_extOperations.h"

bool bIsObserveModeEnabled;

/*******************************************************************************
 *
 * Function         setObserveModeFlag()
 *
 * Description      It sets the observe mode flag
 *
 * Parameters       bool - true to enable observe mode
 *                         false to disable observe mode
 *
 * Returns          void
 *
 ******************************************************************************/
void setObserveModeFlag(bool flag) { bIsObserveModeEnabled = flag; }

/*******************************************************************************
 *
 * Function         isObserveModeEnabled()
 *
 * Description      It gets the observe mode flag
 *
 * Returns          bool true if the observed mode is enabled
 *                  otherwise false
 *
 ******************************************************************************/
bool isObserveModeEnabled() { return bIsObserveModeEnabled; }

/*******************************************************************************
 *
 * Function         handleObserveMode()
 *
 * Description      This handles the ObserveMode command and enables the observe
 *                  Mode flag
 *
 * Returns          It returns number of bytes received.
 *
 ******************************************************************************/
int handleObserveMode(uint16_t data_len, const uint8_t* p_data) {
  if (data_len <= 4) {
    return 0;
  }

  uint8_t status = NCI_RSP_FAIL;
  if (phNxpNciHal_isObserveModeSupported()) {
    setObserveModeFlag(p_data[NCI_MSG_INDEX_FEATURE_VALUE]);
    status = NCI_RSP_OK;
  }

  phNxpNciHal_vendorSpecificCallback(p_data[NCI_OID_INDEX],
                                     p_data[NCI_MSG_INDEX_FOR_FEATURE], status);

  return p_data[NCI_MSG_LEN_INDEX];
}
