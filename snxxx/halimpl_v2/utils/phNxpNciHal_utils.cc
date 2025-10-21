/*
 *
 *  Copyright 2013-2025 NXP
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
#include <errno.h>
#include <log/log.h>
#include <phNfcNciConstants.h>
#include <phNxpLog.h>
#include <phNxpNciHal.h>
#include <phNxpNciHal_utils.h>
#include <pthread.h>

#include <iomanip>
#include <sstream>

#include "NfcExtension.h"
#include "ObserveMode.h"
#include "phNxpNciHal_extOperations.h"

#define ASCII_OFFSET_NUM 48
#define ASCII_OFFSET_CHAR 55

extern phNxpNciHal_Control_t nxpncihal_ctrl;
extern bool_t phNxpLog_isLxLoggingEnabled();
/*********************** Link list functions **********************************/

/*******************************************************************************
**
** Function         listInit
**
** Description      List initialization
**
** Returns          1, if list initialized, 0 otherwise
**
*******************************************************************************/
int listInit(struct listHead* pList) {
  pList->pFirst = NULL;
  if (pthread_mutex_init(&pList->mutex, NULL) != 0) {
    NXPLOG_NCIHAL_E("Mutex creation failed (errno=0x%08x)", errno);
    return 0;
  }

  return 1;
}

/*******************************************************************************
**
** Function         listDestroy
**
** Description      List destruction
**
** Returns          1, if list destroyed, 0 if failed
**
*******************************************************************************/
int listDestroy(struct listHead* pList) {
  int bListNotEmpty = 1;
  while (bListNotEmpty) {
    bListNotEmpty = listGetAndRemoveNext(pList, NULL);
  }

  if (pthread_mutex_destroy(&pList->mutex) == -1) {
    NXPLOG_NCIHAL_E("Mutex destruction failed (errno=0x%08x)", errno);
    return 0;
  }

  return 1;
}

/*******************************************************************************
**
** Function         listAdd
**
** Description      Add a node to the list
**
** Returns          1, if added, 0 if otherwise
**
*******************************************************************************/
int listAdd(struct listHead* pList, void* pData) {
  struct listNode* pNode;
  struct listNode* pLastNode;
  int result;

  /* Create node */
  pNode = (struct listNode*)malloc(sizeof(struct listNode));
  if (pNode == NULL) {
    result = 0;
    NXPLOG_NCIHAL_E("Failed to malloc");
    goto clean_and_return;
  }
  pNode->pData = pData;
  pNode->pNext = NULL;
  pthread_mutex_lock(&pList->mutex);

  /* Add the node to the list */
  if (pList->pFirst == NULL) {
    /* Set the node as the head */
    pList->pFirst = pNode;
  } else {
    /* Seek to the end of the list */
    pLastNode = pList->pFirst;
    while (pLastNode->pNext != NULL) {
      pLastNode = pLastNode->pNext;
    }

    /* Add the node to the current list */
    pLastNode->pNext = pNode;
  }

  result = 1;

clean_and_return:
  pthread_mutex_unlock(&pList->mutex);
  return result;
}

/*******************************************************************************
**
** Function         listRemove
**
** Description      Remove node from the list
**
** Returns          1, if removed, 0 if otherwise
**
*******************************************************************************/
int listRemove(struct listHead* pList, void* pData) {
  struct listNode* pNode;
  struct listNode* pRemovedNode;
  int result;

  pthread_mutex_lock(&pList->mutex);

  if (pList->pFirst == NULL) {
    /* Empty list */
    NXPLOG_NCIHAL_D("Failed to deallocate (list empty)");
    result = 0;
    goto clean_and_return;
  }

  pNode = pList->pFirst;
  if (pList->pFirst->pData == pData) {
    /* Get the removed node */
    pRemovedNode = pNode;

    /* Remove the first node */
    pList->pFirst = pList->pFirst->pNext;
  } else {
    while (pNode->pNext != NULL) {
      if (pNode->pNext->pData == pData) {
        /* Node found ! */
        break;
      }
      pNode = pNode->pNext;
    }

    if (pNode->pNext == NULL) {
      /* Node not found */
      result = 0;
      NXPLOG_NCIHAL_E("Failed to deallocate (not found %8p)", pData);
      goto clean_and_return;
    }

    /* Get the removed node */
    pRemovedNode = pNode->pNext;

    /* Remove the node from the list */
    pNode->pNext = pNode->pNext->pNext;
  }

  /* Deallocate the node */
  free(pRemovedNode);

  result = 1;

clean_and_return:
  pthread_mutex_unlock(&pList->mutex);
  return result;
}

/*******************************************************************************
**
** Function         listGetAndRemoveNext
**
** Description      Get next node on the list and remove it
**
** Returns          1, if successful, 0 if otherwise
**
*******************************************************************************/
int listGetAndRemoveNext(struct listHead* pList, void** ppData) {
  struct listNode* pNode;
  int result;

  pthread_mutex_lock(&pList->mutex);

  if (pList->pFirst == NULL) {
    /* Empty list */
    NXPLOG_NCIHAL_D("Failed to deallocate (list empty)");
    result = 0;
    goto clean_and_return;
  }

  /* Work on the first node */
  pNode = pList->pFirst;

  /* Return the data */
  if (ppData != NULL) {
    *ppData = pNode->pData;
  }

  /* Remove and deallocate the node */
  pList->pFirst = pNode->pNext;
  free(pNode);

  result = 1;

clean_and_return:
  listDump(pList);
  pthread_mutex_unlock(&pList->mutex);
  return result;
}

/*******************************************************************************
**
** Function         listDump
**
** Description      Dump list information
**
** Returns          None
**
*******************************************************************************/
void listDump(struct listHead* pList) {
  struct listNode* pNode = pList->pFirst;

  NXPLOG_NCIHAL_D("Node dump:");
  while (pNode != NULL) {
    NXPLOG_NCIHAL_D("- %8p (%8p)", pNode, pNode->pData);
    pNode = pNode->pNext;
  }

  return;
}

/* END Linked list source code */

/****************** Semaphore and mutex helper functions **********************/

static phNxpNciHal_Monitor_t* nxpncihal_monitor = NULL;

/*******************************************************************************
**
** Function         phNxpNciHal_init_monitor
**
** Description      Initialize the semaphore monitor
**
** Returns          Pointer to monitor, otherwise NULL if failed
**
*******************************************************************************/
phNxpNciHal_Monitor_t* phNxpNciHal_init_monitor(void) {
  NXPLOG_NCIHAL_D("Entering phNxpNciHal_init_monitor");

  if (nxpncihal_monitor == NULL) {
    nxpncihal_monitor =
        (phNxpNciHal_Monitor_t*)malloc(sizeof(phNxpNciHal_Monitor_t));
  }

  if (nxpncihal_monitor != NULL) {
    memset(nxpncihal_monitor, 0x00, sizeof(phNxpNciHal_Monitor_t));

    if (pthread_mutex_init(&nxpncihal_monitor->reentrance_mutex, NULL) != 0) {
      NXPLOG_NCIHAL_E("reentrance_mutex creation returned 0x%08x", errno);
      goto clean_and_return;
    }

    if (pthread_mutex_init(&nxpncihal_monitor->concurrency_mutex, NULL) != 0) {
      NXPLOG_NCIHAL_E("concurrency_mutex creation returned 0x%08x", errno);
      pthread_mutex_destroy(&nxpncihal_monitor->reentrance_mutex);
      goto clean_and_return;
    }

    if (listInit(&nxpncihal_monitor->sem_list) != 1) {
      NXPLOG_NCIHAL_E("Semaphore List creation failed");
      pthread_mutex_destroy(&nxpncihal_monitor->concurrency_mutex);
      pthread_mutex_destroy(&nxpncihal_monitor->reentrance_mutex);
      goto clean_and_return;
    }
  } else {
    NXPLOG_NCIHAL_E("nxphal_monitor creation failed");
    goto clean_and_return;
  }

  NXPLOG_NCIHAL_D("Returning with SUCCESS");

  return nxpncihal_monitor;

clean_and_return:
  NXPLOG_NCIHAL_D("Returning with FAILURE");

  if (nxpncihal_monitor != NULL) {
    free(nxpncihal_monitor);
    nxpncihal_monitor = NULL;
  }

  return NULL;
}

/*******************************************************************************
**
** Function         phNxpNciHal_cleanup_monitor
**
** Description      Clean up semaphore monitor
**
** Returns          None
**
*******************************************************************************/
void phNxpNciHal_cleanup_monitor(void) {
  if (nxpncihal_monitor != NULL) {
    pthread_mutex_destroy(&nxpncihal_monitor->concurrency_mutex);
    REENTRANCE_UNLOCK();
    pthread_mutex_destroy(&nxpncihal_monitor->reentrance_mutex);
    phNxpNciHal_releaseall_cb_data();
    listDestroy(&nxpncihal_monitor->sem_list);
  }

  free(nxpncihal_monitor);
  nxpncihal_monitor = NULL;

  return;
}

/*******************************************************************************
**
** Function         phNxpNciHal_get_monitor
**
** Description      Get monitor
**
** Returns          Pointer to monitor
**
*******************************************************************************/
phNxpNciHal_Monitor_t* phNxpNciHal_get_monitor(void) {
  if (nxpncihal_monitor == NULL) {
    NXPLOG_NCIHAL_E("nxpncihal_monitor is null");
  }
  return nxpncihal_monitor;
}

/* Initialize the callback data */
NFCSTATUS phNxpNciHal_init_cb_data(phNxpNciHal_Sem_t* pCallbackData,
                                   void* pContext) {
  /* Create semaphore */
  if (sem_init(&pCallbackData->sem, 0, 0) == -1) {
    NXPLOG_NCIHAL_E("Semaphore creation failed (errno=0x%08x)", errno);
    return NFCSTATUS_FAILED;
  }

  /* Set default status value */
  pCallbackData->status = NFCSTATUS_FAILED;

  /* Copy the context */
  pCallbackData->pContext = pContext;

  /* Add to active semaphore list */
  phNxpNciHal_Monitor_t* hal_monitor = phNxpNciHal_get_monitor();
  if (hal_monitor == NULL) {
    NXPLOG_NCIHAL_E("Failed to get the monitor");
    return NFCSTATUS_FAILED;
  }
  struct listHead* head = &hal_monitor->sem_list;
  if (head == NULL || listAdd(head, pCallbackData) != 1) {
    NXPLOG_NCIHAL_E("Failed to add the semaphore to the list");
    return NFCSTATUS_FAILED;
  }

  return NFCSTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         phNxpNciHal_cleanup_cb_data
**
** Description      Clean up callback data
**
** Returns          None
**
*******************************************************************************/
void phNxpNciHal_cleanup_cb_data(phNxpNciHal_Sem_t* pCallbackData) {
  /* Destroy semaphore */
  if (sem_destroy(&pCallbackData->sem)) {
    NXPLOG_NCIHAL_E(
        "phNxpNciHal_cleanup_cb_data: Failed to destroy semaphore "
        "(errno=0x%08x)",
        errno);
  }

  /* Remove from active semaphore list */
  phNxpNciHal_Monitor_t* hal_monitor = phNxpNciHal_get_monitor();
  if (hal_monitor == NULL) {
    NXPLOG_NCIHAL_E("Failed to get the monitor");
    return;
  }
  listHead* head = &hal_monitor->sem_list;
  if (head == NULL || listRemove(head, pCallbackData) != 1) {
    NXPLOG_NCIHAL_E(
        "phNxpNciHal_cleanup_cb_data: Failed to remove semaphore from the "
        "list");
  }

  return;
}

/*******************************************************************************
**
** Function         phNxpNciHal_releaseall_cb_data
**
** Description      Release all callback data
**
** Returns          None
**
*******************************************************************************/
void phNxpNciHal_releaseall_cb_data(void) {
  phNxpNciHal_Sem_t* pCallbackData;
  phNxpNciHal_Monitor_t* hal_monitor = phNxpNciHal_get_monitor();
  if (hal_monitor == NULL) {
    NXPLOG_NCIHAL_E("Failed to get the monitor");
    return;
  }
  listHead* head = &hal_monitor->sem_list;

  if (head == NULL) return;

  while (listGetAndRemoveNext(head, (void**)&pCallbackData)) {
    pCallbackData->status = NFCSTATUS_FAILED;
    sem_post(&pCallbackData->sem);
  }

  return;
}

/* END Semaphore and mutex helper functions */

/**************************** Other functions *********************************/

/*******************************************************************************
**
** Function         phNxpNciHal_print_packet
**
** Description      Print packet
**
** Returns          None
**
*******************************************************************************/
void phNxpNciHal_print_packet(const char* pString, const uint8_t* p_data,
                              uint16_t len, bool isNxpAvcNciPrint) {
  tNFC_printType printType = getPrintType(pString);
  if (printType == PRINT_UNKNOWN) return;  // logging is disabled
  uint32_t i;
  char* print_buffer = (char*)calloc((len * 3 + 1), sizeof(char));
  if (NULL != print_buffer) {
    for (i = 0; i < len; i++) {
      snprintf(&print_buffer[i * 2], 3, "%02X", p_data[i]);
    }
    switch (printType) {
      case PRINT_SEND: {
        if (isNxpAvcNciPrint) {
          NXPAVCLOG_NCIX_I("len = %3d > %s", len, print_buffer);
        } else {
          NXPLOG_NCIX_I("len = %3d > %s", len, print_buffer);
        }
        break;
      }
      case PRINT_RECV: {
#if (NXP_DEBUG_LOG == TRUE)
        if (isNxpAvcNciPrint) {
          NXPAVCLOG_NCIR_I("len = %3d > %s", len, print_buffer);
        } else {
          NXPLOG_NCIR_I("len = %3d > %s", len, print_buffer);
        }
        break;
#else
        if (!phNxpLog_isLxLoggingEnabled() && len >= NCI_MSG_LEN_INDEX &&
            p_data[NCI_GID_INDEX] == NCI_PROP_NTF_GID &&
            p_data[NCI_OID_INDEX] == NCI_PROP_LX_NTF_OID) {
          break;
        }
        if (isNxpAvcNciPrint) {
          NXPAVCLOG_NCIR_I("len = %3d > %s", len, print_buffer);
        } else {
          NXPLOG_NCIR_I("len = %3d > %s", len, print_buffer);
        }
        break;
#endif
      }
      case PRINT_DEBUG:
        NXPLOG_NCIHAL_D(" Debug Info > len = %3d > %s", len, print_buffer);
        break;
      default:
        // Nothing to do
        break;
    }
    free(print_buffer);
  } else {
    NXPLOG_NCIX_E("\nphNxpNciHal_print_packet:Failed to Allocate memory\n");
  }
  return;
}

/******************************************************************************
 * Function         phNxpNciHal_StringToHex
 *
 * Description      This function will convert string to hexadecimal
 *                  representation.
 * Parameters       str - string to be converted.
 *                  len - Length of passed string
 *                  hex - Output converted hex bytes.
 *
 * Returns          None.
 *
 ******************************************************************************/
void phNxpNciHal_StringToHex(const char* str, size_t len, char* hex) {
  if (str == NULL || hex == NULL || (len % 2) != 0) {
    return;
  }
  memset(hex, 0, (len / 2));
  // Convert from string to hexadecimal format
  for (size_t i = 0; i < len; i += 2) {
    uint8_t temp = 0x00;
    // 1st Nibble of byte
    if (str[i] >= '0' && str[i] <= '9') {
      temp = (char(str[i]) - ASCII_OFFSET_NUM) << 4;
    } else if (toupper(str[i]) >= 'A' && toupper(str[i]) <= 'F') {
      temp = (char(toupper(str[i])) - ASCII_OFFSET_CHAR) << 4;
    } else {
      return;
    }
    // 2nd Nibble of byte
    if (str[i + 1] >= '0' && str[i + 1] <= '9') {
      temp = temp | (char(str[i + 1]) - ASCII_OFFSET_NUM);
    } else if (toupper(str[i + 1]) >= 'A' && toupper(str[i + 1]) <= 'F') {
      temp = temp | (char(toupper(str[i + 1])) - ASCII_OFFSET_CHAR);
    } else {
      return;
    }
    hex[i / 2] = temp;
  }
  return;
}

/******************************************************************************
 * Function         phNxpNciHal_HexToString
 *
 * Description      This function will convert hexadecimal buffer to
 *                  string representation.
 * Parameters       hex - hex bytes to be converted to string.
 *                  len - Length of passed hex buffer.
 *
 * Returns          string output.
 *
 ******************************************************************************/
std::string phNxpNciHal_HexToString(const uint8_t* hex, size_t len) {
  std::stringstream ss;
  // Convert to character
  for (size_t i = 0; i < len; i++) {
    ss << std::setfill('0') << std::hex << std::uppercase << std::setw(2)
       << (0xFF & hex[i]);
  }
  return ss.str();
}

/*******************************************************************************
**
** Function         getPrintType
**
** Description      get Print packet type (TX or RX or Debug)
**
** Returns          tNFC_printType enum.
**
*******************************************************************************/
tNFC_printType getPrintType(const char* pString) {
  if ((0 == memcmp(pString, "SEND", 0x04)) &&
      (nfc_debug_enabled ||
       (gLog_level.ncix_log_level >= NXPLOG_LOG_INFO_LOGLEVEL))) {
    return PRINT_SEND;
  } else if ((0 == memcmp(pString, "RECV", 0x04)) &&
             (nfc_debug_enabled ||
              (gLog_level.ncir_log_level >= NXPLOG_LOG_INFO_LOGLEVEL))) {
    return PRINT_RECV;
  } else if ((0 == memcmp(pString, "DEBUG", 0x05)) &&
             (nfc_debug_enabled ||
              (gLog_level.hal_log_level >= NXPLOG_LOG_DEBUG_LOGLEVEL))) {
    return PRINT_DEBUG;
  }
  return PRINT_UNKNOWN;
}

/*******************************************************************************
**
** Function         phNxpNciHal_emergency_recovery
**
** Description      Abort the process in case of ESE_OVER_TEMP_ERROR, FW Assert,
**                  Watchdog Reset, Input Clock lost and unrecoverable error.
**                  Ignore the other status.
**
** Returns          None
**
*******************************************************************************/

void phNxpNciHal_emergency_recovery(uint8_t status) {
  NXPLOG_NCIHAL_D("%s: %d", __func__, status);

  switch (status) {
    case NCI2_0_CORE_RESET_TRIGGER_TYPE_OVER_TEMPERATURE:
    case CORE_RESET_TRIGGER_TYPE_WATCHDOG_RESET:
    case CORE_RESET_TRIGGER_TYPE_INPUT_CLOCK_LOST:
    case CORE_RESET_TRIGGER_TYPE_UNRECOVERABLE_ERROR: {
      phNxpNciHal_decodeGpioStatus();
      NXPLOG_NCIHAL_E("abort()");
      phNxpExtn_HandleHalEvent(NFCC_HAL_FATAL_ERR_CODE);
      abort();
    }
    case CORE_RESET_TRIGGER_TYPE_FW_ASSERT: {
      phNxpExtn_HandleHalEvent(NFCC_HAL_ASSERT_ERR_CODE);
      phNxpNciHal_decodeGpioStatus();
      NXPLOG_NCIHAL_E("abort()");
      abort();
    } break;
    case CORE_RESET_TRIGGER_TYPE_POWERED_ON: {
      if (nxpncihal_ctrl.halStatus != HAL_STATUS_CLOSE &&
          nxpncihal_ctrl.power_reset_triggered == false) {
        phNxpNciHal_decodeGpioStatus();
        NXPLOG_NCIHAL_E("abort()");
        phNxpExtn_HandleHalEvent(NFCC_HAL_FATAL_ERR_CODE);
        abort();
      }
    } break;
    default:
      NXPLOG_NCIHAL_E("%s: Core reset with Invalid status : %d ", __func__,
                      status);
      break;
  }
}

/*******************************************************************************
**
** Function         phNxpNciHal_Memcpy
**
** Description      Copies the values stored in the source memory to the
**                  values stored in the destination memory only with source
**                  size.
**
** Returns          None
**
*******************************************************************************/
void phNxpNciHal_Memcpy(void* pDest, size_t destSize, const void* pSrc,
                        size_t srcSize) {
  NXPLOG_NCIHAL_D("%s Enter srcSize:%zu, destSize:%zu", __func__, srcSize,
                  destSize);
  if (srcSize > destSize) {
    srcSize = destSize;  // Truncate to avoid over flow
    NXPLOG_NCIHAL_E("%s Truncated length to avoid over flow ", __func__);
  }
  memcpy(pDest, pSrc, srcSize);
}
