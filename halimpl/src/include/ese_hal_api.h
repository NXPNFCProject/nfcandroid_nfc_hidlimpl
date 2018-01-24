/******************************************************************************
 *
 *  Copyright (C) 2012-2014 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
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
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
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

/******************************************************************************
 *
 *  ESE Hardware Abstraction Layer API
 *
 ******************************************************************************/
#ifndef ESE_HAL_API_H
#define ESE_HAL_API_H
//#include <hardware/ese.h>
#include "data_types.h"
//#include "ese_hal_target.h"
//#include "hal_nxpese.h"
/*******************************************************************************
** tHAL_HCI_NETWK_CMD Definitions
*******************************************************************************/
#define HAL_ESE_HCI_NO_UICC_HOST 0x00
#define HAL_ESE_HCI_UICC0_HOST 0x01
#define HAL_ESE_HCI_UICC1_HOST 0x02
#define HAL_ESE_HCI_UICC2_HOST 0x04
typedef uint8_t tHAL_ESE_STATUS;
typedef void(tHAL_ESE_STATUS_CBACK)(tHAL_ESE_STATUS status);
typedef void(tHAL_ESE_CBACK)(uint8_t event, tHAL_ESE_STATUS status);
typedef void(tHAL_ESE_DATA_CBACK)(uint16_t data_len, uint8_t* p_data);

/*******************************************************************************
** tHAL_ESE_ENTRY HAL entry-point lookup table
*******************************************************************************/

typedef void(tHAL_SPIAPI_OPEN)(tHAL_ESE_CBACK* p_hal_cback,
                            tHAL_ESE_DATA_CBACK* p_data_cback);
typedef void(tHAL_SPIAPI_CLOSE)(void);
typedef void(tHAL_SPIAPI_WRITE)(uint16_t data_len, uint8_t* p_data);
typedef void(tHAL_SPIAPI_READ)(uint16_t data_len, uint8_t* p_data);
typedef int(tHAL_SPIAPI_IOCTL)(long arg, void* p_data);

#define ESE_HAL_DM_PRE_SET_MEM_LEN 5
typedef struct {
  uint32_t addr;
  uint32_t data;
} tESE_HAL_DM_PRE_SET_MEM;

/* data members for ESE_HAL-HCI */
typedef struct {
  bool ese_hal_prm_nvm_required; /* set ese_hal_prm_nvm_required to true, if the
                                    platform wants to abort PRM process without
                                    NVM */
  uint16_t ese_hal_esec_enable_timeout; /* max time to wait for RESET NTF after
                                           setting REG_PU to high */
  uint16_t ese_hal_post_xtal_timeout;   /* max time to wait for RESET NTF after
                                           setting Xtal frequency */
#if (ESE_HAL_HCI_INCLUDED == true)
  bool ese_hal_first_boot; /* set ese_hal_first_boot to true, if platform
                              enables ESE for the first time after bootup */
  uint8_t ese_hal_hci_uicc_support; /* set ese_hal_hci_uicc_support to Zero, if
                                       no UICC is supported otherwise set
                                       corresponding bit(s) for every supported
                                       UICC(s) */
#endif
} tESE_HAL_CFG;

typedef struct {
  tHAL_SPIAPI_OPEN* open;
  tHAL_SPIAPI_CLOSE* close;
  tHAL_SPIAPI_WRITE* write;
  tHAL_SPIAPI_READ* Read;
  tHAL_SPIAPI_IOCTL* ioctl;
} tHAL_ESE_ENTRY;

  tHAL_ESE_ENTRY* p_hal;
typedef struct {
  tHAL_ESE_ENTRY* hal_entry_func;
  uint8_t boot_mode;
} tHAL_ESE_CONTEXT;
tHAL_ESE_ENTRY* getInstance();//RVM
/*******************************************************************************
** HAL API Function Prototypes
*******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
**
** Function         HAL_EseOpen
**
** Description      Open transport and intialize the ESEC, and
**                  Register callback for HAL event notifications,
**
**                  HAL_OPEN_CPLT_EVT will notify when operation is complete.
**
** Returns          void
**
*******************************************************************************/
void HAL_EseOpen(tHAL_ESE_CBACK* p_hal_cback,
                 tHAL_ESE_DATA_CBACK* p_data_cback);

/*******************************************************************************
**
** Function         HAL_EseClose
**
** Description      Prepare for shutdown. A HAL_CLOSE_CPLT_EVT will be
**                  reported when complete.
**
** Returns          void
**
*******************************************************************************/
void HAL_EseClose(void);

/*******************************************************************************
**
** Function         HAL_EseWrite
**
** Description      Send an NCI control message or data packet to the
**                  transport. If an NCI command message exceeds the transport
**                  size, HAL is responsible for fragmenting it, Data packets
**                  must be of the correct size.
**
** Returns          void
**
*******************************************************************************/
void HAL_EseWrite(uint16_t data_len, uint8_t* p_data);

/*******************************************************************************
**
** Function         HAL_EseRead
**
** Description      Send an NCI control message or data packet to the
**                  transport. If an NCI command message exceeds the transport
**                  size, HAL is responsible for fragmenting it, Data packets
**                  must be of the correct size.
**
** Returns          void
**
*******************************************************************************/
void HAL_EseRead(uint16_t data_len, uint8_t* p_data);

#ifdef __cplusplus
}
#endif

#endif /* ESE_HAL_API_H  */
