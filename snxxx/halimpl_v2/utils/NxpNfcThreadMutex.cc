/*
 *
 *  Copyright 2021-2024 NXP
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
#define LOG_TAG "NxpNfcThreadMutex"

#include <android-base/logging.h>
#include "NxpNfcThreadMutex.h"
#include <android-base/stringprintf.h>

using android::base::StringPrintf;

/*******************************************************************************
**
** Function:    NfcHalThreadMutex::NfcHalThreadMutex()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
NfcHalThreadMutex::NfcHalThreadMutex() {
  pthread_mutexattr_t mutexAttr;

  pthread_mutexattr_init(&mutexAttr);
  if(pthread_mutex_init(&mMutex, &mutexAttr))
    LOG(DEBUG) << StringPrintf("init mutex success");
  else
    LOG(ERROR) << StringPrintf("fail to init mutex");
  pthread_mutexattr_destroy(&mutexAttr);
}

/*******************************************************************************
**
** Function:    NfcHalThreadMutex::~NfcHalThreadMutex()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
NfcHalThreadMutex::~NfcHalThreadMutex() { pthread_mutex_destroy(&mMutex); }

/*******************************************************************************
**
** Function:    NfcHalThreadMutex::lock()
**
** Description: lock kthe mutex
**
** Returns:     none
**
*******************************************************************************/
void NfcHalThreadMutex::lock() { pthread_mutex_lock(&mMutex); }

/*******************************************************************************
**
** Function:    NfcHalThreadMutex::unblock()
**
** Description: unlock the mutex
**
** Returns:     none
**
*******************************************************************************/
void NfcHalThreadMutex::unlock() { pthread_mutex_unlock(&mMutex); }

/*******************************************************************************
**
** Function:    NfcHalThreadCondVar::NfcHalThreadCondVar()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
NfcHalThreadCondVar::NfcHalThreadCondVar() {
  pthread_condattr_t CondAttr;

  pthread_condattr_init(&CondAttr);
  pthread_condattr_setclock(&CondAttr, CLOCK_MONOTONIC);
  pthread_cond_init(&mCondVar, &CondAttr);

  pthread_condattr_destroy(&CondAttr);
}

/*******************************************************************************
**
** Function:    NfcHalThreadCondVar::~NfcHalThreadCondVar()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
NfcHalThreadCondVar::~NfcHalThreadCondVar() { pthread_cond_destroy(&mCondVar); }

/*******************************************************************************
**
** Function:    NfcHalThreadCondVar::timedWait()
**
** Description: wait on the mCondVar or till timeout happens
**
** Returns:     none
**
*******************************************************************************/
void NfcHalThreadCondVar::timedWait(struct timespec* time) {
  pthread_cond_timedwait(&mCondVar, *this, time);
}

/*******************************************************************************
**
** Function:    NfcHalThreadCondVar::timedWait()
**
** Description: wait on the mCondVar or till timeout happens
**
** Returns:     none
**
*******************************************************************************/
void NfcHalThreadCondVar::timedWait(uint8_t sec) {
  struct timespec timeout_spec;
  clock_gettime(CLOCK_REALTIME, &timeout_spec);
  timeout_spec.tv_sec += sec;
  pthread_cond_timedwait(&mCondVar, *this, &timeout_spec);
}

/*******************************************************************************
**
** Function:    NfcHalThreadCondVar::signal()
**
** Description: signal the mCondVar
**
** Returns:     none
**
*******************************************************************************/
void NfcHalThreadCondVar::signal() {
  NfcHalAutoThreadMutex a(*this);
  pthread_cond_signal(&mCondVar);
}

/*******************************************************************************
**
** Function:    NfcHalAutoThreadMutex::NfcHalAutoThreadMutex()
**
** Description: class constructor, automatically lock the mutex
**
** Returns:     none
**
*******************************************************************************/
NfcHalAutoThreadMutex::NfcHalAutoThreadMutex(NfcHalThreadMutex& m) : mm(m) {
  mm.lock();
}

/*******************************************************************************
**
** Function:    NfcHalAutoThreadMutex::~NfcHalAutoThreadMutex()
**
** Description: class destructor, automatically unlock the mutex
**
** Returns:     none
**
*******************************************************************************/
NfcHalAutoThreadMutex::~NfcHalAutoThreadMutex() { mm.unlock(); }
