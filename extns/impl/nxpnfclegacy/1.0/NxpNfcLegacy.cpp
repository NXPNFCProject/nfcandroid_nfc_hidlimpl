/******************************************************************************
 *
 *  Copyright 2020 NXP
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
#include <log/log.h>

#include "NxpNfcLegacy.h"
#include "phNxpNciHal.h"
#include <phTmlNfc.h>

extern bool nfc_debug_enabled;

namespace vendor {
namespace nxp {
namespace nxpnfclegacy {
namespace V1_0 {
namespace implementation {


Return<uint8_t>
NxpNfcLegacy::setEseState(NxpNfcHalEseState EseState) {
  NFCSTATUS status = NFCSTATUS_FAILED;
  ALOGD("NxpNfcLegacy::setEseState Entry");

  if(EseState == NxpNfcHalEseState::HAL_NFC_ESE_IDLE_MODE)
  {
    ALOGD("NxpNfcLegacy::setEseState state == HAL_NFC_ESE_IDLE_MODE");
    status = phNxpNciHal_setEseState(phNxpNciHalNfc_e_SetIdleMode);
  }
  if (EseState == NxpNfcHalEseState::HAL_NFC_ESE_WIRED_MODE) {
    ALOGD("NxpNfcLegacy::setEseState state == HAL_NFC_ESE_WIRED_MODE");
    status = phNxpNciHal_setEseState(phNxpNciHalNfc_e_SetWiredMode);
  }

  ALOGD("NxpNfcLegacy::setEseState Exit");
  return status;
}


}  // namespace implementation
}  // namespace V1_0
}  // namespace nxpnfclegacy
}  // namespace nxp
}  // namespace vendor
