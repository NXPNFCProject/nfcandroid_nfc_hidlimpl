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

#ifndef NFC_EXTENSION_H
#define NFC_EXTENSION_H
#include <phTmlNfc.h>
#include <cstdint>
#include "Nxp_Features.h"
#include "phNfcStatus.h"

typedef struct {
  phLibNfc_DeferredCall_t tDeferredInfo;
  phLibNfc_Message_t tMsg;
  phTmlNfc_TransactInfo_t tTransactionInfo;
} NciDeferredData_t;

static NciDeferredData_t nciMsgDeferredData;
static NciDeferredData_t nciRspNtfDeferredData;

/**
 * @brief Holds NCI packet data length and data buffer
 *
 */
typedef struct {
  uint16_t* data_len;
  uint8_t* p_data;
} NciData_t;

/**
 * @brief Holds functional event datas to support
 *        extension features
 */

typedef union {
  NciData_t nci_msg;
  NciData_t nci_rsp_ntf;
  uint8_t write_status;
  uint8_t hal_state;
  uint8_t rf_state;
  uint8_t handle_event;
} NfcExtEventData_t;

/**
 * @brief Holds functional event codes to support
 *        extension features.
 */
typedef enum {
  HANDLE_VENDOR_NCI_MSG,
  HANDLE_VENDOR_NCI_RSP_NTF,
  HANDLE_WRITE_COMPLETE_STATUS,
  HANDLE_HAL_CONTROL_GRANTED,
  HANDLE_NFC_HAL_STATE_UPDATE,
  HANDLE_RF_HAL_STATE_UPDATE,
  HANDLE_EVENT,
  HANDLE_WRITE_EXTN_MSG,
} NfcExtEvent_t;

typedef void (*fp_extn_init_t)();
typedef void (*fp_extn_deinit_t)();
typedef NFCSTATUS (*fp_extn_handle_nfc_event_t)(NfcExtEvent_t,
                                                NfcExtEventData_t);

/**
 * @brief This function sets up and initialize the extension feature
 * @return void
 *
 */
void phNxpExtn_LibSetup();

/**
 * @brief This function de-initializes the extension feature
 * @return void
 *
 */
void phNxpExtn_LibClose();

/**
 * @brief updates the RF state
 * @param  state represents RF state of NFC
 * @return void
 *
 */
void phNxpExtn_NfcRfStateUpdate(uint8_t state);

/**
 * @brief updates the NFC HAL state
 * @param  state represents HAL state of NFC
 * @return void
 *
 */
void phNxpExtn_NfcHalStateUpdate(uint8_t state);

/**
 * @brief Adds the write message to queue and
 *        worker thread sends it to controller
 * @param  pBuffer pointer to write buffer
 * @param  wLength length of the write buffer
 * @return void
 *
 */
NFCSTATUS phNxpHal_EnqueueWrite(uint8_t* pBuffer, uint16_t wLength);

/**
 * @brief Adds the resopnse message to queue and
 *        worker thread sends it to controller
 * @param  pBuffer pointer to write buffer
 * @param  wLength length of the write buffer
 * @return void
 *
 */
NFCSTATUS phNxpHal_EnqueueRsp(uint8_t* pBuffer, uint16_t wLength);

/**
 * @brief updates the HAL control granted event
 *        to extension library
 * \note  called by Nfc HAL to notify HAL control
 *        granted event
 * @return void
 *
 */
void phNxpExtn_NfcHalControlGranted();

/**
 * @brief updates the NCI packet write completion status
 * @param status indicates NFCSTATUS_SUCCESS, if NCI packet
 *        written to controller sucessfully otherwise
 *        returns NFCSTATUS_FAILED
 * @return void
 *
 */
void phNxpExtn_WriteCompleteStatusUpdate(NFCSTATUS status);

/**
 * @brief sends the NCI packet to handle extension feature
 * @param  dataLen length of the NCI packet
 * @param  pData data buffer pointer
 * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
 * feature and handled by extension library otherwise
 * NFCSTATUS_EXTN_FEATURE_FAILURE.
 *
 */
NFCSTATUS phNxpExtn_HandleNciMsg(uint16_t dataLen, const uint8_t* pData);

/**
 * @brief sends the NCI packet to handle extension feature
 * @param  dataLen length of the NCI packet
 * @param  pData data buffer pointer
 * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
 * feature and handled by extension library otherwise
 * NFCSTATUS_EXTN_FEATURE_FAILURE.
 * \Note Handler implements this function is responsible to
 * stop the response timer.
 *
 */
NFCSTATUS phNxpExtn_HandleNciRspNtf(uint16_t dataLen, const uint8_t* pData);

/**
 * @brief sends the NCI packet to handle extension feature and update NCI
 * packet if feature is enabled.
 * @param dataLen Length of NCI packet
 * @param pData data buffer pointer
 * @return returns NFCSTATUS_EXTN_FEATURE_SUCCESS, if it is vendor specific
 * feature and handled by extension library otherwise
 * NFCSTATUS_EXTN_FEATURE_FAILURE.
 */
NFCSTATUS phNxpExtn_WriteExt(uint16_t* dataLen, uint8_t* pData);

/**
 * @brief  requests control of NFCC to libnfc-nci.
 *         When control is provided to HAL it is
 *         notified through phNxpExtn_NfcHalControlGranted.
 * @return void
 *
 */
void phNxpHal_RequestControl();

/**
 * @brief releases control of NFCC to libnfc-nci.
 *
 * @return void
 *
 */
void phNxpHal_ReleaseControl();

/**
 * @brief this function to send the NCI packet to upper layer
 * @param  data_len length of the NCI packet
 * @param  p_data data buffer pointer
 * @return void
 *
 */
void phNxpHal_NfcDataCallback(uint16_t dataLen, const uint8_t* pData);

/**
 * @brief Read byte array value from the config file.
 * @param name name of the config param to read.
 * @param pValue pointer to input buffer.
 * @param bufflen input buffer length.
 * @param len out parameter to return the number of bytes read from
 *            config file, return -1 in case bufflen is not enough.
 * @return 1 if config param name is found in the config file, else 0
 *
 */
uint8_t phNxpHal_GetNxpByteArrayValue(const char* name, char* pValue,
                                      long bufflen, long* len);

/**
 * @brief API function for getting a numerical value of a setting
 * @param name - name of the config param to read.
 * @param pValue - pointer to input buffer.
 * @param len - sizeof required pValue
 * @return 1 if config param name is found in the config file, else 0
 */
uint8_t phNxpHal_GetNxpNumValue(const char* name, void* pValue,
                                unsigned long len);

/**
 * @brief this function returns the chip type
 * @param  void
 * @return return the chip type version
 *
 */
tNFC_chipType phNxpHal_GetChipType();

#endif  // NFC_EXTENSION_H
