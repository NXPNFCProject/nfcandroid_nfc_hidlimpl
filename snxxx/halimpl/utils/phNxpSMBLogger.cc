/*
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 ** http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 **
 ** Copyright 2022 NXP
 **
 */
#include "phNxpSMBLogger.h"
#include <fstream>
#include <iomanip>

#include <phNxpConfig.h>
#include <phNxpLog.h>

using namespace std;

phNxpSMBLogger::phNxpSMBLogger() { logging_enabled = false; }
phNxpSMBLogger& phNxpSMBLogger::getInstance() {
  static phNxpSMBLogger _nxpSMBLogger;
  return _nxpSMBLogger;
}

void phNxpSMBLogger::initialize() {
  NXPLOG_NCIHAL_D("SMBLogger: init");
  unsigned long value = 0;
  if (GetNxpNumValue(NAME_NXP_SMBLOG_ENABLED, &value, sizeof(value))) {
    logging_enabled = (value == 1) ? true : false;
  }
  if (!logging_enabled) return;

  logFile.open(SMB_LOG_FILEPATH,
               std::ofstream::out | std::ofstream::binary | std::ofstream::app);
  if (logFile.fail()) {
    NXPLOG_NCIHAL_D("SMBLogger: Log file couldn't be opened! %d", errno);
  }
}
void phNxpSMBLogger::Log(uint8_t* p_ntf, uint16_t p_len) {
  if (!logging_enabled) return;
  if (logFile.is_open()) {
    while (p_len) {
      logFile << std::setfill('0') << std::hex << std::uppercase << std::setw(2)
              << (0xFF & *p_ntf);
      ++p_ntf;
      --p_len;
    }
    logFile << endl;
  } else {
    NXPLOG_NCIHAL_D("SMBLogger: Log file is not opened");
  }
}

void phNxpSMBLogger::finalize() {
  if (!logging_enabled) return;
  NXPLOG_NCIHAL_D("SMBLogger: closing the Log file");
  logFile.close();
}
