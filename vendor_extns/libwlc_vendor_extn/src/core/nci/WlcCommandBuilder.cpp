/**
 *
 *  Copyright 2024-2026 NXP
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
 **/
#include "WlcCommandBuilder.h"
#include <phNxpLog.h>

//struct NciDiscoverMaps;
WlcCommandBuilder *WlcCommandBuilder::sNciCommandBuilder;

WlcCommandBuilder::WlcCommandBuilder() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s: enter", __func__);
}

WlcCommandBuilder::~WlcCommandBuilder() {
  NXPLOG_EXTNS_D(NXPLOG_ITEM_NXP_GEN_EXTN, "%s: enter", __func__);
}

WlcCommandBuilder *WlcCommandBuilder::getInstance() {
  if (sNciCommandBuilder == nullptr) {
    sNciCommandBuilder = new WlcCommandBuilder();
  }
  return sNciCommandBuilder;
}

/*uint8_t
WlcCommandBuilder::sendDiscoverMapCmd(uint8_t num,
                                      struct NciDiscoverMaps *discMaps) {
  return 1;
}*/
