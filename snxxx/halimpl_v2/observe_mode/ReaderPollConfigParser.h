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

#include <vector>

using namespace std;

typedef void(reader_poll_info_callback_t)(uint16_t data_len, uint8_t* p_data);

/**
 * @brief This class handles the parsing of Lx notifications and
 * send reader poll info notification's. It identifis A, B and F
 * Modulation event's and RF ON & OFF event's, all the other
 * notifications it considers it as Unknown event's
 *
 */
class ReaderPollConfigParser {
 private:
  reader_poll_info_callback_t* callback = nullptr;
  uint8_t lastKnownGain = 0x00;
  uint8_t lastKnownModEvent = 0x00;

  /*****************************************************************************
   *
   * Function         getWellKnownModEventData
   *
   * Description      Frames Well known type reader poll info notification
   *
   * Parameters       event - Event type A, B & F
   *                  timeStamp - time stamp of the event
   *                  gain - RSSI value
   *                  data - data contains REQ, WUP and AFI
   *
   * Returns          Returns Well known type reader poll info notification
   *
   ****************************************************************************/
  vector<uint8_t> getWellKnownModEventData(uint8_t event,
                                           vector<uint8_t> timeStamp,
                                           uint8_t gain, vector<uint8_t> data);

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
  vector<uint8_t> getRFEventData(vector<uint8_t> timeStamp, uint8_t gain,
                                 bool rfState);

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
  vector<uint8_t> getUnknownEvent(vector<uint8_t> data,
                                  vector<uint8_t> timeStamp, uint8_t gain);

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
  vector<uint8_t> getEvent(vector<uint8_t> p_event, bool isCmaEvent);

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
  void notifyPollingLoopInfoEvent(vector<uint8_t> p_data);

#if (NXP_UNIT_TEST == TRUE)
  /*
    Friend class is used to test private function's of ReaderPollConfigParser
  */
  friend class ReaderPollConfigParserTest;
#endif

 public:
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
  bool parseAndSendReaderPollInfo(uint8_t* p_ntf, uint16_t p_len);

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
  bool isLxNotification(uint8_t* p_ntf, uint16_t p_len);

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
  void setReaderPollCallBack(reader_poll_info_callback_t* callback);
};
