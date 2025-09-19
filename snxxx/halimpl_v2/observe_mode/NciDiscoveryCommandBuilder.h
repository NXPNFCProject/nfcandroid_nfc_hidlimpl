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

#include <vector>

using std::vector;

#define NciDiscoveryCommandBuilderInstance \
  (NciDiscoveryCommandBuilder::getInstance())

/**
 * @brief DiscoveryConfiguration is the data class
 * which holds RF tech mode and Disc Frequency values
 */
class DiscoveryConfiguration {
 public:
  uint8_t mRfTechMode;
  uint8_t mDiscFrequency;
  DiscoveryConfiguration(uint8_t rfTechMode, uint8_t discFrequency) {
    mRfTechMode = rfTechMode;
    mDiscFrequency = discFrequency;
  };
};

/**
 * @brief NciDiscoveryCommandBuilder class handles the RF Discovery
 * command, parses the command and identifies the RF tech mode
 * and frequency. It helps to alter the configuration and build
 * the command
 */
class NciDiscoveryCommandBuilder {
 private:
  uint8_t currentObserveModeTech = 0x00;
  vector<uint8_t> currentDiscoveryCommand;
  vector<DiscoveryConfiguration> mRfDiscoverConfiguration;
  bool mIsRfDiscoveryReceived;

  /*****************************************************************************
   *
   * Function         parse
   *
   * Description      It parse the RF discovery commands and filters find the
   *                  configurations
   *
   * Parameters       data - RF discovery command
   *
   * Returns          return true if parse is successful otherwise false
   *
   ****************************************************************************/
  bool parse(vector<uint8_t> data);

  /*****************************************************************************
   *
   * Function         removeListenParams
   *
   * Description      Removes the listen mode from the configuration list
   *
   * Returns          void
   *
   ****************************************************************************/
  void removeListenParams();

  /*****************************************************************************
   *
   * Function         addObserveModeParams
   *
   * Description      Adds Observe mode to the config list
   *
   * Returns          void
   *
   ****************************************************************************/
  void addObserveModeParams();

  /*****************************************************************************
   *
   * Function         build
   *
   * Description      It frames the RF discovery command from the config list
   *
   * Returns          return the discovery command
   *
   ****************************************************************************/
  vector<uint8_t> build();

  /*****************************************************************************
   *
   * Function         isDiscoveryCommand
   *
   * Description      Checks the command is RF discovery command or not
   *
   * Parameters       data - Any command
   *
   * Returns          return true if the command is RF discovery command
   *                  otherwise false
   *
   ****************************************************************************/
  bool isDiscoveryCommand(vector<uint8_t> data);

#if (NXP_UNIT_TEST == TRUE)
  /*
    Friend class is used to test private function's of
    NciDiscoveryCommandBuilder
  */
  friend class NciDiscoveryCommandBuilderTest;
#endif
 public:
  /*****************************************************************************
   *
   * Function         setDiscoveryCommand
   *
   * Description      It sets the current discovery command
   *
   * Parameters       data - RF discovery command
   *
   * Returns          return void
   *
   ****************************************************************************/
  void setDiscoveryCommand(uint16_t data_len, const uint8_t* p_data);

  /*****************************************************************************
   *
   * Function         getDiscoveryCommand
   *
   * Description      It returns the current discovery command
   *
   * Returns          return current discovery command which is set
   *
   ****************************************************************************/
  vector<uint8_t> getDiscoveryCommand();

  /*****************************************************************************
   *
   * Function         reConfigRFDiscCmd
   *
   * Description      It parse the discovery command and alter the configuration
   *                  to enable Observe Mode
   *
   * Parameters       data - RF discovery command
   *
   * Returns          return the discovery command for Observe mode
   *
   ****************************************************************************/
  vector<uint8_t> reConfigRFDiscCmd();

  /*****************************************************************************
   *
   * Function         setObserveModePerTech
   *
   * Description      Sets ObserveMode tech mode
   *
   * Parameters       techMode - ObserveMode tech mode
   *
   * Returns          return void
   *
   ****************************************************************************/
  void setObserveModePerTech(uint8_t techMode);

  /*****************************************************************************
   *
   * Function         getCurrentObserveModeTechValue
   *
   * Description      gets Current ObserveMode per tech
   *
   * Returns          return Current Observe mode tech
   *
   ****************************************************************************/
  uint8_t getCurrentObserveModeTechValue();

  /*****************************************************************************
   *
   * Function         setRfDiscoveryReceived
   *
   * Description      Set flags when it receives discovery command received,
   *                  set to false during nfc init begin
   *
   * Returns          return void
   *
   ****************************************************************************/
  void setRfDiscoveryReceived(bool flag);

  /*****************************************************************************
   *
   * Function         isRfDiscoveryCommandReceived
   *
   * Description      returns true if discovery command set otherwise false
   *
   * Returns          return bool
   *
   ****************************************************************************/
  bool isRfDiscoveryCommandReceived();

  static NciDiscoveryCommandBuilder& getInstance();
};
