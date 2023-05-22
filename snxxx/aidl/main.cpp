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

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <dlfcn.h>

#include <thread>

#include "Nfc.h"
#include "NxpNfc.h"
#include "phNxpNciHal_Adaptation.h"
#include "phNxpNciHal_Recovery.h"

using ::aidl::android::hardware::nfc::Nfc;
using ::aidl::vendor::nxp::nxpnfc_aidl::INxpNfc;
using ::aidl::vendor::nxp::nxpnfc_aidl::NxpNfc;
using namespace std;

typedef int (*NXPEsePreProcess)(void);

void startNxpNfcAidlService() {
  ALOGI("NXP NFC Extn Service is starting.");
  std::shared_ptr<NxpNfc> nxp_nfc_service = ndk::SharedRefBase::make<NxpNfc>();
  const std::string nxpNfcInstName =
      std::string() + NxpNfc::descriptor + "/default";
  ALOGI("NxpNfc Registering service: %s", nxpNfcInstName.c_str());
  binder_status_t status = AServiceManager_addService(
      nxp_nfc_service->asBinder().get(), nxpNfcInstName.c_str());
  ALOGI("NxpNfc Registered INxpNfc service status: %d", status);
  CHECK(status == STATUS_OK);
  ABinderProcess_joinThreadPool();
}

int main() {
  // Ignore this dlopen if you don't need it.
  std::string valueStr =
      android::base::GetProperty("persist.vendor.nfc.nxppreprocess", "Done");
  if (valueStr.compare("Done") != 0) {
    void* nxpdll = dlopen(valueStr.c_str(), RTLD_NOW);
    if (nxpdll) {
      NXPEsePreProcess fn = (NXPEsePreProcess)dlsym(nxpdll, "pre_process");
      if (fn)
        ALOGD("%s: Preprocess %s", __func__, fn() == 0 ? "Done" : "Error");
    }
  }

  ALOGI("NFC AIDL HAL starting up");
  if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
    ALOGE("failed to set thread pool max thread count");
    return 1;
  }
  std::shared_ptr<Nfc> nfc_service = ndk::SharedRefBase::make<Nfc>();

  const std::string nfcInstName = std::string() + Nfc::descriptor + "/default";
  binder_status_t status = AServiceManager_addService(
      nfc_service->asBinder().get(), nfcInstName.c_str());
  CHECK(status == STATUS_OK);

#if (NXP_NFC_RECOVERY == TRUE)
  phNxpNciHal_RecoverFWTearDown();
#endif
  thread t1(startNxpNfcAidlService);
  ABinderProcess_joinThreadPool();
  return 0;
}
