/*
 * Copyright 2024-2026 NXP
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
#include "phNxpLog.h"

WiredSeHandle gWiredSeHandle;

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
  NXPLOG_NCIHAL_D("%s: wired se not supoorted!");
  return NULL;
}