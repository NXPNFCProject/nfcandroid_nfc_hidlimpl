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

#include "ReaderPollConfigParser.h"

#include <phNfcNciConstants.h>

using namespace std;

/*****************************************************************************
 *
 * Function         getWellKnownModEventData
 *
 * Description      Frames Well known type reader poll info notification
 *
 * Parameters       event - Event type A, B & F
 *                  timeStamp - time stamp of the event
 *                  gain - RSSI value
 *
 * Returns          Returns Well known type reader poll info notification
 *
 ****************************************************************************/
vector<uint8_t> ReaderPollConfigParser::getWellKnownModEventData(
    uint8_t event, vector<uint8_t> timeStamp, uint8_t gain,
    vector<uint8_t> data = vector<uint8_t>()) {
  vector<uint8_t> eventData;
  eventData.push_back(event);
  eventData.push_back(SHORT_FLAG);  // Always short frame
  eventData.push_back(timeStamp.size() + GAIN_FIELD_LENGTH + data.size());
  eventData.insert(std::end(eventData), std::begin(timeStamp),
                   std::end(timeStamp));
  eventData.push_back(gain);
  eventData.insert(std::end(eventData), std::begin(data), std::end(data));
  return eventData;
}

/*****************************************************************************
 *
 * Function         getUnknownEvent
 *
 * Description      Frames Unknown event type reader poll info notification
 *
 * Parameters       data - Data bytes of Unknown event
 *                  timeStamp - time stamp of the event
 *                  gain - RSSI value
 *
 * Returns          Returns Unknown type reader poll info notification
 *
 ***************************************************************************/
vector<uint8_t> ReaderPollConfigParser::getUnknownEvent(
    vector<uint8_t> data, vector<uint8_t> timeStamp, uint8_t gain) {
  uint8_t eventLength = timeStamp.size() + GAIN_FIELD_LENGTH + (int)data.size();
  vector<uint8_t> eventData;
  eventData.push_back(TYPE_UNKNOWN);
  eventData.push_back(SHORT_FLAG);  // Always short frame
  eventData.push_back(eventLength);
  eventData.insert(std::end(eventData), std::begin(timeStamp),
                   std::end(timeStamp));
  eventData.push_back(gain);
  eventData.insert(std::end(eventData), std::begin(data), std::end(data));
  return eventData;
}

/*****************************************************************************
 *
 * Function         getRFEventData
 *
 * Description      Frames Well known type reader poll info notification
 *
 * Parameters       timeStamp - time stamp of the event
 *                  gain - RSSI value
 *                  rfState - 0x00 for RF OFF, 0x01 for RF ON
 *
 * Returns          Returns RF State reader poll info notification
 *
 ****************************************************************************/
vector<uint8_t> ReaderPollConfigParser::getRFEventData(
    vector<uint8_t> timeStamp, uint8_t gain, bool rfState) {
  uint8_t eventLength =
      timeStamp.size() + GAIN_FIELD_LENGTH + RF_STATE_FIELD_LENGTH;
  vector<uint8_t> eventData;
  eventData.push_back(TYPE_RF_FLAG);
  eventData.push_back(SHORT_FLAG);  // Always short frame
  eventData.push_back(eventLength);
  eventData.insert(std::end(eventData), std::begin(timeStamp),
                   std::end(timeStamp));
  eventData.push_back(gain);
  eventData.push_back((uint8_t)(rfState ? 0x01 : 0x00));
  return eventData;
}

/*****************************************************************************
 *
 * Function         parseCmaEvent
 *
 * Description      This function parses the unknown frames
 *
 * Parameters       p_event - Data bytes of type Unknown event
 *
 * Returns          Filters Type-B/Type-F data frames
 *                  and converts other frame to  unknown frame
 *
 ***************************************************************************/
vector<uint8_t> ReaderPollConfigParser::parseCmaEvent(vector<uint8_t> p_event) {
  vector<uint8_t> event_data = vector<uint8_t>();
  if (lastKnownModEvent == EVENT_MOD_B && p_event.size() > 0 &&
      p_event[0] == TYPE_B_APF) {  // Type B Apf value is 0x05
    if (this->notificationType != TYPE_ONLY_MOD_EVENTS) {
      event_data =
          getWellKnownModEventData(TYPE_MOD_B, std::move(unknownEventTimeStamp),
                                   lastKnownGain, std::move(p_event));
    }
  } else if (lastKnownModEvent == EVENT_MOD_F &&
             p_event[0] == TYPE_F_CMD_LENGH && p_event[2] == TYPE_F_ID &&
             p_event[3] == TYPE_F_ID) {
    if (this->notificationType != TYPE_ONLY_MOD_EVENTS) {
      event_data =
          getWellKnownModEventData(TYPE_MOD_F, std::move(unknownEventTimeStamp),
                                   lastKnownGain, std::move(p_event));
    }
  } else {
    event_data = getUnknownEvent(
        std::move(p_event), std::move(unknownEventTimeStamp), lastKnownGain);
  }
  return event_data;
}

/*****************************************************************************
 *
 * Function         getEvent
 *
 * Description      It identifies the type of event and gets the reader poll
 *                  info
 *                  notification
 *
 * Parameters       p_event - Vector Lx Notification
 *                  cmaEventType - CMA event type
 *
 * Returns          This function return reader poll info notification
 *
 ****************************************************************************/
vector<uint8_t> ReaderPollConfigParser::getEvent(vector<uint8_t> p_event,
                                                 uint8_t cmaEventType) {
  vector<uint8_t> event_data;
  if ((cmaEventType == L2_EVT_TAG &&
       (int)p_event.size() < MIN_LEN_NON_CMA_EVT) ||
      (cmaEventType == CMA_EVT_TAG && (int)p_event.size() < MIN_LEN_CMA_EVT) ||
      (cmaEventType == CMA_EVT_EXTRA_DATA_TAG &&
       (int)p_event.size() < MIN_LEN_CMA_EXTRA_DATA_EVT)) {
    return event_data;
  }

  if (cmaEventType == L2_EVT_TAG) {
    // Timestamp should be in Big Endian format
    int idx = 3;
    vector<uint8_t> timestamp;
    timestamp.push_back(p_event[idx--]);
    timestamp.push_back(p_event[idx--]);
    timestamp.push_back(p_event[idx--]);
    timestamp.push_back(p_event[idx]);
    lastKnownGain = p_event[INDEX_OF_L2_EVT_GAIN];
    switch (p_event[INDEX_OF_L2_EVT_TYPE] & LX_TYPE_MASK) {
      // Trigger Type
      case L2_EVENT_TRIGGER_TYPE:
        // Modulation detected
        switch ((p_event[INDEX_OF_L2_EVT_TYPE] & LX_EVENT_MASK) >> 4) {
          case EVENT_MOD_A:
            lastKnownModEvent = EVENT_MOD_A;
            if (this->notificationType != TYPE_ONLY_CMA_EVENTS) {
              event_data = getWellKnownModEventData(
                  TYPE_MOD_A, std::move(timestamp), lastKnownGain);
            }
            break;

          case EVENT_MOD_B:
            lastKnownModEvent = EVENT_MOD_B;
            if (this->notificationType != TYPE_ONLY_CMA_EVENTS) {
              event_data = getWellKnownModEventData(
                  TYPE_MOD_B, std::move(timestamp), lastKnownGain);
            }
            break;

          case EVENT_MOD_F:
            lastKnownModEvent = EVENT_MOD_F;
            if (this->notificationType != TYPE_ONLY_CMA_EVENTS) {
              event_data = getWellKnownModEventData(
                  TYPE_MOD_F, std::move(timestamp), lastKnownGain);
            }
            break;

          default:
            event_data = getUnknownEvent(
                vector<uint8_t>(p_event.begin() + INDEX_OF_L2_EVT_TYPE,
                                p_event.end()),
                std::move(timestamp), lastKnownGain);
        }
        break;

      case EVENT_RF_ON:
        // External RF Field is ON
        event_data = getRFEventData(std::move(timestamp), lastKnownGain, true);
        break;

      case EVENT_RF_OFF:
        event_data = getRFEventData(std::move(timestamp), lastKnownGain, false);
        break;

      default:
        event_data = getUnknownEvent(
            vector<uint8_t>(p_event.begin() + INDEX_OF_L2_EVT_TYPE,
                            p_event.end()),
            std::move(timestamp), lastKnownGain);
        break;
    }

  } else if (cmaEventType == CMA_EVT_TAG) {
    // Timestamp should be in Big Endian format
    int idx = 3;
    vector<uint8_t> timestamp;
    timestamp.push_back(p_event[idx--]);
    timestamp.push_back(p_event[idx--]);
    timestamp.push_back(p_event[idx--]);
    timestamp.push_back(p_event[idx]);
    switch (p_event[INDEX_OF_CMA_EVT_TYPE]) {
      // Trigger Type
      case CMA_EVENT_TRIGGER_TYPE:
        switch (p_event[INDEX_OF_CMA_EVT_DATA]) {
          case REQ_A:
            if (this->notificationType != TYPE_ONLY_MOD_EVENTS) {
              event_data = getWellKnownModEventData(
                  TYPE_MOD_A, std::move(timestamp), lastKnownGain, {REQ_A});
            }
            break;

          case WUP_A:
            if (this->notificationType != TYPE_ONLY_MOD_EVENTS) {
              event_data = getWellKnownModEventData(
                  TYPE_MOD_A, std::move(timestamp), lastKnownGain, {WUP_A});
            }
            break;
          default:
            event_data = getUnknownEvent(
                vector<uint8_t>(p_event.begin() + INDEX_OF_CMA_EVT_DATA,
                                p_event.end()),
                std::move(timestamp), lastKnownGain);
        }
        break;
      case CMA_DATA_TRIGGER_TYPE: {
        readExtraBytesForUnknownEvent = true;
        extraByteLength = p_event[INDEX_OF_CMA_EVT_DATA];
        unknownEventTimeStamp = timestamp;
        break;
      }
      default: {
        vector<uint8_t> payloadData = vector<uint8_t>(
            p_event.begin() + INDEX_OF_CMA_EVT_TYPE, p_event.end());
        event_data = getUnknownEvent(std::move(payloadData),
                                     std::move(timestamp), lastKnownGain);
      }
    }
  } else if (cmaEventType == CMA_EVT_EXTRA_DATA_TAG &&
             readExtraBytesForUnknownEvent) {
    extraBytes.insert(std::end(extraBytes), std::begin(p_event),
                      std::end(p_event));

    // If the required bytes received from Extra Data frames, process the unknown
    // event and reset the extra data bytes
    if (extraBytes.size() >= extraByteLength) {
      event_data = parseCmaEvent(std::move(extraBytes));
      resetExtraBytesInfo();
    }
  }

  return event_data;
}

/*****************************************************************************
 *
 * Function         notifyPollingLoopInfoEvent
 *
 * Description      It sends polling info notification to upper layer
 *
 * Parameters       p_data - Polling loop info notification
 *
 * Returns          void
 *
 ****************************************************************************/
void ReaderPollConfigParser::notifyPollingLoopInfoEvent(
    vector<uint8_t> p_data) {
  if (this->callback == NULL) return;

  vector<uint8_t> readerPollInfoNotifications;
  readerPollInfoNotifications.push_back(NCI_PROP_NTF_GID);
  readerPollInfoNotifications.push_back(NCI_PROP_NTF_ANDROID_OID);
  readerPollInfoNotifications.push_back((int)p_data.size() + 1);
  readerPollInfoNotifications.push_back(OBSERVE_MODE_OP_CODE);
  readerPollInfoNotifications.insert(std::end(readerPollInfoNotifications),
                                     std::begin(p_data), std::end(p_data));
  this->callback((int)readerPollInfoNotifications.size(),
                 readerPollInfoNotifications.data());
}

/*****************************************************************************
 *
 * Function         parseAndSendReaderPollInfo
 *
 * Description      Function to parse Lx Notification & Send Reader Poll info
 *                  notification
 *
 * Parameters       p_ntf - Lx Notification
 *                  p_len - Notification length
 *
 * Returns          This function return true in case of success
 *                  In case of failure returns false.
 *
 ****************************************************************************/
bool ReaderPollConfigParser::parseAndSendReaderPollInfo(uint8_t* p_ntf,
                                                        uint16_t p_len) {
  if (!p_ntf || (p_ntf && !isLxNotification(p_ntf, p_len))) {
    return false;
  }
  vector<uint8_t> lxNotification = vector<uint8_t>(p_ntf, p_ntf + p_len);
  uint16_t idx = NCI_MESSAGE_OFFSET;

  vector<uint8_t> readerPollInfoNotifications;
  while (idx < p_len) {
    uint8_t entryTag = ((lxNotification[idx] & LX_TAG_MASK) >> 4);
    uint8_t entryLength = (lxNotification[idx] & LX_LENGTH_MASK);

    idx++;
    if ((entryTag == L2_EVT_TAG || entryTag == CMA_EVT_TAG ||
         entryTag == CMA_EVT_EXTRA_DATA_TAG) &&
        lxNotification.size() >= (idx + entryLength)) {
      /*
        Reset the extra data bytes, If it receives other events while reading
        for unknown event chained frames
      */
      if (readExtraBytesForUnknownEvent &&
          (entryTag == L2_EVT_TAG || entryTag == CMA_EVT_TAG)) {
        resetExtraBytesInfo();
      }
      vector<uint8_t> readerPollInfo =
          getEvent(vector<uint8_t>(lxNotification.begin() + idx,
                                   lxNotification.begin() + idx + entryLength),
                   entryTag);

      if ((int)(readerPollInfoNotifications.size() + readerPollInfo.size()) >=
          0xFF) {
        notifyPollingLoopInfoEvent(std::move(readerPollInfoNotifications));
        readerPollInfoNotifications.clear();
      }
      readerPollInfoNotifications.insert(std::end(readerPollInfoNotifications),
                                         std::begin(readerPollInfo),
                                         std::end(readerPollInfo));
    }

    idx += entryLength;
  }

  if (readerPollInfoNotifications.size() <= 0 ||
      readerPollInfoNotifications.size() >= 0xFF) {
    return false;
  }

  notifyPollingLoopInfoEvent(std::move(readerPollInfoNotifications));

  return true;
}

/*****************************************************************************
 *
 * Function         parseAndSendReaderPollInfo
 *
 * Description      Function to check it is Lx Notification or not
 *
 * Parameters       p_ntf - Lx Notification
 *                  p_len - Notification length
 *
 * Returns          This function return true if it is Lx otherwise false
 *
 ****************************************************************************/
bool ReaderPollConfigParser::isLxNotification(uint8_t* p_ntf, uint16_t p_len) {
  return (p_ntf && p_len >= 2 && p_ntf[NCI_GID_INDEX] == NCI_PROP_NTF_GID &&
          p_ntf[NCI_OID_INDEX] == NCI_PROP_LX_NTF_OID);
}

/*****************************************************************************
 *
 * Function         setReaderPollCallBack
 *
 * Description      Function to set the callback, It will be used to notify
 *                  each reader polling data notifications
 *
 * Parameters       callback - nfc data callback object
 *
 * Returns          void
 *
 ****************************************************************************/
void ReaderPollConfigParser::setReaderPollCallBack(
    reader_poll_info_callback_t* callback) {
  this->callback = callback;
}

/*****************************************************************************
 *
 * Function         resetExtraBytesInfo
 *
 * Description      Function to reset the extra bytes info of UnknownEvent
 *
 * Parameters       None
 *
 * Returns          void
 *
 ****************************************************************************/
void ReaderPollConfigParser::resetExtraBytesInfo() {
  readExtraBytesForUnknownEvent = false;
  extraByteLength = 0;
  extraBytes = vector<uint8_t>();
  unknownEventTimeStamp = vector<uint8_t>();
}

/*****************************************************************************
 *
 * Function         setNotificationType
 *
 * Description      Function to select the Notification type for Observe mode
 *                  By default all type of notification enabled if not set
 *
 * Parameters       None
 *
 * Returns          void
 *
 ****************************************************************************/
void ReaderPollConfigParser::setNotificationType(uint8_t notificationType) {
  this->notificationType = notificationType;
}
