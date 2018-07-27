/*
 * Copyright (C) 2011, 2012 The Android Open Source Project
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
#define LOG_TAG "vendor.nxp.nxpnfc@1.0-impl"
#include "NxpNfc.h"
#include "hal_nxpese.h"
#include "phNxpNciHal_Adaptation.h"
#include <log/log.h>

namespace vendor {
namespace nxp {
namespace nxpnfc {
namespace V1_0 {
namespace implementation {

// Methods from ::vendor::nxp::nxpnfc::V1_0::INxpNfc follow.
Return<void> NxpNfc::ioctl(uint64_t ioctlType,
                           const hidl_vec<uint8_t>& inOutData,
                           ioctl_cb _hidl_cb) {
  ALOGD_IF(true, "%s: enter", __FUNCTION__);
  uint32_t status;
  nfc_nci_IoctlInOutData_t inpOutData;
  NfcData outputData;
  nfc_nci_IoctlInOutData_t* pInOutData =
      (nfc_nci_IoctlInOutData_t*)&inOutData[0];

  /*data from proxy->stub is copied to local data which can be updated by
   * underlying HAL implementation since its an inout argument*/
  memcpy(&inpOutData, pInOutData, sizeof(nfc_nci_IoctlInOutData_t));
  if (ioctlType == HAL_NFC_IOCTL_SET_TRANSIT_CONFIG) {
    /*As transit configurations are appended at the end of
    nfc_nci_IoctlInOutData_t, Assign appropriate pointer to TransitConfig*/
    inpOutData.inp.data.transitConfig.val =
        ((char *)pInOutData) + sizeof(nfc_nci_IoctlInOutData_t);
  }
  status = phNxpNciHal_ioctl(ioctlType, &inpOutData);
  /*copy data and additional fields indicating status of ioctl operation
   * and context of the caller. Then invoke the corresponding proxy callback*/
  inpOutData.out.ioctlType = ioctlType;
  inpOutData.out.context = pInOutData->inp.context;
  inpOutData.out.result = status;
  outputData.setToExternal((uint8_t*)&inpOutData.out,
                           sizeof(nfc_nci_ExtnOutputData_t));
  ALOGD_IF(true, "%s: exit", __FUNCTION__);
  _hidl_cb(outputData);
  return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace nxpnfc
}  // namespace nxp
}  // namespace vendor
