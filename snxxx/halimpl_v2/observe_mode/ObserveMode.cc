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
#include <NxpNfcThreadMutex.h>
#include <ObserveMode.h>
#include <phNfcNciConstants.h>

#include <vector>

#include "NciDiscoveryCommandBuilder.h"
#include "NfcExtension.h"
#include "ReaderPollConfigParser.h"
#include "phNxpNciHal_extOperations.h"

using std::vector;

bool gWaitingForDiscRsp;
bool gWaitingForRfDeActivateRsp;
bool bIsObserveModeEnabled;
bool bIsObserveChangeInProgress;

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

void setObserveChangeInProgress(bool flag) {
  bIsObserveChangeInProgress = flag;
}

bool isObserveChangeInProgress() { return bIsObserveChangeInProgress; }
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
    // ObserveMode per tech will be set to 0x01/0x00 for observe mode old
    // command
    NciDiscoveryCommandBuilderInstance.setObserveModePerTech(
        p_data[NCI_MSG_INDEX_FEATURE_VALUE]);
    status = NCI_RSP_OK;
  }

  phNxpNciHal_vendorSpecificCallback(
      p_data[NCI_OID_INDEX], p_data[NCI_MSG_INDEX_FOR_FEATURE], {status});

  return p_data[NCI_MSG_LEN_INDEX];
}

/*******************************************************************************
 *
 * Function         deactivateRfDiscovery()
 *
 * Description      sends RF deactivate command
 *
 * Returns          It returns Rf deactivate status
 *
 ******************************************************************************/
NFCSTATUS deactivateRfDiscovery() {
  if (NciDiscoveryCommandBuilderInstance.isRfDiscoveryCommandReceived()) {
    uint8_t rf_deactivate_cmd[] = {0x21, 0x06, 0x01, 0x00};
    uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
    uint16_t rsp_len = 0;
    ReaderPollConfigParser::resetLastKnownValues();
    return phNxpNciHal_send_ext_cmd(sizeof(rf_deactivate_cmd),
                                    rf_deactivate_cmd, &rsp_len, rsp);
  } else {
    setObserveChangeInProgress(true);  // ObserveMode Recovery needed
    return NFCSTATUS_SUCCESS;
  }
}

/*******************************************************************************
 *
 * Function         sendRfDiscoveryCommand()
 *
 * Description      sends RF discovery command
 *
 * Parameters       isObserveModeEnable
 *                      - true to send discovery with field detect mode
 *                      - false to send default discovery command
 *
 * Returns          It returns Rf deactivate status
 *
 ******************************************************************************/
NFCSTATUS sendRfDiscoveryCommand(bool isObserveModeEnable) {
  if (NciDiscoveryCommandBuilderInstance.isRfDiscoveryCommandReceived()) {
    uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
    uint16_t rsp_len = 0;
    vector<uint8_t> discoveryCommand =
        isObserveModeEnable
            ? NciDiscoveryCommandBuilderInstance.reConfigRFDiscCmd()
            : NciDiscoveryCommandBuilderInstance.getDiscoveryCommand();
    return phNxpNciHal_send_ext_cmd(discoveryCommand.size(),
                                    &discoveryCommand[0], &rsp_len, rsp);
  } else {
    return NFCSTATUS_SUCCESS;
  }
}

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
void resetDiscovery() {
  NciDiscoveryCommandBuilderInstance.setRfDiscoveryReceived(true);
  if (isObserveChangeInProgress()) {
    NXPLOG_NCIHAL_D("%s reset discovery ", __func__);

    gWaitingForRfDeActivateRsp = true;
    uint8_t rf_deactivate_cmd[] = {0x21, 0x06, 0x01, 0x00};
    setObserveChangeInProgress(false);
    phNxpHal_EnqueueWrite(&rf_deactivate_cmd[0], sizeof(rf_deactivate_cmd));
  }
}

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
NFCSTATUS handleObserveModeRfStateRspNtf(uint16_t dataLen, uint8_t* pData) {
  if (gWaitingForRfDeActivateRsp && dataLen >= NCI_RSP_SIZE &&
      pData[NCI_GID_INDEX] == NCI_RF_DISC_RSP_GID &&
      pData[NCI_OID_INDEX] == NCI_RF_DEACTIVATE_OID) {
    gWaitingForRfDeActivateRsp = false;
    gWaitingForDiscRsp = true;
    vector<uint8_t> discoveryCommand =
        isObserveModeEnabled()
            ? NciDiscoveryCommandBuilderInstance.reConfigRFDiscCmd()
            : NciDiscoveryCommandBuilderInstance.getDiscoveryCommand();
    phNxpHal_EnqueueWrite(&discoveryCommand[0], discoveryCommand.size());

    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }
  if (gWaitingForDiscRsp && dataLen >= 2 &&
      pData[NCI_GID_INDEX] == NCI_RF_DISC_RSP_GID &&
      pData[NCI_OID_INDEX] == NCI_RF_DISC_COMMAND_OID) {
    gWaitingForDiscRsp = false;
    return NFCSTATUS_EXTN_FEATURE_SUCCESS;
  }

  return NFCSTATUS_EXTN_FEATURE_FAILURE;
}

/*******************************************************************************
 *
 * Function         handleObserveModeTechCommand()
 *
 * Description      This handles the ObserveMode command and enables the observe
 *                  Mode flag
 *
 * Returns          It returns number of bytes received.
 *
 ******************************************************************************/
int handleObserveModeTechCommand(uint16_t data_len, const uint8_t* p_data) {
  NFCSTATUS nciStatus = NFCSTATUS_FAILED;
  uint8_t status = NCI_RSP_FAIL;
  uint8_t techValue = p_data[NCI_MSG_INDEX_FEATURE_VALUE];
  uint8_t rsp[PHNCI_MAX_DATA_LEN] = {0};
  uint16_t rsp_len = 0;

  if (phNxpNciHal_isObserveModeSupported() &&
      (techValue & OBSERVE_MODE_TECH_COMMAND_SUPPORT_FLAG_FOR_ALL_TECH ||
       techValue == NCI_ANDROID_PASSIVE_OBSERVE_PARAM_DISABLE)) {
    bool flag =
        (techValue & OBSERVE_MODE_TECH_COMMAND_SUPPORT_FLAG_FOR_ALL_TECH)
            ? true
            : false;
    // send RF Deactivate command
    nciStatus = deactivateRfDiscovery();
    if (nciStatus == NFCSTATUS_SUCCESS) {
      if (flag && techValue != NciDiscoveryCommandBuilderInstance
                                   .getCurrentObserveModeTechValue()) {
        // send Observe Mode Tech command
        NciDiscoveryCommandBuilderInstance.setObserveModePerTech(techValue);

        nciStatus =
            phNxpNciHal_send_ext_cmd(data_len, (uint8_t*)p_data, &rsp_len, rsp);
        if (nciStatus != NFCSTATUS_SUCCESS) {
          NXPLOG_NCIHAL_E("%s ObserveMode tech command failed", __func__);
        }
      }

      // Send RF Discovery command
      NFCSTATUS rfDiscoveryStatus =
          sendRfDiscoveryCommand(nciStatus == NFCSTATUS_SUCCESS ? flag : false);

      if (rfDiscoveryStatus == NFCSTATUS_SUCCESS &&
          nciStatus == NFCSTATUS_SUCCESS) {
        setObserveModeFlag(flag);
        status = NCI_RSP_OK;
      } else if (rfDiscoveryStatus != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E(
            "%s Rf Discovery command failed, reset back to default discovery",
            __func__);
        // Recovery to fallback to default discovery when there is a failure
        nciStatus = deactivateRfDiscovery();
        if (nciStatus != NFCSTATUS_SUCCESS) {
          NXPLOG_NCIHAL_E("%s Rf Deactivate command failed on recovery",
                          __func__);
        }
        rfDiscoveryStatus = sendRfDiscoveryCommand(false);
        if (rfDiscoveryStatus != NFCSTATUS_SUCCESS) {
          NXPLOG_NCIHAL_E("%s Rf Discovery command failed on recovery",
                          __func__);
        }
      }
    } else {
      NXPLOG_NCIHAL_E("%s Rf Deactivate command failed", __func__);
    }
  } else {
    NXPLOG_NCIHAL_E(
        "%s ObserveMode feature or tech which is requested is not supported",
        __func__);
  }

  phNxpNciHal_vendorSpecificCallback(
      p_data[NCI_OID_INDEX], p_data[NCI_MSG_INDEX_FOR_FEATURE], {status});

  return p_data[NCI_MSG_LEN_INDEX];
}

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
int handleGetObserveModeStatus(uint16_t data_len, const uint8_t* p_data) {
  // 2F 0C 01 04 => ObserveMode Status Command length is 4 Bytes
  if (data_len < 4) {
    return 0;
  }
  vector<uint8_t> response;
  response.push_back(0x00);
  response.push_back(
      isObserveModeEnabled()
          ? NciDiscoveryCommandBuilderInstance.getCurrentObserveModeTechValue()
          : 0x00);
  phNxpNciHal_vendorSpecificCallback(p_data[NCI_OID_INDEX],
                                     p_data[NCI_MSG_INDEX_FOR_FEATURE],
                                     std::move(response));

  return p_data[NCI_MSG_LEN_INDEX];
}
