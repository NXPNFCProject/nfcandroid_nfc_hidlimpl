/*
 * Copyright 2024-2025 NXP
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

#include "phNxpNciHal_WiredSeIface.h"

#include <dlfcn.h>
#include <phNxpNciHal.h>

#define TERMINAL_TYPE_ESE 0x01
#define TERMINAL_TYPE_EUICC 0x05
#define TERMINAL_TYPE_EUICC2 0x06

/*******************************************************************************
**
** Function         phNxpNciHal_WiredSeStart()
**
** Description      Starts wired-se HAL. This is the first Api to be invoked.
**                  Once it is started it will run throughout the process
**                  lifecycle.
**                  It is recommended to call from main() of service.
**
** Returns          Pointer to WiredSeHandle if WiredSe HAL is started.
**                  NULL otherwise
*******************************************************************************/

WiredSeHandle* phNxpNciHal_WiredSeStart() {
  WiredSeHandle* outHandle = (WiredSeHandle*)calloc(1, sizeof(WiredSeHandle));
  if (outHandle == NULL) {
    return NULL;
  }
  // Open WiredSe shared library
  NXPLOG_NCIHAL_D("Opening (/vendor/lib64/WiredSe.so)");
  outHandle->dlHandle = dlopen("/vendor/lib64/WiredSe.so", RTLD_NOW);
  if (outHandle->dlHandle == NULL) {
    NXPLOG_NCIHAL_E("Error : opening (/vendor/lib64/WiredSe.so) %s!!",
                    dlerror());
    free(outHandle);
    return NULL;
  }
  outHandle->start =
      (WiredSeStartFunc_t)dlsym(outHandle->dlHandle, "WiredSeService_Start");
  if (outHandle->start == NULL) {
    NXPLOG_NCIHAL_E("Error : Failed to find symbol WiredSeService_Start %s!!",
                    dlerror());
    dlclose(outHandle->dlHandle);
    free(outHandle);
    return NULL;
  }
  outHandle->dispatchEvent = (WiredSeDispatchEventFunc_t)dlsym(
      outHandle->dlHandle, "WiredSeService_DispatchEvent");
  if (outHandle->dispatchEvent == NULL) {
    NXPLOG_NCIHAL_E(
        "Error : Failed to find symbol WiredSeService_DispatchEvent "
        "%s!!",
        dlerror());
    dlclose(outHandle->dlHandle);
    free(outHandle);
    return NULL;
  }
  NXPLOG_NCIHAL_D("Opened (/vendor/lib64/WiredSe.so)");
  if (outHandle->start(&outHandle->pWiredSeService) != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("Failed to start WiredSeService");
    dlclose(outHandle->dlHandle);
    free(outHandle);
    return NULL;
  }
  return outHandle;
}

/*******************************************************************************
**
** Function         phNxpNciHal_WiredSeDispatchEvent()
**
** Description      Dispatch events to wired-se subsystem.
**
** Parameters       outHandle - WiredSe Handle
** Returns          NFCSTATUS_SUCCESS if success.
**                  NFCSTATUS_FAILURE otherwise
*******************************************************************************/
NFCSTATUS phNxpNciHal_WiredSeDispatchEvent(WiredSeHandle* inHandle,
                                           WiredSeEvtType evtType,
                                           WiredSeEvtData evtData) {
  if (inHandle == NULL || inHandle->dispatchEvent == NULL ||
      inHandle->pWiredSeService == NULL) {
    return NFCSTATUS_FAILED;
  }
  WiredSeEvt event;
  event.eventData = evtData;
  event.event = evtType;
  return inHandle->dispatchEvent(inHandle->pWiredSeService, event);
}
