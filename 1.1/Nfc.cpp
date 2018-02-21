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

#define LOG_TAG "android.hardware.nfc@1.1-impl"

#include <log/log.h>
#include "Nfc.h"
extern "C" {
#include "halimpl/inc/phNxpNciHal_Adaptation.h"
}

#define UNUSED(x) (void)(x)
#define HALSTATUS_SUCCESS (0x0000)
#define HALSTATUS_FAILED  (0x00FF)

typedef uint32_t HALSTATUS;

namespace android {
namespace hardware {
namespace nfc {
namespace V1_1 {
namespace implementation {

sp<INfcClientCallback> Nfc::mCallback = nullptr;

// Methods from ::android::hardware::nfc::V1_0::INfc follow.
Return<V1_0::NfcStatus> Nfc::open(const sp<INfcClientCallback>& clientCallback) {
    ALOGD("Nfc::open Enter");
    HALSTATUS status = HALSTATUS_FAILED;
    if (clientCallback == nullptr) {
        return V1_0::NfcStatus::FAILED;
    } else {
        mCallback = clientCallback;
        mCallback->linkToDeath(this, 0 /*cookie*/);
    }

    status = phNxpNciHal_open(eventCallback, dataCallback);
    ALOGD("Nfc::open Exit");
    if(status != HALSTATUS_SUCCESS) {
        return V1_0::NfcStatus::FAILED;
    } else {
        return V1_0::NfcStatus::OK;
    }
}

Return<uint32_t> Nfc::write(const hidl_vec<uint8_t>& data) {
    hidl_vec<uint8_t> copy = data;
    return phNxpNciHal_write(copy.size(), &copy[0]);
}

Return<V1_0::NfcStatus> Nfc::coreInitialized(const hidl_vec<uint8_t>& data) {
    HALSTATUS status = HALSTATUS_FAILED;
    hidl_vec<uint8_t> copy = data;
    status = phNxpNciHal_core_initialized(&copy[0]);
    if(status != HALSTATUS_SUCCESS) {
        return V1_0::NfcStatus::FAILED;
    } else {
        return V1_0::NfcStatus::OK;
    }
}

Return<V1_0::NfcStatus> Nfc::prediscover() {
    HALSTATUS status = HALSTATUS_FAILED;
    status = phNxpNciHal_pre_discover();
    return V1_0::NfcStatus {};
}

Return<V1_0::NfcStatus> Nfc::close() {
    HALSTATUS status = HALSTATUS_FAILED;

    if (mCallback == nullptr) {
        return V1_0::NfcStatus::FAILED;
    } else {
        mCallback->unlinkToDeath(this);
    }

    status = phNxpNciHal_close();
    if(status != HALSTATUS_SUCCESS) {
        return V1_0::NfcStatus::FAILED;
    } else {
        return V1_0::NfcStatus::OK;
    }
}

Return<V1_0::NfcStatus> Nfc::controlGranted() {
    HALSTATUS status = HALSTATUS_FAILED;
    status = phNxpNciHal_control_granted();
    if(status != HALSTATUS_SUCCESS) {
        return V1_0::NfcStatus::FAILED;
    } else {
        return V1_0::NfcStatus::OK;
    }
}

Return<V1_0::NfcStatus> Nfc::powerCycle() {
    HALSTATUS status = HALSTATUS_FAILED;
    status = phNxpNciHal_power_cycle();
    if(status != HALSTATUS_SUCCESS) {
        return V1_0::NfcStatus::FAILED;
    } else {
        return V1_0::NfcStatus::OK;
    }
}

// Methods from ::android::hardware::nfc::V1_1::INfc follow.
Return<void> Nfc::factoryReset() {
    phNxpNciHal_do_factory_reset();
    return Void();
}

Return<V1_0::NfcStatus> Nfc::closeForPowerOffCase() {
    HALSTATUS status = HALSTATUS_FAILED;
    status = phNxpNciHal_shutdown();
    if(status != HALSTATUS_SUCCESS) {
        return V1_0::NfcStatus::FAILED;
    } else {
        return V1_0::NfcStatus::OK;
    }
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace nfc
}  // namespace hardware
}  // namespace android
