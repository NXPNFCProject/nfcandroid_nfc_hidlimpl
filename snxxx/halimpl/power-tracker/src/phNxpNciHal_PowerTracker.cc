/*
 * Copyright 2022 NXP
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
#include "phNxpNciHal_PowerTracker.h"
#include "IntervalTimer.h"
#include "NfcProperties.sysprop.h"
#include "NxpNfcThreadMutex.h"
#include "phNxpConfig.h"
#include "phNxpNciHal_ext.h"

#include <aidl/android/vendor/powerstats/BnPixelStateResidencyCallback.h>
#include <aidl/android/vendor/powerstats/IPixelStateResidencyProvider.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using namespace vendor::nfc::nxp;
using ::aidl::android::hardware::power::stats::StateResidency;
using ::aidl::android::vendor::powerstats::BnPixelStateResidencyCallback;
using ::aidl::android::vendor::powerstats::IPixelStateResidencyCallback;
using ::aidl::android::vendor::powerstats::IPixelStateResidencyProvider;
using ::ndk::ScopedAStatus;
using ::ndk::SpAIBinder;
using std::vector;

/******************* Macro definition *****************************************/
#define STATE_STANDBY_STR "STANDBY"
#define STATE_ULPDET_STR "ULPDET"
#define STATE_ACTIVE_STR "ACTIVE"
#define STEP_TIME_MS 100

#define TIME_MS(spec) (spec.tv_sec * 1000 + (long)(spec.tv_nsec / 1000000))

const char* kPowerStatsSrvName = "power.stats-vendor";
const char* kPowerEntityName = "Nfc";

/******************* Local functions *****************************************/
static void* phNxpNciHal_pollPowerTrackerData(void* pContext);
static NFCSTATUS phNxpNciHal_syncPowerTrackerDataLocked();

/******************* Enums and Class declarations ***********************/
enum NfccStates {
  STANDBY = 0,
  ULPDET,
  ACTIVE,
  MAX_STATES,
};

typedef struct {
  uint32_t stateEntryCount;
  uint32_t stateTickCount;
} StateResidencyData;

typedef struct {
  long pollDurationMs;
  pthread_t thread;
  NfcHalThreadCondVar event;    // To signal events.
  NfcHalThreadMutex dataMutex;  // To protect state data
  bool_t pollPowerTracker;
  struct timespec lastSyncTime;
  StateResidencyData stateData[MAX_STATES];
  IntervalTimer ulpdetTimer;
  struct timespec ulpdetStartTime;
  bool_t ulpdetOn;
} PowerTrackerContext;

/**
 * Class implemets power state residency callback.
 *
 * power.stats-venor service will involke getStateResidency callback
 * periodically for fetching current value of cached power data for each Nfc
 * state.
 */
class NfcStateResidencyCallback : public BnPixelStateResidencyCallback {
 public:
  NfcStateResidencyCallback(PowerTrackerContext* context) {
    NXPLOG_NCIHAL_I("%s: descriptor = %s", __func__, descriptor);
    pContext = context;
  }
  /**
   * Returns current state residency data for all Nfc states.
   */
  virtual ScopedAStatus getStateResidency(
      vector<StateResidency>* stateResidency) {

    if (stateResidency == nullptr) {
      return ScopedAStatus::fromServiceSpecificErrorWithMessage(
          NFCSTATUS_INVALID_PARAMETER, "Invalid Parama");
    }
    stateResidency->resize(MAX_STATES);

    NXPLOG_NCIHAL_I("%s: Returning state residency data", __func__);

    // STANDBY
    (*stateResidency)[STANDBY].id = STANDBY;
    (*stateResidency)[STANDBY].totalTimeInStateMs =
        NfcProps::standbyStateTick().value_or(0) * STEP_TIME_MS;
    (*stateResidency)[STANDBY].totalStateEntryCount =
          NfcProps::standbyStateEntryCount().value_or(0);
    (*stateResidency)[STANDBY].lastEntryTimestampMs = 0;
    // ULPDET
    (*stateResidency)[ULPDET].id = ULPDET;
    (*stateResidency)[ULPDET].totalTimeInStateMs =
        NfcProps::ulpdetStateTick().value_or(0) * STEP_TIME_MS;
    (*stateResidency)[ULPDET].totalStateEntryCount =
          NfcProps::ulpdetStateEntryCount().value_or(0);
    (*stateResidency)[ULPDET].lastEntryTimestampMs = 0;
    // ACTIVE
    (*stateResidency)[ACTIVE].id = ACTIVE;
    (*stateResidency)[ACTIVE].totalTimeInStateMs =
        NfcProps::activeStateTick().value_or(0) * STEP_TIME_MS;
    (*stateResidency)[ACTIVE].totalStateEntryCount =
          NfcProps::activeStateEntryCount().value_or(0);
    (*stateResidency)[ACTIVE].lastEntryTimestampMs = 0;

    return ScopedAStatus::ok();
  }
  ~NfcStateResidencyCallback() {}

 private:
  PowerTrackerContext* pContext = NULL;
};

/******************* External variables
 * *****************************************/
extern phNxpNciHal_Control_t nxpncihal_ctrl;

/*********************** Global Variables *************************************/

static PowerTrackerContext gContext = {
    .pollDurationMs = 0,
    .thread = 0,
    .pollPowerTracker = false,
    .lastSyncTime.tv_sec = 0,
    .lastSyncTime.tv_nsec = 0,
    .stateData[STANDBY].stateEntryCount = 0,
    .stateData[STANDBY].stateTickCount = 0,
    .stateData[ULPDET].stateEntryCount = 0,
    .stateData[ULPDET].stateTickCount = 0,
    .stateData[ACTIVE].stateEntryCount = 0,
    .stateData[ACTIVE].stateTickCount = 0,
};

std::shared_ptr<IPixelStateResidencyProvider> gStateResidencyProvider = NULL;
std::shared_ptr<IPixelStateResidencyCallback> gStateResidencyCallback =
    ndk::SharedRefBase::make<NfcStateResidencyCallback>(&gContext);

/*******************************************************************************
**
** Function         phNxpNciHal_startPowerTracker()
**
** Description      Function to start power tracker feature.
**
** Parameters       pollDuration - Poll duration in MS for fetching power data
*from NFCC.
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/

NFCSTATUS phNxpNciHal_startPowerTracker(unsigned long pollDuration) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  uint8_t cmd_configPowerTracker[] = {
      0x20, 0x02,  // CORE_SET_CONFIG_CMD
      0x05,        // Total LEN
      0x01,        // Num of param fields to follow
      0xA0, 0x6D,  // NCI_CORE_PROP_SYSTEM_POWER_TRACE_ENABLE
      0x01,        // Length of val filed to follow
      0x01,        // Value field. Enabled
  };

  NXPLOG_NCIHAL_I("%s: Starting PowerTracker with poll duration %ld", __func__,
                  pollDuration);
  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_configPowerTracker),
                                    cmd_configPowerTracker);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("Failed to Start PowerTracker, status - %d", status);
    return status;
  }
  if (!gContext.pollPowerTracker) {
    // Initialize last sync time to the start time.
    if (clock_gettime(CLOCK_REALTIME, &gContext.lastSyncTime) == -1) {
      NXPLOG_NCIHAL_E("%s Fail get time; errno=0x%X", __func__, errno);
    }
    // Sync Initial values of variables from sys properties.
    // so that previous values are used in case of Nfc HAL crash.
    gContext.stateData[ACTIVE].stateEntryCount =
        NfcProps::activeStateEntryCount().value_or(0);
    gContext.stateData[ACTIVE].stateTickCount =
        NfcProps::activeStateTick().value_or(0);
    gContext.stateData[STANDBY].stateEntryCount =
        NfcProps::standbyStateEntryCount().value_or(0);
    gContext.stateData[STANDBY].stateTickCount =
        NfcProps::standbyStateTick().value_or(0);
    gContext.stateData[ULPDET].stateEntryCount =
        NfcProps::ulpdetStateEntryCount().value_or(0);
    gContext.stateData[ULPDET].stateTickCount =
        NfcProps::ulpdetStateTick().value_or(0);

    // Start polling Thread
    gContext.pollDurationMs = pollDuration;
    gContext.pollPowerTracker = true;
    status = pthread_create(&gContext.thread, NULL,
                            phNxpNciHal_pollPowerTrackerData, &gContext);
    if (status != 0) {
      NXPLOG_NCIHAL_E("Failed to create PowerTracker, thread - %d", status);
      return status;
    }
    // Register with power states HAL.
    SpAIBinder binder(AServiceManager_checkService(kPowerStatsSrvName));
    gStateResidencyProvider = IPixelStateResidencyProvider::fromBinder(binder);
    if (gStateResidencyProvider == nullptr) {
      NXPLOG_NCIHAL_E("%s: Failed to get PowerStats service", __func__);
    } else {
      NXPLOG_NCIHAL_D("%s: Registering with Power Stats service", __func__);
      gStateResidencyProvider->registerCallback(kPowerEntityName,
                                                gStateResidencyCallback);
      NXPLOG_NCIHAL_D("%s: Starting thread pool for communication with powerstats", __func__);
      ABinderProcess_setThreadPoolMaxThreadCount(1);
      ABinderProcess_startThreadPool();
      NXPLOG_NCIHAL_D("%s: Thread pool started successfully", __func__);
    }
  } else {
    NXPLOG_NCIHAL_E("PowerTracker already enabled");
  }
  return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_pollPowerTrackerData
**
** Description      Thread funcation which tracks power data in a loop with
**                  configured duration until power tracker feature is stopped.
**
** Parameters       pContext - Power tracker thread context if any.
** Returns          None
*******************************************************************************/

static void* phNxpNciHal_pollPowerTrackerData(void* pCtx) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  PowerTrackerContext* pContext = (PowerTrackerContext*)pCtx;

  NXPLOG_NCIHAL_D("Starting to poll for PowerTracker data");
  // Poll till power tracker is cancelled.
  while (pContext->pollPowerTracker) {
    struct timespec absoluteTime;

    if (clock_gettime(CLOCK_REALTIME, &absoluteTime) == -1) {
      NXPLOG_NCIHAL_E("%s Fail get time; errno=0x%X", __func__, errno);
    } else {
      absoluteTime.tv_sec += pContext->pollDurationMs / 1000;
      long ns =
          absoluteTime.tv_nsec + ((pContext->pollDurationMs % 1000) * 1000000);
      if (ns > 1000000000) {
        absoluteTime.tv_sec++;
        absoluteTime.tv_nsec = ns - 1000000000;
      } else
        absoluteTime.tv_nsec = ns;
    }
    pContext->event.lock();
    // Wait for poll duration
    pContext->event.timedWait(&absoluteTime);

    NfcHalAutoThreadMutex(gContext.dataMutex);
    // Sync and cache power tracker data.
    if (pContext->pollPowerTracker) {
      status = phNxpNciHal_syncPowerTrackerDataLocked();
      if (NFCSTATUS_SUCCESS != status) {
        NXPLOG_NCIHAL_E("Failed to fetch PowerTracker data. error = %d",
                        status);
        // break;
      }
    }
  }
  NXPLOG_NCIHAL_D("Stopped polling for PowerTracker data");
  return NULL;
}

/*******************************************************************************
**
** Function         phNxpNciHal_syncPowerTrackerDataLocked()
**
** Description      Function to sync power tracker data from NFCC chip.
**                  This should be called with dataMutex locked.
**
** Parameters       None.
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/

static NFCSTATUS phNxpNciHal_syncPowerTrackerDataLocked() {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  struct timespec currentTime = {.tv_sec = 0, .tv_nsec = 0};
  ;
  uint8_t cmd_getPowerTrackerData[] = {0x2F,
                                       0x2E,  // NCI_PROP_GET_PWR_TRACK_DATA_CMD
                                       0x00};  // LENGTH
  long activeTick = 0, activeCounter = 0;

  CONCURRENCY_LOCK();  // This lock is to protect origin field.
  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_getPowerTrackerData),
                                    cmd_getPowerTrackerData);
  CONCURRENCY_UNLOCK();
  if (status != NFCSTATUS_SUCCESS) {
    return status;
  }
  if (nxpncihal_ctrl.p_rx_data[3] != NFCSTATUS_SUCCESS) {
    return (NFCSTATUS)nxpncihal_ctrl.p_rx_data[3];
  }

  if (clock_gettime(CLOCK_REALTIME, &currentTime) == -1) {
    NXPLOG_NCIHAL_E("%s Fail get time; errno=0x%X", __func__, errno);
  }
  uint8_t* rsp = nxpncihal_ctrl.p_rx_data;
  activeCounter =
      (rsp[4] << 0) | (rsp[5] << 8) | (rsp[6] << 16) | (rsp[7] << 24);
  activeTick =
      (rsp[8] << 0) | (rsp[9] << 8) | (rsp[10] << 16) | (rsp[11] << 24);

  // Calculate time difference between two sync
  long totalTimeMs = TIME_MS(currentTime) - TIME_MS(gContext.lastSyncTime);
  if (totalTimeMs > gContext.pollDurationMs) {
    NXPLOG_NCIHAL_D("Total time duration (%ld) > Poll duration (%ld)",
                    totalTimeMs, gContext.pollDurationMs);
  }
  gContext.stateData[ACTIVE].stateEntryCount += activeCounter;
  gContext.stateData[ACTIVE].stateTickCount += activeTick;
  gContext.stateData[STANDBY].stateEntryCount += activeCounter;
  gContext.stateData[STANDBY].stateTickCount +=
      ((totalTimeMs / STEP_TIME_MS) - activeTick);
  gContext.lastSyncTime = currentTime;

  // Sync values of variables to sys properties so that
  // previous values can be synced in case of Nfc HAL crash.
  NfcProps::activeStateEntryCount(gContext.stateData[ACTIVE].stateEntryCount);
  NfcProps::activeStateTick(gContext.stateData[ACTIVE].stateTickCount);
  NfcProps::standbyStateEntryCount(gContext.stateData[STANDBY].stateEntryCount);
  NfcProps::standbyStateTick(gContext.stateData[STANDBY].stateTickCount);

  NXPLOG_NCIHAL_D(
      "Successfully fetched power tracker data "
      "Active counter = %u, Active Tick = %u\n"
      "Standby Counter = %u, Standby Tick = %u\n"
      "ULPDET Counter = %u, ULPDET Tick = %u",
      gContext.stateData[ACTIVE].stateEntryCount,
      gContext.stateData[ACTIVE].stateTickCount,
      gContext.stateData[STANDBY].stateEntryCount,
      gContext.stateData[STANDBY].stateTickCount,
      gContext.stateData[ULPDET].stateEntryCount,
      gContext.stateData[ULPDET].stateTickCount);
  return status;
}

static void onUlpdetTimerExpired(union sigval val) {
  (void)val;
  NXPLOG_NCIHAL_D("%s Ulpdet Timer Expired,", __func__);
  struct timespec ulpdetEndTime = {.tv_sec = 0, .tv_nsec = 0};
  // End ulpdet Tick.
  if (clock_gettime(CLOCK_REALTIME, &ulpdetEndTime) == -1) {
    NXPLOG_NCIHAL_E("%s fail get time; errno=0x%X", __func__, errno);
  }
  long ulpdetTimeMs =
      TIME_MS(ulpdetEndTime) - TIME_MS(gContext.ulpdetStartTime);

  NfcHalAutoThreadMutex(gContext.dataMutex);
  // Convert to Tick with 100ms step
  gContext.stateData[ULPDET].stateTickCount += (ulpdetTimeMs / STEP_TIME_MS);
  // Sync values of variables to sys properties so that
  // previous values can be synced in case of Nfc HAL crash.
  NfcProps::ulpdetStateEntryCount(gContext.stateData[ULPDET].stateEntryCount);
  NfcProps::ulpdetStateTick(gContext.stateData[ULPDET].stateTickCount);
  gContext.ulpdetStartTime = ulpdetEndTime;
  if (gContext.ulpdetOn) {
    // Start ULPDET Timer
    NXPLOG_NCIHAL_D("%s Refreshing Ulpdet Timer", __func__);
    gContext.ulpdetTimer.set(gContext.pollDurationMs, NULL,
                             onUlpdetTimerExpired);
  }
}

/*******************************************************************************
**
** Function         phNxpNciHal_onPowerStateChange()
**
** Description      Callback involked internally by HAL whenever there is system
**                  state change
**
** Parameters       state - Can be SCREEN_OFF, SCREEN_ON, ULPDET_OFF, ULPDET_ON
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/

NFCSTATUS phNxpNciHal_onPowerStateChange(PowerState state) {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  union sigval val;
  switch (state) {
    case SCREEN_ON:
      // Signal power tracker thread to sync data from NFCC
      NXPLOG_NCIHAL_D("%s Screen state On, Syncing PowerTracker data",
                      __func__);
      gContext.event.signal();
      break;
    case ULPDET_ON:
      NXPLOG_NCIHAL_D("%s Ulpdet On, Syncing PowerTracker data", __func__);
      if (phNxpNciHal_syncPowerTrackerDataLocked() != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("Failed to fetch PowerTracker data. error = %d",
                        status);
      }
      gContext.dataMutex.lock();
      gContext.stateData[ULPDET].stateEntryCount++;
      if (clock_gettime(CLOCK_REALTIME, &gContext.ulpdetStartTime) == -1) {
        NXPLOG_NCIHAL_E("%s fail get time; errno=0x%X", __func__, errno);
      }
      gContext.ulpdetOn = true;
      gContext.dataMutex.unlock();
      // Start ULPDET Timer
      gContext.ulpdetTimer.set(gContext.pollDurationMs, NULL,
                               onUlpdetTimerExpired);
      break;
    case ULPDET_OFF:
      NXPLOG_NCIHAL_D("%s Ulpdet Off, Killing ULPDET timer", __func__);
      gContext.ulpdetTimer.kill();
      gContext.ulpdetOn = false;
      onUlpdetTimerExpired(val);
      gContext.ulpdetStartTime = {.tv_sec = 0, .tv_nsec = 0};
      break;
    default:
      status = NFCSTATUS_FAILED;
      break;
  }
  NXPLOG_NCIHAL_D("%s Exit", __func__);
  return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_stopPowerTracker()
**
** Description      Function to stop power tracker feature.
**
** Parameters       None
** Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
*******************************************************************************/

NFCSTATUS phNxpNciHal_stopPowerTracker() {
  NFCSTATUS status = NFCSTATUS_SUCCESS;
  uint8_t cmd_configPowerTracker[] = {
      0x20, 0x02,  // CORE_SET_CONFIG_CMD
      0x05,        // Total LEN
      0x01,        // Num of param fields to follow
      0xA0, 0x6D,  // NCI_CORE_PROP_SYSTEM_POWER_TRACE_ENABLE
      0x01,        // Length of val filed to follow
      0x00,        // Value field. Disabled
  };
  status = phNxpNciHal_send_ext_cmd(sizeof(cmd_configPowerTracker),
                                    cmd_configPowerTracker);
  if (status != NFCSTATUS_SUCCESS) {
    NXPLOG_NCIHAL_E("%s Failed to disable PowerTracker, error = %d", __func__,
                    status);
  }
  if (gContext.pollPowerTracker) {
    // Stop Polling Thread
    gContext.pollPowerTracker = false;
    gContext.event.signal();
    pthread_join(gContext.thread, NULL);
  } else {
    NXPLOG_NCIHAL_E("PowerTracker is already disabled");
  }
  // Unregister with PowerStats service
  if (gStateResidencyProvider != nullptr) {
    gStateResidencyProvider->unregisterCallback(gStateResidencyCallback);
    gStateResidencyProvider = nullptr;
    NXPLOG_NCIHAL_D("%s: Unegistered PowerStats callback successfully",
                    __func__);
  }
  NXPLOG_NCIHAL_I("%s: Stopped PowerTracker", __func__);
  return status;
}
