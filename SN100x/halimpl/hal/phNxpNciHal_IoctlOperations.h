/*
 * Copyright (C) 2019 NXP Semiconductors
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

#include "hal_nxpnfc.h"
#include "phNfcStatus.h"
#include "phNxpConfig.h"
#include "phNxpLog.h"
#include <hardware/nfc.h>

/******************************************************************************
 ** Function         phNxpNciHal_ioctlIf
 **
 ** Description      This function shall be called from HAL when libnfc-nci
 **                  calls phNxpNciHal_ioctl() to perform any IOCTL operation
 **
 ** Returns          return 0 on success and -1 on fail,
 ******************************************************************************/
int phNxpNciHal_ioctlIf(long arg, void *p_data);

/*******************************************************************************
**
** Function         phNxpNciHal_loadPersistLog
**
** Description      It shall be used to get persist log.
**
** Parameters       unit8_t index
**
** Returns          It returns persist log from the [index]
*******************************************************************************/
void phNxpNciHal_loadPersistLog(uint8_t index);

/*******************************************************************************
**
** Function         phNxpNciHal_savePersistLog
**
** Description      It shall be used to save persist log to the file[index].
**
** Parameters       unit8_t index
**
** Returns          void
*******************************************************************************/
void phNxpNciHal_savePersistLog(uint8_t index);

/*******************************************************************************
**
** Function         phNxpNciHal_getSystemProperty
**
** Description      It shall be used to get property vale of the Key
**
** Parameters       string key
**
** Returns          It returns the property value of the key
*******************************************************************************/
void phNxpNciHal_getSystemProperty(string key);

/*******************************************************************************
**
** Function         phNxpNciHal_setSystemProperty
**
** Description      It shall be used to save value to system property[key name]
**
** Parameters       string key, string value
**
** Returns          void
*******************************************************************************/
void phNxpNciHal_setSystemProperty(string key, string value);

/*******************************************************************************
**
** Function         phNxpNciHal_getNxpConfig
**
** Description      It shall be used to read config values from the
*libnfc-nxp.conf
**
** Parameters       nxpConfigs config
**
** Returns          void
*******************************************************************************/
void phNxpNciHal_getNxpConfigIf(nxp_nfc_config_t *configs);

/*******************************************************************************
**
** Function         phNxpNciHal_resetEse
**
** Description      It shall be used to to reset eSE by proprietary command.
**
** Parameters       None
**
** Returns          status of eSE reset response
*******************************************************************************/
NFCSTATUS phNxpNciHal_resetEse();

/******************************************************************************
** Function         phNxpNciHal_setNxpTransitConfig
**
** Description      This function overwrite libnfc-nxpTransit.conf file
**                  with transitConfValue.
**
** Returns          void.
**
*******************************************************************************/
void phNxpNciHal_setNxpTransitConfig(char *transitConfValue);

/*******************************************************************************
 **
 ** Function:        phNxpNciHal_CheckFwRegFlashRequired()
 **
 ** Description:     Updates FW and Reg configurations if required
 **
 ** Returns:         status
 **
 ********************************************************************************/
int phNxpNciHal_CheckFwRegFlashRequired(uint8_t *fw_update_req,
                                        uint8_t *rf_update_req,
                                        uint8_t skipEEPROMRead);
