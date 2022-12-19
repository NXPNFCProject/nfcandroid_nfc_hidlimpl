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

#include "Nfc.h"

#include <android-base/logging.h>

#include "phNfcStatus.h"
#include "phNxpConfig.h"
#include "phNxpNciHal_Adaptation.h"
#include "phNxpNciHal_ext.h"
#include "phNxpNciHal_extOperations.h"

#define NXP_MAX_CONFIG_STRING_LEN 260

namespace aidl {
namespace android {
namespace hardware {
namespace nfc {

std::shared_ptr<INfcClientCallback> Nfc::mCallback = nullptr;
AIBinder_DeathRecipient* clientDeathRecipient = nullptr;

void OnDeath(void* cookie) {
    if (Nfc::mCallback != nullptr && !AIBinder_isAlive(Nfc::mCallback->asBinder().get())) {
        LOG(INFO) << __func__ << " Nfc service has died";
        Nfc* nfc = static_cast<Nfc*>(cookie);
        nfc->close(NfcCloseType::DISABLE);
    }
}

::ndk::ScopedAStatus Nfc::open(const std::shared_ptr<INfcClientCallback>& clientCallback) {
    LOG(INFO) << "Nfc::open";
    if (clientCallback == nullptr) {
        LOG(INFO) << "Nfc::open null callback";
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(NfcStatus::FAILED));
    }
    Nfc::mCallback = clientCallback;

    clientDeathRecipient = AIBinder_DeathRecipient_new(OnDeath);
    auto linkRet = AIBinder_linkToDeath(clientCallback->asBinder().get(), clientDeathRecipient,
                                        this /* cookie */);
    if (linkRet != STATUS_OK) {
        LOG(ERROR) << __func__ << ": linkToDeath failed: " << linkRet;
        // Just ignore the error.
    }

    printNfcMwVersion();
    int ret = phNxpNciHal_open(eventCallback, dataCallback);
    LOG(INFO) << "Nfc::open Exit";
    return ret == NFCSTATUS_SUCCESS ? ndk::ScopedAStatus::ok()
                    : ndk::ScopedAStatus::fromServiceSpecificError(
                              static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::close(NfcCloseType type) {
    LOG(INFO) << "Nfc::close";
    if (Nfc::mCallback == nullptr) {
        LOG(ERROR) << __func__ << "mCallback null";
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(NfcStatus::FAILED));
    }
    int ret = 0;
    if (type == NfcCloseType::HOST_SWITCHED_OFF) {
        ret = phNxpNciHal_close(true);
    } else {
        ret = phNxpNciHal_close(false);
    }
    Nfc::mCallback = nullptr;
    AIBinder_DeathRecipient_delete(clientDeathRecipient);
    clientDeathRecipient = nullptr;
    return ret == NFCSTATUS_SUCCESS ? ndk::ScopedAStatus::ok()
                    : ndk::ScopedAStatus::fromServiceSpecificError(
                              static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::coreInitialized() {
    LOG(INFO) << "Nfc::coreInitialized";
    if (Nfc::mCallback == nullptr) {
        LOG(ERROR) << __func__ << "mCallback null";
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(NfcStatus::FAILED));
    }
    int ret = phNxpNciHal_core_initialized();

    return ret == NFCSTATUS_SUCCESS ? ndk::ScopedAStatus::ok()
                    : ndk::ScopedAStatus::fromServiceSpecificError(
                              static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::factoryReset() {
    LOG(INFO) << "Nfc::factoryReset";
    phNxpNciHal_do_factory_reset();
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Nfc::getConfig(NfcConfig* _aidl_return) {
  LOG(INFO) << "Nfc::getConfig";
  NfcConfig config;
  unsigned long num = 0;
  std::array<uint8_t, NXP_MAX_CONFIG_STRING_LEN> buffer;
  buffer.fill(0);
  long retlen = 0;
  memset(&config, 0x00, sizeof(NfcConfig));
  phNxpNciHal_getExtVendorConfig();

  if (GetNxpNumValue(NAME_NFA_POLL_BAIL_OUT_MODE, &num, sizeof(num))) {
    config.nfaPollBailOutMode = (bool)num;
  }
  if (GetNxpNumValue(NAME_ISO_DEP_MAX_TRANSCEIVE, &num, sizeof(num))) {
    config.maxIsoDepTransceiveLength = (uint32_t)num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_OFFHOST_ROUTE, &num, sizeof(num))) {
    config.defaultOffHostRoute = (uint8_t)num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_NFCF_ROUTE, &num, sizeof(num))) {
    config.defaultOffHostRouteFelica = (uint8_t)num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_SYS_CODE_ROUTE, &num, sizeof(num))) {
    config.defaultSystemCodeRoute = (uint8_t)num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_SYS_CODE_PWR_STATE, &num, sizeof(num))) {
    config.defaultSystemCodePowerState =
        phNxpNciHal_updateAutonomousPwrState((uint8_t)num);
  }
  if (GetNxpNumValue(NAME_DEFAULT_ROUTE, &num, sizeof(num))) {
    config.defaultRoute = (uint8_t)num;
  }
  if (GetNxpByteArrayValue(NAME_DEVICE_HOST_ALLOW_LIST, (char*)buffer.data(),
                           buffer.size(), &retlen)) {
    config.hostAllowlist.resize(retlen);
    for (long i = 0; i < retlen; i++) config.hostAllowlist[i] = buffer[i];
  }
  if (GetNxpNumValue(NAME_OFF_HOST_ESE_PIPE_ID, &num, sizeof(num))) {
    config.offHostESEPipeId = (uint8_t)num;
  }
  if (GetNxpNumValue(NAME_OFF_HOST_SIM_PIPE_ID, &num, sizeof(num))) {
    config.offHostSIMPipeId = (uint8_t)num;
  }
  if (GetNxpNumValue(NAME_DEFAULT_ISODEP_ROUTE, &num, sizeof(num))) {
    config.defaultIsoDepRoute = (uint8_t)num;
  }
  if (GetNxpByteArrayValue(NAME_OFFHOST_ROUTE_UICC, (char*)buffer.data(),
                           buffer.size(), &retlen)) {
    config.offHostRouteUicc.resize(retlen);
    for (long i = 0; i < retlen; i++) config.offHostRouteUicc[i] = buffer[i];
  }

  if (GetNxpByteArrayValue(NAME_OFFHOST_ROUTE_ESE, (char*)buffer.data(),
                           buffer.size(), &retlen)) {
    config.offHostRouteEse.resize(retlen);
    for (long i = 0; i < retlen; i++) config.offHostRouteEse[i] = buffer[i];
  }
  if ((GetNxpByteArrayValue(NAME_NFA_PROPRIETARY_CFG, (char*)buffer.data(),
                            buffer.size(), &retlen)) &&
      (retlen == 9)) {
    config.nfaProprietaryCfg.protocol18092Active = (uint8_t)buffer[0];
    config.nfaProprietaryCfg.protocolBPrime = (uint8_t)buffer[1];
    config.nfaProprietaryCfg.protocolDual = (uint8_t)buffer[2];
    config.nfaProprietaryCfg.protocol15693 = (uint8_t)buffer[3];
    config.nfaProprietaryCfg.protocolKovio = (uint8_t)buffer[4];
    config.nfaProprietaryCfg.protocolMifare = (uint8_t)buffer[5];
    config.nfaProprietaryCfg.discoveryPollKovio = (uint8_t)buffer[6];
    config.nfaProprietaryCfg.discoveryPollBPrime = (uint8_t)buffer[7];
    config.nfaProprietaryCfg.discoveryListenBPrime = (uint8_t)buffer[8];
  } else {
    memset(&config.nfaProprietaryCfg, 0xFF, sizeof(ProtocolDiscoveryConfig));
  }
  if ((GetNxpNumValue(NAME_PRESENCE_CHECK_ALGORITHM, &num, sizeof(num))) &&
      (num <= 2)) {
    config.presenceCheckAlgorithm = (PresenceCheckAlgorithm)num;
  }

  *_aidl_return = config;
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Nfc::powerCycle() {
  LOG(INFO) << "powerCycle";
  if (Nfc::mCallback == nullptr) {
    LOG(ERROR) << __func__ << "mCallback null";
    return ndk::ScopedAStatus::fromServiceSpecificError(
        static_cast<int32_t>(NfcStatus::FAILED));
  }
  int ret = phNxpNciHal_power_cycle();
  return ret == NFCSTATUS_SUCCESS
             ? ndk::ScopedAStatus::ok()
             : ndk::ScopedAStatus::fromServiceSpecificError(
                   static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::preDiscover() {
  LOG(INFO) << "preDiscover";
  if (Nfc::mCallback == nullptr) {
    LOG(ERROR) << __func__ << "mCallback null";
    return ndk::ScopedAStatus::fromServiceSpecificError(
        static_cast<int32_t>(NfcStatus::FAILED));
  }
  int ret = phNxpNciHal_pre_discover();
  return ret == NFCSTATUS_SUCCESS
             ? ndk::ScopedAStatus::ok()
             : ndk::ScopedAStatus::fromServiceSpecificError(
                   static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::write(const std::vector<uint8_t>& data, int32_t* _aidl_return) {
    LOG(INFO) << "write";
    if (Nfc::mCallback == nullptr) {
        LOG(ERROR) << __func__ << "mCallback null";
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(NfcStatus::FAILED));
    }
    *_aidl_return = phNxpNciHal_write(data.size(), &data[0]);
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus Nfc::setEnableVerboseLogging(bool enable) {
    LOG(INFO) << "setVerboseLogging";
    phNxpNciHal_setVerboseLogging(enable);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Nfc::isVerboseLoggingEnabled(bool* _aidl_return) {
    *_aidl_return = phNxpNciHal_getVerboseLogging();
    return ndk::ScopedAStatus::ok();
}

}  // namespace nfc
}  // namespace hardware
}  // namespace android
}  // namespace aidl
