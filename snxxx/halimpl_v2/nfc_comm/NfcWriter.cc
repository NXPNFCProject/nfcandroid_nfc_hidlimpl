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
#include "NfcWriter.h"
#include <phNfcNciConstants.h>
#include <phNxpConfig.h>
#include <phNxpNciHal.h>
#include <phNxpNciHal_ext.h>
#include <phNxpTempMgr.h>
#include <type_traits>
#include "NciDiscoveryCommandBuilder.h"
#include "NfcExtension.h"
#include "ObserveMode.h"
#include "phNxpNciHal_WiredSeIface.h"
#include "phNxpNciHal_extOperations.h"

#define MAX_NXP_HAL_EXTN_BYTES 10

bool bEnableMfcExtns = false;

/* NCI HAL Control structure */
extern phNxpNciHal_Control_t nxpncihal_ctrl;

/* Processing of ISO 15693 EOF */
extern uint8_t icode_send_eof;
extern uint8_t write_unlocked_status;

/* TML Context */
extern phTmlNfc_Context_t* gpphTmlNfc_Context;

extern WiredSeHandle* gWiredSeHandle;
extern void* RfFwRegionDnld_handle;

/******************************************************************************
 * Function         Default constructor
 *
 * Description      Default constructor of NfcWriter singleton class
 *
 ******************************************************************************/
NfcWriter::NfcWriter() {}

/******************************************************************************
 * Function         getInstance
 *
 * Description      This function is to provide the instance of NfcWriter
 *                  singleton class
 *
 * Returns          It returns instance of NfcWriter singleton class.
 *
 ******************************************************************************/
NfcWriter& NfcWriter::getInstance() {
  static NfcWriter instance;
  return instance;
}
/******************************************************************************
 * Function         write
 *
 * Description      This function write the data to NFCC through physical
 *                  interface (e.g. I2C) using the NFCC driver interface.
 *                  Before sending the data to NFCC, phNxpNciHal_write_ext
 *                  is called to check if there is any extension processing
 *                  is required for the NCI packet being sent out.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
int NfcWriter::write(uint16_t data_len, const uint8_t* p_data) {
  if (p_data[NCI_GID_INDEX] == NCI_RF_DISC_COMMD_GID &&
      p_data[NCI_OID_INDEX] == NCI_RF_DISC_COMMAND_OID) {
    NciDiscoveryCommandBuilderInstance.setDiscoveryCommand(data_len, p_data);
  } else if (p_data[NCI_GID_INDEX] == NCI_RF_DISC_COMMD_GID &&
             p_data[NCI_OID_INDEX] == NCI_RF_DEACTIVATE_OID) {
    NciDiscoveryCommandBuilderInstance.setRfDiscoveryReceived(false);
  }

  if (bEnableMfcExtns && p_data[NCI_GID_INDEX] == 0x00) {
    return NxpMfcReaderInstance.Write(data_len, p_data);
  } else if (phNxpNciHal_isVendorSpecificCommand(data_len, p_data)) {
    phNxpNciHal_print_packet("SEND", p_data, data_len,
                             RfFwRegionDnld_handle == NULL);
    return phNxpNciHal_handleVendorSpecificCommand(data_len, p_data);
  } else if (isObserveModeEnabled() &&
             p_data[NCI_GID_INDEX] == NCI_RF_DISC_COMMD_GID &&
             p_data[NCI_OID_INDEX] == NCI_RF_DISC_COMMAND_OID) {
    vector<uint8_t> v_data =
        NciDiscoveryCommandBuilderInstance.reConfigRFDiscCmd();
    uint16_t len = static_cast<uint16_t>(v_data.size());
    NFCSTATUS status = phNxpExtn_HandleNciMsg(&len, v_data.data());
    if (status != NFCSTATUS_EXTN_FEATURE_SUCCESS)
      return this->direct_write(v_data.size(), v_data.data());
    else
      return len;
  } else if (IS_HCI_PACKET(p_data)) {
    // Inform WiredSe service that HCI Pkt is sending from libnfc layer
    phNxpNciHal_WiredSeDispatchEvent(gWiredSeHandle, SENDING_HCI_PKT);
  } else if (IS_NFCEE_DISABLE(p_data)) {
    // NFCEE_MODE_SET(DISABLE) is called. Dispatch event to WiredSe so
    // that it can close if session is ongoing on same NFCEE
    phNxpNciHal_WiredSeDispatchEvent(
        gWiredSeHandle, DISABLING_NFCEE,
        createWiredSeEvtData((uint8_t*)p_data, data_len));
  } else {
    NFCSTATUS status = phNxpExtn_HandleNciMsg(&data_len, p_data);
    NXPLOG_NCIHAL_D("Vendor specific status: %d", status);
    if (status == NFCSTATUS_EXTN_FEATURE_SUCCESS) return data_len;
  }
  long value = 0;
  /* NXP Removal Detection timeout Config */
  if (GetNxpNumValue(NAME_NXP_REMOVAL_DETECTION_TIMEOUT, (void*)&value,
                     sizeof(value))) {
    // Change the timeout value as per config file
    uint8_t* wait_time = (uint8_t*)&p_data[3];
    if ((data_len == 0x04) && (p_data[0] == 0x21 && p_data[1] == 0x12)) {
      *wait_time = value;
    }
  }
  return this->direct_write(data_len, p_data);
}

/******************************************************************************
 * Function         direct_write
 *
 * Description      This function write the data to NFCC through physical
 *                  interface (e.g. I2C) using the NFCC driver interface.
 *                  Before sending the data to NFCC, phNxpNciHal_write_ext
 *                  is called to check if there is any extension processing
 *                  is required for the NCI packet being sent out.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
int NfcWriter::direct_write(uint16_t data_len, const uint8_t* p_data) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  int wdata_len = 0;
  uint8_t cmd_icode_eof[] = {0x00, 0x00, 0x00};
  static phLibNfc_Message_t msg;
  // Create an alias in local buffer as buffer may modified internally later
  uint8_t* p_data_updated = (uint8_t*)p_data;

  if (nxpncihal_ctrl.halStatus != HAL_STATUS_OPEN) {
    return NFCSTATUS_FAILED;
  }
  if ((data_len + MAX_NXP_HAL_EXTN_BYTES) > NCI_MAX_DATA_LEN) {
    NXPLOG_NCIHAL_D("cmd_len exceeds limit NCI_MAX_DATA_LEN");
    android_errorWriteLog(0x534e4554, "121267042");
    goto clean_and_return;
  }
  CONCURRENCY_LOCK();
  /* Check for NXP ext before sending write */
  status =
      phNxpNciHal_write_ext(&data_len, (uint8_t**)&p_data_updated,
                            &nxpncihal_ctrl.rsp_len, nxpncihal_ctrl.p_rsp_data);
  if (status != NFCSTATUS_SUCCESS) {
    /* Do not send packet to NFCC, send response directly */
    msg.eMsgType = NCI_HAL_RX_MSG;
    msg.pMsgData = NULL;
    msg.Size = 0;

    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackThreadId,
                          (phLibNfc_Message_t*)&msg);
    NXPLOG_NCIHAL_E("NXP ext check failed 0x%x", status);
    goto clean_and_return;
  }

  wdata_len = this->write_unlocked(data_len, p_data_updated, ORIG_LIBNFC);

  if (IS_CHIP_TYPE_L(sn100u) && IS_CHIP_TYPE_NE(pn557) && icode_send_eof == 1) {
    usleep(10000);
    icode_send_eof = 2;
    status = phNxpNciHal_send_ext_cmd(3, cmd_icode_eof);
    if (status != NFCSTATUS_SUCCESS) {
      NXPLOG_NCIHAL_E("ICODE end of frame command failed");
    }
  }

clean_and_return:
  /* No data written */
  CONCURRENCY_UNLOCK();
  // Free buffer if it is internally allocated
  if (p_data_updated != p_data) free(p_data_updated);
  return wdata_len;
}

/******************************************************************************
 * Function         write_unlocked
 *
 * Description      This is the actual function which is being called by
 *                  write. This function writes the data to NFCC.
 *                  It waits till write callback provide the result of write
 *                  process.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
int NfcWriter::write_unlocked(uint16_t data_len, const uint8_t* p_data,
                              int origin) {
  write_unlocked_status = NFCSTATUS_FAILED;
  /* check for write synchronyztion */
  if (this->check_ncicmd_write_window(data_len, (uint8_t*)p_data) !=
      NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_D("NfcWriter::write_unlocked  CMD window  check failed");
    return 0;
  }
  return write_window_checked_unlocked(data_len, p_data, origin);
}

/******************************************************************************
 * Function         write_window_checked_unlocked
 *
 * Description      Same as write_unlocked but without waiting for  command
 *                  window. It will be used whenever write is to be invoked
 *                  in HAL worker thread context to avoid blocking HAL worker
 *                  thread for command window on which previous responses
 *                  also needs to be processed.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
int NfcWriter::write_window_checked_unlocked(uint16_t data_len,
                                             const uint8_t* p_data,
                                             int origin) {
  NFCSTATUS status = NFCSTATUS_INVALID_PARAMETER;
  phNxpNciHal_Sem_t cb_data;
  nxpncihal_ctrl.retry_cnt = 0;
  int sem_val = 0;
  write_unlocked_status = NFCSTATUS_FAILED;

  if (origin == ORIG_NXPHAL) HAL_ENABLE_EXT();
  do {
    if (!phNxpTempMgr::GetInstance().IsICTempOk()) {
      phNxpTempMgr::GetInstance().Wait();
    }

    status = phTmlNfc_Write((uint8_t*)p_data, (uint16_t)data_len);
    if (status == NFCSTATUS_SUCCESS) {
      if (origin == ORIG_EXTNS &&
          p_data[NCI_GID_INDEX] == NCI_RF_DISC_COMMD_GID &&
          p_data[NCI_OID_INDEX] == NCI_RF_DISC_COMMAND_OID) {
        NciDiscoveryCommandBuilderInstance.setDiscoveryCommand(data_len,
                                                               p_data);
      }
      write_unlocked_status = NFCSTATUS_SUCCESS;
      break;
    }

    if (nxpncihal_ctrl.retry_cnt++ < MAX_RETRY_COUNT) {
      NXPLOG_NCIHAL_D(
          "write_unlocked failed - NFCC Maybe in Standby Mode - Retry");
    } else {
      data_len = 0;
      NXPLOG_NCIHAL_E(
          "write_unlocked failed - NFCC Maybe in Standby Mode (max count = "
          "0x%x)",
          nxpncihal_ctrl.retry_cnt);

      status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);

      if (NFCSTATUS_SUCCESS == status) {
        NXPLOG_NCIHAL_D("NFCC Reset - SUCCESS\n");
      } else {
        NXPLOG_NCIHAL_D("NFCC Reset - FAILED\n");
      }
      if (nxpncihal_ctrl.p_nfc_stack_data_cback != NULL &&
          nxpncihal_ctrl.halStatus != HAL_STATUS_CLOSE) {
        if (nxpncihal_ctrl.p_rx_data != NULL) {
          NXPLOG_NCIHAL_D("Doing abort which will trigger the recovery\n");
          // abort which will trigger the recovery.
          phNxpExtn_HandleHalEvent(NFCC_HAL_FATAL_ERR_CODE);
          abort();
        } else {
          (*nxpncihal_ctrl.p_nfc_stack_data_cback)(0x00, NULL);
        }
        write_unlocked_status = NFCSTATUS_FAILED;
      }
      break;
    }
  } while (true);

clean_and_return:
  if (write_unlocked_status == NFCSTATUS_FAILED) {
    sem_getvalue(&(nxpncihal_ctrl.syncSpiNfc), &sem_val);
    if (((p_data[0] & NCI_MT_MASK) == NCI_MT_CMD) && sem_val == 0) {
      sem_post(&(nxpncihal_ctrl.syncSpiNfc));
      NXPLOG_NCIHAL_D("HAL write  failed CMD window check releasing \n");
    }
  }
  return data_len;
}

/******************************************************************************
 * Function         check_ncicmd_write_window
 *
 * Description      This function is called to check the write synchroniztion
 *                  status if write already acquired then wait for corresponding
                    read to complete.
 *
 * Returns          return 0x00 on success and 0xFF on fail.
 *
 ******************************************************************************/

int NfcWriter::check_ncicmd_write_window(uint16_t cmd_len, uint8_t* p_cmd) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  int sem_timedout = 2, s;
  struct timespec ts;

  if (cmd_len < 1) {
    android_errorWriteLog(0x534e4554, "153880357");
    return NFCSTATUS_FAILED;
  }

  if ((p_cmd[0] & 0xF0) == 0x20) {
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += sem_timedout;
    while ((s = sem_timedwait_monotonic_np(&nxpncihal_ctrl.syncSpiNfc, &ts)) ==
               -1 &&
           errno == EINTR) {
      continue; /* Restart if interrupted by handler */
    }
    if (s != -1) {
      status = NFCSTATUS_SUCCESS;
    }
  } else {
    /* cmd window check not required for writing data packet */
    status = NFCSTATUS_SUCCESS;
  }
  return status;
}
