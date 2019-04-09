/******************************************************************************
 *
 *  Copyright 2018 NXP
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

#include "eSEClient.h"
#include "eSEClientIntf.h"
#include <cutils/log.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "hal_nxpese.h"
#include "phNxpNciHal.h"

se_extns_entry se_intf;


/***************************************************************************
**
** Function:        checkEseClientUpdate
**
** Description:     Check the initial condition
                    and interafce for eSE Client update for LS and JCOP download
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void checkEseClientUpdate()
{
  ALOGD("%s enter:  ", __func__);
  checkeSEClientRequired(ESE_INTF_SPI);
  se_intf.isJcopUpdateRequired = getJcopUpdateRequired();
  se_intf.isLSUpdateRequired = getLsUpdateRequired();
  se_intf.sJcopUpdateIntferface = getJcopUpdateIntf();
  se_intf.sLsUpdateIntferface = getLsUpdateIntf();

  if((se_intf.isJcopUpdateRequired && se_intf.sJcopUpdateIntferface)||
   (se_intf.isLSUpdateRequired && se_intf.sLsUpdateIntferface)) {
    seteSEClientState(ESE_UPDATE_STARTED);
  }
  else
  {
     se_intf.isJcopUpdateRequired = false;
     se_intf.isLSUpdateRequired = false;
     setJcopUpdateRequired(0);
     setLsUpdateRequired(0);
     ALOGD("%s LS and JCOP download not required ", __func__);
  }
}

/*******************************************************************************
**
** Function:        seteSEClientState
**
** Description:     Function to set the eSEUpdate state
**
** Returns:         SUCCESS of ok
**
*******************************************************************************/
void seteSEClientState(uint8_t state)
{
  ALOGE("%s: State = %d", __FUNCTION__, state);
  ese_update = (ese_update_state_t)state;
}
