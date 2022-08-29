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

#include <fstream>

using namespace std;

class phNxpSMBLogger {
 public:
  // mark copy constructor deleted
  phNxpSMBLogger(const phNxpSMBLogger&) = delete;

  /**
   * Get signleton instance of SMBLogger.
   */
  static phNxpSMBLogger& getInstance();

  /**
   * Open SMB logger file.
   */
  void initialize();
  /**
   * Write SMB debug NTF to logfile.
   */
  void Log(uint8_t* p_ntf, uint16_t p_len);

  /**
   * Close SMB logger file.
   */
  void finalize();

 private:
  // constructor
  phNxpSMBLogger();
  bool logging_enabled;
  std::ofstream logFile;
  const char* SMB_LOG_FILEPATH = "/data/vendor/nfc/NxpNfcSmbLogDump.txt";
};
