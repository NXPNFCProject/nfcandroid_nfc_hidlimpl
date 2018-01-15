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
    HALSTATUS status = HALSTATUS_FAILED;
    ALOGD("Manju OPen enter...........");
    if (clientCallback == nullptr) {
    ALOGD("call back null enter...........");
        return V1_0::NfcStatus::FAILED;
    } else {
    ALOGD("call back notnull enter...........");
        mCallback = clientCallback;
        mCallback->linkToDeath(this, 0 /*cookie*/);
    }

    ALOGD("Manju before halopen enter...........");
    status = phNxpNciHal_open(eventCallback, dataCallback);
    ALOGD("Manju after halopen enter...........");
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
    // TODO implement
    return Void();
}

Return<V1_0::NfcStatus> Nfc::closeForPowerOffCase() {
    // TODO implement
    return V1_0::NfcStatus {};
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

INfc* HIDL_FETCH_INfc(const char* /* name */) {
    return new Nfc();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace nfc
}  // namespace hardware
}  // namespace android
