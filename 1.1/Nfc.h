#ifndef ANDROID_HARDWARE_NFC_V1_1_NFC_H
#define ANDROID_HARDWARE_NFC_V1_1_NFC_H

#include <android/hardware/nfc/1.1/INfc.h>
#include <android/hardware/nfc/1.1/types.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace nfc {
namespace V1_1 {
namespace implementation {

using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::nfc::V1_0::INfc;
using ::android::hardware::nfc::V1_0::INfcClientCallback;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Nfc : public V1_1::INfc, public hidl_death_recipient {
public:
    // Methods from ::android::hardware::nfc::V1_0::INfc follow.
    Return<V1_0::NfcStatus> open(const sp<INfcClientCallback>& clientCallback) override;
    Return<uint32_t> write(const hidl_vec<uint8_t>& data) override;
    Return<V1_0::NfcStatus> coreInitialized(const hidl_vec<uint8_t>& data) override;
    Return<V1_0::NfcStatus> prediscover() override;
    Return<V1_0::NfcStatus> close() override;
    Return<V1_0::NfcStatus> controlGranted() override;
    Return<V1_0::NfcStatus> powerCycle() override;

    // Methods from ::android::hardware::nfc::V1_1::INfc follow.
    Return<void> factoryReset();
    Return<V1_0::NfcStatus> closeForPowerOffCase();

    // Methods from ::android::hidl::base::V1_0::IBase follow.


    static void eventCallback(uint8_t event, uint8_t status) {
        if (mCallback != nullptr) {
            auto ret = mCallback->sendEvent((V1_0::NfcEvent)event,
                                            (V1_0::NfcStatus)status);
            if (!ret.isOk()) {
                ALOGW("failed to send event!!!");
            }
        }
    }

    static void dataCallback(uint16_t data_len, uint8_t* p_data) {
        hidl_vec<uint8_t> data;
        data.setToExternal(p_data, data_len);
        if (mCallback != nullptr) {
            auto ret = mCallback->sendData(data);
            if (!ret.isOk()) {
                ALOGW("failed to send data!!!");
            }
        }
    }

    virtual void serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/) {
        close();
    }

private:
    static sp<INfcClientCallback> mCallback;
};

// FIXME: most likely delete, this is only for passthrough implementations
 extern "C" INfc* HIDL_FETCH_INfc(const char* name);

}  // namespace implementation
}  // namespace V1_1
}  // namespace nfc
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_NFC_V1_1_NFC_H
