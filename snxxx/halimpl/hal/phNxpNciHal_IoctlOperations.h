/*
 * Copyright 2019-2024 NXP
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

#include <hardware/nfc.h>

#include "phNfcStatus.h"
#include "phNxpConfig.h"
#include "phNxpLog.h"

#define NCI_PACKET_LEN_INDEX 2
#define NCI_PACKET_TLV_INDEX 3
/*Below are NCI get config response index values for each RF register*/
#define DLMA_ID_TX_ENTRY_INDEX 12
#define RF_CM_TX_UNDERSHOOT_INDEX 5
#define PHONEOFF_TECH_DISABLE_INDEX 8
#define ISO_DEP_MERGE_SAK_INDEX 8
#define INITIAL_TX_PHASE_INDEX 8
#define LPDET_THRESHOLD_INDEX 11
#define NFCLD_THRESHOLD_INDEX 13
#define RF_PATTERN_CHK_INDEX 8
#define GUARD_TIMEOUT_TX2RX_INDEX 8
#define REG_A085_DATA_INDEX 8

/*Below are A085 RF register bitpostions*/
#define CN_TRANSIT_BLK_NUM_CHECK_ENABLE_BIT_POS 6
#define MIFARE_NACK_TO_RATS_ENABLE_BIT_POS 13
#define MIFARE_MUTE_TO_RATS_ENABLE_BIT_POS 9
#define CN_TRANSIT_CMA_BYPASSMODE_ENABLE_BIT_POS 23
#define CHINA_TIANJIN_RF_ENABLE_BIT_POS 28

#define NCI_GET_CMD_TLV_INDEX1 4
#define NCI_GET_CMD_TLV_INDEX2 5

/******************************************************************************
 ** Function         phNxpNciHal_ioctlIf
 **
 ** Description      This function shall be called from HAL when libnfc-nci
 **                  calls phNxpNciHal_ioctl() to perform any IOCTL operation
 **
 ** Returns          return 0 on success and -1 on fail,
 ******************************************************************************/
int phNxpNciHal_ioctlIf(long arg, void* p_data);

/*******************************************************************************
**
** Function         phNxpNciHal_getSystemProperty
**
** Description      It shall be used to get property value of the given Key
**
** Parameters       string key
**
** Returns          It returns the property value of the key
*******************************************************************************/
string phNxpNciHal_getSystemProperty(string key);

/*******************************************************************************
 **
 ** Function         phNxpNciHal_setSystemProperty
 **
 ** Description      It shall be used to save/chage value to system property
 **                  based on provided key.
 **
 ** Parameters       string key, string value
 **
 ** Returns          true if success, false if fail
 *******************************************************************************/
bool phNxpNciHal_setSystemProperty(string key, string value);

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
string phNxpNciHal_getNxpConfigIf();

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
NFCSTATUS phNxpNciHal_resetEse(uint64_t resetType);

/******************************************************************************
** Function         phNxpNciHal_setNxpTransitConfig
**
** Description      This function overwrite libnfc-nxpTransit.conf file
**                  with transitConfValue.
**
** Returns          bool.
**
*******************************************************************************/
bool phNxpNciHal_setNxpTransitConfig(char* transitConfValue);

/*******************************************************************************
 **
 ** Function:        phNxpNciHal_CheckFwRegFlashRequired()
 **
 ** Description:     Updates FW and Reg configurations if required
 **
 ** Returns:         status
 **
 ********************************************************************************/
int phNxpNciHal_CheckFwRegFlashRequired(uint8_t* fw_update_req,
                                        uint8_t* rf_update_req,
                                        uint8_t skipEEPROMRead);

/******************************************************************************
 * Function         phNxpNciHal_txNfccClockSetCmd
 *
 * Description      This function is called after successful download
 *                  to apply the clock setting provided in config file
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_txNfccClockSetCmd(void);

/*******************************************************************************
 **
 ** Function:        property_get_intf()
 **
 ** Description:     Gets property value for the input property name
 **
 ** Parameters       propName:   Name of the property whichs value need to get
 **                  valueStr:   output value of the property.
 **                  defaultStr: default value of the property if value is not
 **                              there this will be set to output value.
 **
 ** Returns:         actual length of the property value
 **
 ********************************************************************************/
int property_get_intf(const char* propName, char* valueStr,
                      const char* defaultStr);

/*******************************************************************************
 **
 ** Function:        property_set_intf()
 **
 ** Description:     Sets property value for the input property name
 **
 ** Parameters       propName:   Name of the property whichs value need to set
 **                  valueStr:   value of the property.
 **
 ** Returns:        returns 0 on success, < 0 on failure
 **
 ********************************************************************************/
int property_set_intf(const char* propName, const char* valueStr);

/*******************************************************************************
 **
 ** Function:        phNxpNciHal_GetNfcGpiosStatus()
 **
 ** Description:     Sets the gpios status flag byte
 **
 ** Parameters       gpiostatus: flag byte
 **
 ** Returns:        returns 0 on success, < 0 on failure
 **
 ********************************************************************************/
NFCSTATUS phNxpNciHal_GetNfcGpiosStatus(uint32_t* gpiosstatus);

/*******************************************************************************
 **
 ** Function:        phNxpNciHal_Abort()
 **
 ** Description:     This function shall be used to trigger the abort
 **
 ** Parameters       None
 **
 ** Returns:        returns 0 on success, < 0 on failure
 **
 ********************************************************************************/
bool phNxpNciHal_Abort();

#undef PROPERTY_VALUE_MAX
#define PROPERTY_VALUE_MAX 92
#define property_get(a, b, c) property_get_intf(a, b, c)
#define property_set(a, b) property_set_intf(a, b)
