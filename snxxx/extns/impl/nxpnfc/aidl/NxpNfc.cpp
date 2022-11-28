/******************************************************************************
 *
 *  Copyright 2022 NXP
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
#include "phNxpNciHal.h"
#include "phNxpNciHal_Adaptation.h"

extern bool nfc_debug_enabled;

namespace aidl {
namespace vendor {
namespace nxp {
namespace nxpnfc_aidl {

::ndk::ScopedAStatus NxpNfc::getVendorParam(const std::string& key, std::string* _aidl_return) {
  LOG(INFO) << "NxpNfc AIDL - getVendorParam";
  *_aidl_return = phNxpNciHal_getSystemProperty(key);
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus NxpNfc::setVendorParam(const std::string& key, const std::string& value, bool* _aidl_return) {
  LOG(INFO) << "NxpNfc AIDL - setVendorParam";
  *_aidl_return=phNxpNciHal_setSystemProperty(key, value);
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus NxpNfc::resetEse(int64_t resetType, bool* _aidl_return) {
  LOG(INFO) << "NxpNfc AIDL - resetEse";
  NFCSTATUS status = NFCSTATUS_FAILED;
  *_aidl_return = false;
  ALOGD("NxpNfc::resetEse Entry");

  status = phNxpNciHal_resetEse(resetType);
  if (NFCSTATUS_SUCCESS == status) {
    *_aidl_return = true;
    status = NFCSTATUS_SUCCESS;
    ALOGD("Reset request (%02x) completed", (uint8_t)resetType);
  } else {
    ALOGE("Reset request (%02x) failed", (uint8_t)resetType);
  }

  ALOGD("NxpNfc::resetEse Exit");
  return status == NFCSTATUS_SUCCESS ? ndk::ScopedAStatus::ok()
                    : ndk::ScopedAStatus::fromServiceSpecificError(
                              static_cast<int64_t>(status));
}

::ndk::ScopedAStatus NxpNfc::setEseUpdateState(NxpNfcHalEseState eSEState, bool* _aidl_return) {
  *_aidl_return = false;

  ALOGD("AIDL NxpNfc::setEseUpdateState Entry");

  if (eSEState == NxpNfcHalEseState::HAL_NFC_ESE_JCOP_UPDATE_COMPLETED ||
      eSEState == NxpNfcHalEseState::HAL_NFC_ESE_LS_UPDATE_COMPLETED) {
    ALOGD(
        "NxpNfc::setEseUpdateState state == HAL_NFC_ESE_JCOP_UPDATE_COMPLETED");
    seteSEClientState((uint8_t)eSEState);
    eSEClientUpdate_NFC_Thread();
  }
  if (eSEState == NxpNfcHalEseState::HAL_NFC_ESE_UPDATE_COMPLETED) {
    *_aidl_return = phNxpNciHal_Abort();
  }

  ALOGD("NxpNfc::setEseUpdateState Exit");
  return ndk::ScopedAStatus::fromServiceSpecificError(
                              static_cast<bool>(*_aidl_return));
}

::ndk::ScopedAStatus NxpNfc::setNxpTransitConfig( const std::string& strval,bool* _aidl_return) {
  *_aidl_return = true;
  ALOGD("NxpNfc::setNxpTransitConfig Entry");

  *_aidl_return = phNxpNciHal_setNxpTransitConfig((char*)strval.c_str());

  ALOGD("NxpNfc::setNxpTransitConfig Exit");
  return ndk::ScopedAStatus::fromServiceSpecificError(
                              static_cast<bool>(*_aidl_return));
}

::ndk::ScopedAStatus NxpNfc::isJcopUpdateRequired(bool* _aidl_return) {
  *_aidl_return = false;
  ALOGD("NxpNfc::isJcopUpdateRequired Entry");

#ifdef NXP_BOOTTIME_UPDATE
  *_aidl_return = getJcopUpdateRequired();
#endif

  ALOGD("NxpNfc::isJcopUpdateRequired Exit");
  return ndk::ScopedAStatus::fromServiceSpecificError(
                              static_cast<bool>(*_aidl_return));
}

::ndk::ScopedAStatus NxpNfc::isLsUpdateRequired(bool* _aidl_return) {
  *_aidl_return = false;
  ALOGD("NxpNfc::isLsUpdateRequired Entry");

#ifdef NXP_BOOTTIME_UPDATE
  *_aidl_return = getLsUpdateRequired();
#endif

  ALOGD("NxpNfc::isLsUpdateRequired Exit");
  return ndk::ScopedAStatus::fromServiceSpecificError(
                              static_cast<bool>(*_aidl_return));
}

}  // namespace nxpnfc
}  // namespace nxp
}  // namespace vendor
}  // namespace aidl
