/*
 * Copyright 2025 NXP
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

#pragma once

#include "phNfcStatus.h"

/**
 * @brief De init the extension library
 *        and resets all it's states
 * @return none.
 *
 */
void phNxpNfcExtn_deInit();

/**
 * @brief sends the NCI packet to handle extension feature
 * @param  dataLen length of the NCI packet
 * @param  pData data buffer pointer
 * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is Hal V2 specific
 * feature and handled by extension library otherwise
 * NFCSTATUS_EXTN_FEATURE_FAILURE.
 *
 */
NFCSTATUS phNxpNfcExtn_HandleNciMsg(uint16_t* dataLen, const uint8_t* pData);

/**
 * @brief sends the NCI packet to handle Hal V2 specific feature
 * @param  dataLen length of the NCI packet
 * @param  pData data buffer pointer
 * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is Hal V2 specific
 * feature and handled by Hal V2  library otherwise
 * NFCSTATUS_EXTN_FEATURE_FAILURE.
 * \Note Handler implements this function is responsible to
 * stop the response timer.
 *
 */
NFCSTATUS phNxpNfcExtn_HandleNciRspNtf(uint16_t* dataLen, const uint8_t* pData);

/**
 * @brief Default proprietary setting for updated for HAL V2 features.
 * @param  none
 *
 */
void phNxpNfcExtn_core_initialized();
