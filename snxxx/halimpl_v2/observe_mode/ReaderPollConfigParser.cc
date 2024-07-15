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
 * Function         getEvent
 *
 * Description      It identifies the type of event and gets the reader poll
 *                  info
 *                  notification
 *
 * Parameters       p_event - Vector Lx Notification
 *                  isCmaEvent - true if it CMA event otherwise false
 *
 * Returns          This function return reader poll info notification
 *
 ****************************************************************************/
vector<uint8_t> ReaderPollConfigParser::getEvent(vector<uint8_t> p_event,
                                                 bool isCmaEvent) {
  vector<uint8_t> event_data;
  if ((!isCmaEvent && (int)p_event.size() < MIN_LEN_NON_CMA_EVT) ||
      (isCmaEvent && (int)p_event.size() < MIN_LEN_CMA_EVT)) {
    return event_data;
  }

  int idx = 0;
  vector<uint8_t> timestamp;
  timestamp.push_back(p_event[idx++]);
  timestamp.push_back(p_event[idx++]);
  timestamp.push_back(p_event[idx++]);
  timestamp.push_back(p_event[idx]);
  if (!isCmaEvent) {
    idx += 2;
    lastKnownGain = p_event[idx];
    switch (p_event[INDEX_OF_L2_EVT_TYPE] & LX_TYPE_MASK) {
      // Trigger Type
      case L2_EVENT_TRIGGER_TYPE:
        // Modulation detected
        switch ((p_event[INDEX_OF_L2_EVT_TYPE] & LX_EVENT_MASK) >> 4) {
          case EVENT_MOD_A:
            lastKnownModEvent = EVENT_MOD_A;
            event_data = getWellKnownModEventData(
                TYPE_MOD_A, std::move(timestamp), lastKnownGain);
            break;

          case EVENT_MOD_B:
            lastKnownModEvent = EVENT_MOD_B;
            event_data = getWellKnownModEventData(
                TYPE_MOD_B, std::move(timestamp), lastKnownGain);
            break;

          case EVENT_MOD_F:
            lastKnownModEvent = EVENT_MOD_F;
            event_data = getWellKnownModEventData(
                TYPE_MOD_F, std::move(timestamp), lastKnownGain);
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

  } else {
    switch (p_event[INDEX_OF_CMA_EVT_TYPE]) {
      // Trigger Type
      case CMA_EVENT_TRIGGER_TYPE:
        switch (p_event[INDEX_OF_CMA_EVT_DATA]) {
          case REQ_A:
            event_data = getWellKnownModEventData(
                TYPE_MOD_A, std::move(timestamp), lastKnownGain, {REQ_A});
            break;

          case WUP_A:
            event_data = getWellKnownModEventData(
                TYPE_MOD_A, std::move(timestamp), lastKnownGain, {WUP_A});
            break;
          default:
            event_data = getUnknownEvent(
                vector<uint8_t>(p_event.begin() + INDEX_OF_CMA_EVT_DATA,
                                p_event.end()),
                std::move(timestamp), lastKnownGain);
        }
        break;
      case CMA_DATA_TRIGGER_TYPE: {
        uint8_t entryLength = p_event[INDEX_OF_CMA_EVT_DATA];
        if (p_event.size() >= INDEX_OF_CMA_EVT_DATA + entryLength) {
          vector<uint8_t> payloadData = vector<uint8_t>(
              p_event.begin() + INDEX_OF_CMA_DATA, p_event.end());

          if (lastKnownModEvent == EVENT_MOD_B &&
              payloadData[0] == TYPE_B_APF) {  // Type B Apf value is 0x05
            event_data =
                getWellKnownModEventData(TYPE_MOD_B, std::move(timestamp),
                                         lastKnownGain, std::move(payloadData));
            break;
          } else if (lastKnownModEvent == EVENT_MOD_F &&
                     payloadData[0] == TYPE_F_CMD_LENGH &&
                     payloadData[2] == TYPE_F_ID &&
                     payloadData[3] == TYPE_F_ID) {
            event_data =
                getWellKnownModEventData(TYPE_MOD_F, std::move(timestamp),
                                         lastKnownGain, std::move(payloadData));
            break;
          } else {
            event_data = getUnknownEvent(std::move(payloadData),
                                         std::move(timestamp), lastKnownGain);
            break;
          }
        }
        [[fallthrough]];
      }
      default:
        vector<uint8_t> payloadData = vector<uint8_t>(
            p_event.begin() + INDEX_OF_CMA_EVT_TYPE, p_event.end());
        event_data = getUnknownEvent(std::move(payloadData),
                                     std::move(timestamp), lastKnownGain);
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
    if ((entryTag == L2_EVT_TAG || entryTag == CMA_EVT_TAG) &&
        lxNotification.size() >= (idx + entryLength)) {
      vector<uint8_t> readerPollInfo =
          getEvent(vector<uint8_t>(lxNotification.begin() + idx,
                                   lxNotification.begin() + idx + entryLength),
                   entryTag == CMA_EVT_TAG);
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
