/*
 * Copyright 2024 NXP
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

#ifndef NXPNCIHALWORKER_H
#define NXPNCIHALWORKER_H

#include <pthread.h>
#include <atomic>
#include <cstdbool>

class phNxpNciHal_WorkerThread {
 public:
  static phNxpNciHal_WorkerThread& getInstance();

  phNxpNciHal_WorkerThread(const phNxpNciHal_WorkerThread&) = delete;
  phNxpNciHal_WorkerThread& operator=(const phNxpNciHal_WorkerThread&) = delete;

  /******************************************************************************
   * Function:       Start()
   *
   * Description:    This method creates & initiates the worker thread for
   *                 handling NFCC communication.
   *
   * Returns:        bool: True if the worker thread was successfully started
   *                 otherwise false.
   ******************************************************************************/
  bool Start();

  /******************************************************************************
   * Function:       Stop()
   *
   * Description:    This method stops the worker thread and clear the resources
   *
   * Returns:        bool: True if the worker thread was successfully stoped
   *                 otherwise false.
   ******************************************************************************/
  bool Stop();

 private:
  phNxpNciHal_WorkerThread();
  ~phNxpNciHal_WorkerThread();

  static void* WorkerThread(void* arg);
  void Run();
  pthread_t worker_thread;
  volatile std::atomic<bool> thread_running;
};
#endif  // NXPNCIHALWORKER_H
