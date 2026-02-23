/*
 * Copyright 2024-2025 NXP
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
#include <phNfcStatus.h>

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
void setObserveModeFlag(bool flag);

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
bool isObserveModeEnabled();

/*******************************************************************************
 *
 * Function         setObserveChangeInProgress()
 *
 * Description      It sets the observe mode change in progress
 *
 * Parameters       bool - true to observe mode status change in progress
 *                         false if not
 *
 * Returns          void
 *
 ******************************************************************************/
void setObserveChangeInProgress(bool flag);

/*******************************************************************************
 *
 * Function         isObserveChangeInProgress()
 *
 * Description      returns true if observe mode status change in progress
 *                  otherwise false
 *
 * Returns          bool true if observe mode status change in progress
 *                  otherwise false
 *
 ******************************************************************************/
bool isObserveChangeInProgress();

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
int handleObserveMode(uint16_t data_len, const uint8_t* p_data);

/*******************************************************************************
 *
 * Function         handleObserveModeTechCommand
 *
 * Description      This handles the ObserveMode command and enables the observe
 *                  Mode flag
 *
 * Returns          It returns number of bytes received.
 *
 ******************************************************************************/
int handleObserveModeTechCommand(uint16_t data_len, const uint8_t* p_data);

/*******************************************************************************
 *
 * Function         handleGetObserveModeStatus()
 *
 * Description      Handles the Get Observe mode command and gives the observe
 *                  mode status
 *
 * Returns          It returns number of bytes received.
 *
 ******************************************************************************/
int handleGetObserveModeStatus(uint16_t data_len, const uint8_t* p_data);

/*******************************************************************************
 *
 * Function         resetDiscovery()
 *
 * Description      Resets RF discovery by sending deactivate command followed
 *                  by discovery command. This function handles observe mode
 *                  recovery by checking if observe mode change is in progress.
 *                  Uses synchronous communication with timeout mechanism.
 *
 * Parameters       None
 *
 * Returns          None
 *
 ******************************************************************************/
void resetDiscovery();

/*******************************************************************************
 *
 * Function         handleObserveModeRfStateRspNtf()
 *
 * Description      Handles RF state response and notification messages for
 *                  observe mode operations. Processes RF deactivate and
 *                  discovery responses to synchronize command execution.
 *
 * Parameters       dataLen - Length of the response data
 *                  pData   - Pointer to response data buffer
 *
 * Returns          NFCSTATUS_EXTN_FEATURE_SUCCESS if response is handled
 *                  successfully, otherwise appropriate error status
 *
 ******************************************************************************/
NFCSTATUS handleObserveModeRfStateRspNtf(uint16_t dataLen, uint8_t* pData);
