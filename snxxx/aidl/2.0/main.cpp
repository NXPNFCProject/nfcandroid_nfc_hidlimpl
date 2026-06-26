/******************************************************************************
 *
 *  Copyright 2022, 2024-2025 NXP
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
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <thread>

#include "Nfc.h"
#include "NxpNfc.h"
#include "phNxpNciHal_Adaptation.h"
#include "phNxpNciHal_Recovery.h"
#include "phNxpNciHal_WiredSeIface.h"

using ::aidl::android::hardware::nfc::Nfc;
using ::aidl::vendor::nxp::nxpnfc_aidl::INxpNfc;
using ::aidl::vendor::nxp::nxpnfc_aidl::NxpNfc;
using namespace std;

extern WiredSeHandle* gWiredSeHandle;

void startNxpNfcAidlService() {
  ALOGI("NXP NFC Extn Service is starting.");
  unsigned long dynamicHal = 0;
  if (!GetNxpNumValue("NFC_DYNAMIC_HAL", &dynamicHal, sizeof(dynamicHal))) {
    NXPLOG_NCIHAL_D("NFC_DYNAMIC_HAL not found in config. Using default value");
  }
  if (gWiredSeHandle != NULL && dynamicHal == 1) {
    NXPLOG_NCIHAL_D("WiredSe support is enabled, Force disabling dynamic Hal");
    dynamicHal = 0;
  }
  std::shared_ptr<NxpNfc> nxp_nfc_service = ndk::SharedRefBase::make<NxpNfc>();
  const std::string nxpNfcInstName =
      std::string() + NxpNfc::descriptor + "/default";
  binder_status_t status = STATUS_OK;
  if (dynamicHal == 1) {
    ALOGI("NxpNfc Registering Lazy service: %s", nxpNfcInstName.c_str());
    status = AServiceManager_registerLazyService(
        nxp_nfc_service->asBinder().get(), nxpNfcInstName.c_str());
  } else {
    ALOGI("NxpNfc Registering service: %s", nxpNfcInstName.c_str());
    status = AServiceManager_addService(nxp_nfc_service->asBinder().get(),
                                        nxpNfcInstName.c_str());
  }
  ALOGI("NxpNfc Registered INxpNfc service status: %d", status);
  CHECK(status == STATUS_OK);
  ABinderProcess_joinThreadPool();
}

int main() {
  ALOGI("NFC AIDL HAL starting up");
  if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
    ALOGE("failed to set thread pool max thread count");
    return 1;
  }
  // Starts Wired SE HAL instance if platform supports
  gWiredSeHandle = phNxpNciHal_WiredSeStart();
  if (gWiredSeHandle == NULL) {
    ALOGE("Wired Se HAL Disabled");
  }
  unsigned long dynamicHal = 0;
  if (!GetNxpNumValue("NFC_DYNAMIC_HAL", &dynamicHal, sizeof(dynamicHal))) {
    NXPLOG_NCIHAL_D("NFC_DYNAMIC_HAL not found in config. Using default value");
  }
  if (gWiredSeHandle != NULL && dynamicHal == 1) {
    NXPLOG_NCIHAL_D("WiredSe support is enabled, Force disabling dynamic Hal");
    dynamicHal = 0;
  }
  std::shared_ptr<Nfc> nfc_service = ndk::SharedRefBase::make<Nfc>();

  const std::string nfcInstName = std::string() + Nfc::descriptor + "/default";
  binder_status_t status = STATUS_OK;
  if (dynamicHal == 1) {
    ALOGI("Nfc Registering Lazy service: %s", nfcInstName.c_str());
    status = AServiceManager_registerLazyService(nfc_service->asBinder().get(),
                                                 nfcInstName.c_str());
  } else {
    ALOGI("Nfc Registering service: %s", nfcInstName.c_str());
    status = AServiceManager_addService(nfc_service->asBinder().get(),
                                        nfcInstName.c_str());
  }
  CHECK(status == STATUS_OK);

#if (NXP_NFC_RECOVERY == TRUE)
  phNxpNciHal_RecoverFWTearDown();
#endif
  thread t1(startNxpNfcAidlService);
  ABinderProcess_joinThreadPool();
  return 0;
}
