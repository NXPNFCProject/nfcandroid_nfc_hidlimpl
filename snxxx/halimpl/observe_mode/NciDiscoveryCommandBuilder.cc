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

#include "NciDiscoveryCommandBuilder.h"

#include <phNfcNciConstants.h>

using namespace std;

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
bool NciDiscoveryCommandBuilder::parse(vector<uint8_t> data) {
  if (!isDiscoveryCommand(data)) return false;

  mRfDiscoverConfiguration.clear();
  int dataSize = (int)data.size();

  if (dataSize <= NCI_HEADER_MIN_LEN &&
      (dataSize < ((data[RF_DISC_CMD_NO_OF_CONFIG_INDEX] *
                    RF_DISC_CMD_EACH_CONFIG_LENGTH) +
                   NCI_HEADER_MIN_LEN))) {
    return false;
  }

  for (int i = RF_DISC_CMD_CONFIG_START_INDEX; i < ((int)data.size() - 1);
       i = i + RF_DISC_CMD_EACH_CONFIG_LENGTH) {
    mRfDiscoverConfiguration.push_back(
        DiscoveryConfiguration(data[i], data[i + 1]));
  }
  return true;
}

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
bool NciDiscoveryCommandBuilder::isDiscoveryCommand(vector<uint8_t> data) {
  if ((int)data.size() >= 2 && data[NCI_GID_INDEX] == NCI_RF_DISC_COMMD_GID &&
      data[NCI_OID_INDEX] == NCI_RF_DISC_COMMAND_OID) {
    return true;
  }
  return false;
}

/*****************************************************************************
 *
 * Function         removeListenParams
 *
 * Description      Removes the listen mode from the configuration list
 *
 * Returns          void
 *
 ****************************************************************************/
void NciDiscoveryCommandBuilder::removeListenParams() {
  std::vector<DiscoveryConfiguration>::iterator configIterator =
      mRfDiscoverConfiguration.begin();

  while (configIterator != mRfDiscoverConfiguration.end()) {
    if (NFC_A_PASSIVE_LISTEN_MODE == configIterator->mRfTechMode ||
        NFC_B_PASSIVE_LISTEN_MODE == configIterator->mRfTechMode ||
        NFC_F_PASSIVE_LISTEN_MODE == configIterator->mRfTechMode ||
        NFC_ACTIVE_LISTEN_MODE == configIterator->mRfTechMode) {
      configIterator = mRfDiscoverConfiguration.erase(configIterator);
    } else {
      ++configIterator;
    }
  }
}

/*****************************************************************************
 *
 * Function         addObserveModeParams
 *
 * Description      Adds Observe mode to the config list
 *
 * Returns          void
 *
 ****************************************************************************/
void NciDiscoveryCommandBuilder::addObserveModeParams() {
  mRfDiscoverConfiguration.push_back(
      DiscoveryConfiguration(OBSERVE_MODE, OBSERVE_MODE_DISCOVERY_CYCLE));
}

/*****************************************************************************
 *
 * Function         build
 *
 * Description      It frames the RF discovery command from the config list
 *
 * Returns          return the discovery command
 *
 ****************************************************************************/
vector<uint8_t> NciDiscoveryCommandBuilder::build() {
  vector<uint8_t> discoveryCommand;
  discoveryCommand.push_back(NCI_RF_DISC_COMMD_GID);
  discoveryCommand.push_back(NCI_RF_DISC_COMMAND_OID);
  int discoveryLength = (mRfDiscoverConfiguration.size() * 2) + 1;
  int numberOfConfigurations = mRfDiscoverConfiguration.size();
  discoveryCommand.push_back(discoveryLength);
  discoveryCommand.push_back(numberOfConfigurations);
  for (int i = 0; i < numberOfConfigurations; i++) {
    discoveryCommand.push_back(mRfDiscoverConfiguration[i].mRfTechMode);
    discoveryCommand.push_back(mRfDiscoverConfiguration[i].mDiscFrequency);
  }
  return discoveryCommand;
}

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
vector<uint8_t> NciDiscoveryCommandBuilder::reConfigRFDiscCmd(
    uint16_t data_len, const uint8_t* p_data) {
  if (!p_data) {
    return vector<uint8_t>();
  }

  vector<uint8_t> discoveryCommand = vector<uint8_t>(p_data, p_data + data_len);
  bool status = parse(std::move(discoveryCommand));
  if (status) {
    removeListenParams();
    addObserveModeParams();
    return build();
  } else {
    return vector<uint8_t>();
  }
}
