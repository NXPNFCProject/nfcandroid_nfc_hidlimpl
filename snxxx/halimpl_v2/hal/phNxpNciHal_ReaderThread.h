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

#ifndef NXPNCIHALREADER_H
#define NXPNCIHALREADER_H

#include <pthread.h>

#include <atomic>
#include <cstdbool>

class phNxpNciHal_ReaderThread {
 public:
  static phNxpNciHal_ReaderThread& getInstance();

  phNxpNciHal_ReaderThread(const phNxpNciHal_ReaderThread&) = delete;
  phNxpNciHal_ReaderThread& operator=(const phNxpNciHal_ReaderThread&) = delete;

  /******************************************************************************
   * Function:       Start()
   *
   * Description:    This method creates & initiates the reader thread for
   *                 handling NFCC communication.
   *
   * Returns:        bool: True if the reader thread was successfully started
   *                 otherwise false.
   ******************************************************************************/
  bool Start();

  /******************************************************************************
   * Function:       Stop()
   *
   * Description:    This method stops the reader thread and clear the resources
   *
   * Returns:        bool: True if the reader thread was successfully stopped
   *                 otherwise false.
   ******************************************************************************/
  bool Stop();

 private:
  phNxpNciHal_ReaderThread();
  ~phNxpNciHal_ReaderThread();

  static void* ReaderThread(void* arg);
  void Run();
  pthread_t reader_thread;
  volatile std::atomic<bool> thread_running;
};
#endif  // NXPNCIHALREADER_H
