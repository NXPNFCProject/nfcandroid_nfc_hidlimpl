/*
 *
 *  Copyright 2021-2026 NXP
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
#define LOG_TAG "PalThreadMutex"

#include "PalThreadMutex.h"
#include "phNxpLog.h"

/*******************************************************************************
**
** Function:    PalThreadMutex::PalThreadMutex()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
PalThreadMutex::PalThreadMutex() {
  pthread_mutexattr_t mutexAttr;

  pthread_mutexattr_init(&mutexAttr);
  if (pthread_mutex_init(&mMutex, &mutexAttr)) {
    NXPLOG_EXTNS_D(LOG_TAG, "init mutex success");
  } else {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to init mutex");
  }
  if (pthread_mutexattr_destroy(&mutexAttr) != 0) {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to destroy mutex attribute");
  }
}

/*******************************************************************************
**
** Function:    PalThreadMutex::~PalThreadMutex()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
PalThreadMutex::~PalThreadMutex() {
  if (pthread_mutex_destroy(&mMutex) != 0) {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to destroy mutex");
  }
}

/*******************************************************************************
**
** Function:    PalThreadMutex::lock()
**
** Description: lock kthe mutex
**
** Returns:     none
**
*******************************************************************************/
void PalThreadMutex::lock() {
  if (pthread_mutex_lock(&mMutex) != 0) {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to lock mutex");
  }
}

/*******************************************************************************
**
** Function:    PalThreadMutex::unblock()
**
** Description: unlock the mutex
**
** Returns:     none
**
*******************************************************************************/
void PalThreadMutex::unlock() { pthread_mutex_unlock(&mMutex); }

/*******************************************************************************
**
** Function:    PalThreadCondVar::PalThreadCondVar()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
PalThreadCondVar::PalThreadCondVar() {
  pthread_condattr_t CondAttr;

  if (pthread_condattr_init(&CondAttr) != 0) {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to init condition attribute");
  }
  if (pthread_condattr_setclock(&CondAttr, CLOCK_MONOTONIC) != 0) {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to set clock for condition attribute");
  }
  if (pthread_cond_init(&mCondVar, &CondAttr) != 0) {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to init condition variable");
  }

  if (pthread_condattr_destroy(&CondAttr) != 0) {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to destroy condition attribute");
  }
}

/*******************************************************************************
**
** Function:    PalThreadCondVar::~PalThreadCondVar()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
PalThreadCondVar::~PalThreadCondVar() {
  if (pthread_cond_destroy(&mCondVar) != 0) {
    NXPLOG_EXTNS_E(LOG_TAG, "fail to destroy condition variable");
  }
}

/*******************************************************************************
**
** Function:    PalThreadCondVar::timedWait()
**
** Description: wait on the mCondVar or till timeout happens
**
** Returns:     none
**
*******************************************************************************/
void PalThreadCondVar::timedWait(struct timespec *time) {
  if (pthread_cond_timedwait(&mCondVar, static_cast<pthread_mutex_t*>(*this), time) != 0) {
     NXPLOG_EXTNS_D(LOG_TAG, "timed wait returned non-zero");
  }
}

/*******************************************************************************
**
** Function:    PalThreadCondVar::timedWait()
**
** Description: wait on the mCondVar or till timeout happens
**
** Returns:     none
**
*******************************************************************************/
void PalThreadCondVar::timedWait(uint8_t sec) {
  struct timespec timeout_spec;
  clock_gettime(CLOCK_MONOTONIC, &timeout_spec);
  timeout_spec.tv_sec += sec;
  pthread_cond_timedwait(&mCondVar, static_cast<pthread_mutex_t*>(*this), &timeout_spec);
}

/*******************************************************************************
**
** Function:    PalThreadCondVar::signal()
**
** Description: signal the mCondVar
**
** Returns:     none
**
*******************************************************************************/
void PalThreadCondVar::signal() {
  const PalAutoThreadMutex a(*this);
  if (pthread_cond_signal(&mCondVar) != 0) {
     NXPLOG_EXTNS_E(LOG_TAG, "fail to signal condition variable");
  }
}

/*******************************************************************************
**
** Function:    PalAutoThreadMutex::PalAutoThreadMutex()
**
** Description: class constructor, automatically lock the mutex
**
** Returns:     none
**
*******************************************************************************/
PalAutoThreadMutex::PalAutoThreadMutex(PalThreadMutex &m) : mm(m) { mm.lock(); }

/*******************************************************************************
**
** Function:    PalAutoThreadMutex::~PalAutoThreadMutex()
**
** Description: class destructor, automatically unlock the mutex
**
** Returns:     none
**
*******************************************************************************/
PalAutoThreadMutex::~PalAutoThreadMutex() { mm.unlock(); }
