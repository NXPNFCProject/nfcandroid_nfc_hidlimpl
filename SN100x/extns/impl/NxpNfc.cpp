/******************************************************************************
 *
 *  Copyright 2018-2019 NXP
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

#include "NxpNfc.h"
#include "phNxpNciHal_Adaptation.h"
#include "hal_nxpnfc.h"
#include "eSEClientIntf.h"
#include "phNxpNciHal.h"
#include "eSEClientExtns.h"

extern bool nfc_debug_enabled;

namespace vendor {
namespace nxp {
namespace nxpnfc {
namespace V1_0 {
namespace implementation {

// Methods from ::vendor::nxp::nxpnfc::V1_0::INxpNfc follow.
Return<void> NxpNfc::ioctl(uint64_t ioctlType, const hidl_vec<uint8_t>& inOutData, ioctl_cb _hidl_cb) {
    int status;
    nfc_nci_IoctlInOutData_t inpOutData;
    ALOGD("HAL ioctl enter......");
    nfc_nci_IoctlInOutData_t *pInOutData=(nfc_nci_IoctlInOutData_t*)&inOutData[0];

    /*data from proxy->stub is copied to local data which can be updated by
     * underlying HAL implementation since its an inout argument*/
    memcpy(&inpOutData,pInOutData,sizeof(nfc_nci_IoctlInOutData_t));
    if (ioctlType == HAL_NFC_IOCTL_SET_TRANSIT_CONFIG) {
      /*As transit configurations are appended at the end of
      nfc_nci_IoctlInOutData_t, Assign appropriate pointer to TransitConfig*/
      if (inpOutData.inp.data.transitConfig.len == 0) {
        inpOutData.inp.data.transitConfig.val = NULL;
      } else {
        inpOutData.inp.data.transitConfig.val =
            ((char *)pInOutData) + sizeof(nfc_nci_IoctlInOutData_t);
      }
    }
    status = phNxpNciHal_ioctl(ioctlType, &inpOutData);
    if(HAL_NFC_IOCTL_ESE_JCOP_DWNLD == ioctlType)
    {
      ALOGD("NxpNfc::ioctl == HAL_NFC_IOCTL_ESE_JCOP_DWNLD");
      if(pInOutData->inp.data.nciCmd.p_cmd[0] == ESE_JCOP_UPDATE_COMPLETED
      || pInOutData->inp.data.nciCmd.p_cmd[0] == ESE_LS_UPDATE_COMPLETED)
      {
        ALOGD("NxpNfc::ioctl state == ESE_UPDATE_COMPLETED");
        seteSEClientState(pInOutData->inp.data.nciCmd.p_cmd[0]);
        eSEClientUpdate_NFC_Thread();
      }
      if (pInOutData->inp.data.nciCmd.p_cmd[0] == ESE_UPDATE_COMPLETED) {
        status =
            phNxpNciHal_ioctl(HAL_NFC_IOCTL_ESE_UPDATE_COMPLETE, &inpOutData);
      }
    }
    else if(HAL_NFC_IOCTL_GET_ESE_UPDATE_STATE == ioctlType)
    {
      inpOutData.out.data.status =
          (getJcopUpdateRequired() | (getLsUpdateRequired() << 8));
    }
    /*copy data and additional fields indicating status of ioctl operation
     * and context of the caller. Then invoke the corresponding proxy callback*/
    inpOutData.out.ioctlType = ioctlType;
    inpOutData.out.context   = pInOutData->inp.context;
    inpOutData.out.result    = status;
    NfcData  outputData;
    outputData.setToExternal((uint8_t*)&inpOutData.out, sizeof(nfc_nci_ExtnOutputData_t));
    _hidl_cb(outputData);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace nxpnfc
}  // namespace nxp
}  // namespace vendor
