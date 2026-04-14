/*
 * Copyright 2025-2026 NXP
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

#include "phNxpNciHal_ReaderAnnotation.h"

#include <phNxpLog.h>
#include <cstdint>
#include <vector>

using namespace std;

/**
 * Structure to hold parsed entry data from Google format
 */
struct ReaderAnnotationData {
  uint8_t position_type;               // Google format: Position type byte
  uint8_t annotation_len;              // Calculated annotation length
  const uint8_t* annotation_data;      // Pointer to annotation data
  bool has_proprietary_field;          // Flag if proprietary field exists
  uint8_t proprietary_setting_length;  // Extra settings length
  const uint8_t* proprietary_setting;  // Pointer to extra settings data
};

/**
 * Updates the Protocol Type (RF Tech Type) in NCI format.
 *
 * Google Format => NCI Format conversion:
 * - Google: position_type (lower nibble) = RF technology identifier
 * - NCI: rf_tech_type = position_type & 0x0F
 * - Special case: 0x00 is converted to 0x08
 *
 * Example:
 * - Google: position_type = 0x20 => NCI: rf_tech_type = 0x08 (0x20 & 0x0F =
 * 0x00 => 0x08)
 * - Google: position_type = 0x33 => NCI: rf_tech_type = 0x03 (0x33 & 0x0F =
 * 0x03)
 *
 * @param nciFmtReaderAnnoByte Output vector to append RF tech type
 * @param entry Parsed entry data containing position_type
 */
static void updateProtocolType(vector<uint8_t>& nciFmtReaderAnnoByte,
                               const ReaderAnnotationData& entry) {
  // Google Format => NCI Format: Extract RF Tech type from lower nibble
  uint8_t rf_tech_type = (entry.position_type & 0x0F);

  // Special case: if rf_tech_type is 0x00, convert to 0x08
  if (rf_tech_type == 0x00) {
    rf_tech_type = 0x08;
  }

  nciFmtReaderAnnoByte.push_back(rf_tech_type);
}

/**
 * Updates the Control Message byte in NCI format.
 *
 * Google Format => NCI Format conversion:
 * - Bit 0: Set if annotation_len > 0 (Broadcast Poll LVs present)
 * - Bit 1: Set if transmit_position > 0 (Broadcast Transmit Position LVs
 * present)
 * - Bit 7: Set if proprietary field exists (Proprietary LVs present)
 *
 * Example:
 * - Google: annotation_len=2, position_type=0x20, has_proprietary=true
 * - NCI: control_msg = 0x83 (0x80 | 0x02 | 0x01)
 *   - Bit 7 (0x80): Proprietary field exists
 *   - Bit 1 (0x02): Transmit position = 2 (upper nibble of 0x20)
 *   - Bit 0 (0x01): Annotation data present
 *
 * @param nciFmtReaderAnnoByte Output vector to append control message
 * @param entry Parsed entry data
 */
static void updateControlMessage(vector<uint8_t>& nciFmtReaderAnnoByte,
                                 const ReaderAnnotationData& entry) {
  // Google Format => NCI Format: Build control message flags
  uint8_t control_msg = 0x00;

  // Extract transmit position from upper nibble
  uint8_t transmit_position = (entry.position_type >> 4) & 0x0F;
  bool has_transmit_position = (transmit_position >= 2);

  // Bit 7: Set if proprietary field exists (even if length is 0)
  if (entry.has_proprietary_field) {
    control_msg |= 0x80;  // Proprietary LVs present
  }

  // Bit 1: Set if transmit position is present
  if (has_transmit_position) {
    control_msg |= 0x02;  // Broadcast Transmit Position LVs present
  }

  // Bit 0: Set if annotation data is present
  if (entry.annotation_len > 0) {
    control_msg |= 0x01;  // Broadcast Poll LVs present
  }

  nciFmtReaderAnnoByte.push_back(control_msg);
}

/**
 * Updates the Broadcast Poll Position (Annotation Data) in NCI format.
 *
 * Google Format => NCI Format conversion:
 * - Google: annotation_plus_wait_length - 1 = annotation_len
 * - Google: annotation_data[] = raw annotation bytes
 * - NCI: If annotation_len > 0:
 *   - Byte: annotation_len
 *   - Bytes: annotation_data[0..annotation_len-1]
 *
 * Example:
 * - Google: annotation_plus_wait_length=0x03, annotation_data={0xAA, 0xBB}
 * - NCI: 0x02, 0xAA, 0xBB
 *   - 0x02: Annotation data length (0x03 - 1)
 *   - 0xAA, 0xBB: Annotation data
 *
 * @param nciFmtReaderAnnoByte Output vector to append annotation data
 * @param entry Parsed entry data containing annotation information
 */
static void updateBroadcastPollPosition(vector<uint8_t>& nciFmtReaderAnnoByte,
                                        const ReaderAnnotationData& entry) {
  // Google Format => NCI Format: Add annotation data if present
  if (entry.annotation_len > 0) {
    // Length of annotation data
    nciFmtReaderAnnoByte.push_back(entry.annotation_len);

    // Annotation data bytes
    for (uint8_t i = 0; i < entry.annotation_len; i++) {
      nciFmtReaderAnnoByte.push_back(entry.annotation_data[i]);
    }
  }
}

/**
 * Updates the Transmit Position in NCI format.
 *
 * Google Format => NCI Format conversion:
 * - Google: position_type (upper nibble) = transmit position
 * - NCI: If transmit_position > 0:
 *   - Byte: 0x01 (Transmit Position length, always 1 byte)
 *   - Byte: transmit_position value
 *
 * Example:
 * - Google: position_type = 0x20 => transmit_position = 0x02 (upper nibble)
 * - NCI: 0x01, 0x02
 *   - 0x01: Transmit Position length
 *   - 0x02: Transmit Position value
 *
 * @param nciFmtReaderAnnoByte Output vector to append transmit position
 * @param entry Parsed entry data containing position_type
 */
static void updateTransmitPosition(vector<uint8_t>& nciFmtReaderAnnoByte,
                                   const ReaderAnnotationData& entry) {
  // Google Format => NCI Format: Extract and add transmit position
  uint8_t transmit_position = (entry.position_type >> 4) & 0x0F;

  if (transmit_position >= 2) {
    nciFmtReaderAnnoByte.push_back(
        0x01);  // Transmit Position length (always 1 byte)
    nciFmtReaderAnnoByte.push_back(transmit_position);
  }
}

/**
 * Updates the Proprietary Bytes (Extra Settings) in NCI format.
 *
 * Google Format => NCI Format conversion:
 * - Google: extra_settings_length = proprietary_setting_length
 * - Google: extra_settings_data[] = proprietary_setting[]
 * - NCI: If proprietary field exists:
 *   - Byte: proprietary_setting_length
 *   - Bytes: proprietary_setting[0..proprietary_setting_length-1]
 *
 * Example:
 * - Google: extra_settings_length=0x02, extra_settings_data={0xCC, 0xDD}
 * - NCI: 0x02, 0xCC, 0xDD
 *   - 0x02: Proprietary value length
 *   - 0xCC, 0xDD: Proprietary value data
 *
 * @param nciFmtReaderAnnoByte Output vector to append proprietary data
 * @param entry Parsed entry data containing proprietary information
 */
static void updateProprietaryBytes(vector<uint8_t>& nciFmtReaderAnnoByte,
                                   const ReaderAnnotationData& entry) {
  // Google Format => NCI Format: Add proprietary data if field exists
  if (entry.has_proprietary_field) {
    // Proprietary Value Length
    nciFmtReaderAnnoByte.push_back(entry.proprietary_setting_length);

    // Proprietary Value Data (Extra settings data)
    for (uint8_t i = 0; i < entry.proprietary_setting_length; i++) {
      nciFmtReaderAnnoByte.push_back(entry.proprietary_setting[i]);
    }
  }
}

/**
 * Calculates the parameter length for an entry in NCI format.
 *
 * Google Format => NCI Format conversion:
 * - NCI param_len includes:
 *   - 1 byte: control_msg (always present)
 *   - If annotation_len > 0: 1 + annotation_len bytes
 *   - If transmit_position > 0: 2 bytes (length + value)
 *   - If proprietary field exists: 1 + proprietary_setting_length bytes
 *
 * Example:
 * - Google: annotation_len=2, transmit_position=2, proprietary_len=2
 * - NCI: param_len = 1 + (1+2) + 2 + (1+2) = 9
 *   - 1: control_msg
 *   - 3: annotation (1 length + 2 data)
 *   - 2: transmit position (1 length + 1 value)
 *   - 3: proprietary (1 length + 2 data)
 *
 * @param entry Parsed entry data
 * @return uint8_t Calculated parameter length
 */
static uint8_t calculateParamLength(const ReaderAnnotationData& entry) {
  uint8_t param_len = 1;  // control_msg is always present

  // Add annotation data length
  if (entry.annotation_len > 0) {
    param_len += 1 + entry.annotation_len;  // annotation_len + annotation_data
  }

  // Add transmit position length
  uint8_t transmit_position = (entry.position_type >> 4) & 0x0F;
  if (transmit_position >= 2) {
    param_len += 1 + 1;  // transmit_position_len + transmit_position_value
  }

  // Add proprietary data length
  if (entry.has_proprietary_field) {
    param_len += 1 + entry.proprietary_setting_length;
  }

  return param_len;
}

/**
 * Parses a single entry from Google format input data.
 *
 * @param p_data Pointer to input data
 * @param data_len Total length of input data
 * @param input_offset Current offset in input data (will be updated)
 * @param entry Output structure to store parsed entry data
 * @return bool True if parsing successful, false otherwise
 */
static bool parseReaderAnnotationData(const uint8_t* p_data, uint16_t data_len,
                                      uint16_t& input_offset,
                                      ReaderAnnotationData& entry) {
  // Check if we have enough data for entry header
  if (input_offset + 3 > data_len) {
    NXPLOG_NCIHAL_E(
        "%s: Insufficient data for entry header - offset=%d, data_len=%d",
        __func__, input_offset, data_len);
    return false;
  }

  entry.position_type = p_data[input_offset];
  const uint8_t annotation_plus_wait_length = p_data[input_offset + 1];
  // const uint8_t waiting_time = p_data[input_offset + 2];  // Not used in
  // conversion

  input_offset += 3;

  // Calculate annotation data length
  entry.annotation_len =
      annotation_plus_wait_length > 0 ? annotation_plus_wait_length - 1 : 0;

  // Check if we have enough data for annotation data
  if (input_offset + entry.annotation_len > data_len) {
    NXPLOG_NCIHAL_E(
        "%s: Insufficient data for annotation - offset=%d, annotation_len=%d, "
        "data_len=%d",
        __func__, input_offset, entry.annotation_len, data_len);
    return false;
  }

  entry.annotation_data = &p_data[input_offset];
  input_offset += entry.annotation_len;

  // Initialize proprietary fields
  entry.proprietary_setting_length = 0;
  entry.proprietary_setting = nullptr;
  entry.has_proprietary_field = false;

  // Check if we have more data for proprietary settings
  if (input_offset < data_len) {
    entry.has_proprietary_field = true;
    entry.proprietary_setting_length = p_data[input_offset];
    input_offset += 1;

    // Check if we have enough data for extra settings
    if (input_offset + entry.proprietary_setting_length > data_len) {
      NXPLOG_NCIHAL_E(
          "%s: Insufficient data for proprietary settings - offset=%d, "
          "proprietary_len=%d, data_len=%d",
          __func__, input_offset, entry.proprietary_setting_length, data_len);
      return false;
    }

    if (entry.proprietary_setting_length > 0) {
      entry.proprietary_setting = &p_data[input_offset];
      input_offset += entry.proprietary_setting_length;
    }
  }

  return true;
}
/**
 * Converts annotation data to broadcast poll command format.
 *
 * Google Format => NCI Format conversion overview:
 *
 * Google Format (Input):
 * - Header: 0x2F, 0x0C
 * - Length, OpCode, NumEntries
 * - For each entry:
 *   - position_type (RF tech + transmit position)
 *   - annotation_plus_wait_length
 *   - waiting_time
 *   - annotation_data[]
 *   - extra_settings_length
 *   - extra_settings_data[]
 *
 * NCI Format (Output):
 * - Header: 0x21, 0x1A
 * - Length, NumEntries
 * - For each entry:
 *   - rf_tech_type
 *   - param_length
 *   - control_msg
 *   - [annotation_len, annotation_data[]]  (if present)
 *   - [transmit_pos_len, transmit_pos]     (if present)
 *   - [proprietary_len, proprietary_data[]] (if present)
 *
 * Example conversion (VALID_INPUT_BASIC => EXPECTED_OUTPUT_BASIC):
 * Google: {0x2F, 0x0C, 0x0A, 0x09, 0x01, 0x20, 0x03, 0x0A, 0xAA, 0xBB, 0x02,
 * 0xCC, 0xDD} NCI:    {0x21, 0x1A, 0x0C, 0x01, 0x08, 0x09, 0x83, 0x02, 0xAA,
 * 0xBB, 0x01, 0x02, 0x02, 0xCC, 0xDD}
 *
 * @param data_len Length of the input data buffer
 * @param p_data Pointer to the input data buffer containing annotation data
 * @return vector<uint8_t> Formatted broadcast poll command data, empty if input
 * is invalid
 */
vector<uint8_t> covertAnnotationToBrodcastPollCommand(uint16_t data_len,
                                                     const uint8_t* p_data) {
  vector<uint8_t> broadcastPollCmd;

  // Validate input
  if (data_len < 5 || p_data == nullptr) {
    NXPLOG_NCIHAL_E("%s: Invalid input parameters - data_len=%d, p_data=%p",
                    __func__, data_len, p_data);
    return broadcastPollCmd;
  }

  // Validate command header (Google format)
  if (p_data[0] != 0x2F || p_data[1] != 0x0C) {
    NXPLOG_NCIHAL_E(
        "%s: Invalid command header - expected [0x2F, 0x0C], got [0x%02X, "
        "0x%02X]",
        __func__, p_data[0], p_data[1]);
    return broadcastPollCmd;
  }

  // Parse input data
  const uint8_t num_entries = p_data[4];

  // Google Format => NCI Format: Build output header
  broadcastPollCmd.push_back(0x21);  // NCI command header byte 1
  broadcastPollCmd.push_back(0x1A);  // NCI command header byte 2

  // Reserve space for length (will be filled later)
  const size_t length_pos = broadcastPollCmd.size();
  broadcastPollCmd.push_back(0x00);  // Placeholder for length

  // Number of entries (same in both formats)
  broadcastPollCmd.push_back(num_entries);

  if (num_entries == 0) {
    // Disable broadcast poll command case
    const uint8_t output_len = 1;  // Only num_entries byte
    broadcastPollCmd[length_pos] = output_len;
    return broadcastPollCmd;
  }

  uint16_t input_offset = 5;  // Start after header + opcode + num_entries

  // Process each ReaderAnnotationData: Google Format => NCI Format
  for (uint8_t entry_idx = 0; entry_idx < num_entries; entry_idx++) {
    ReaderAnnotationData readerAnnotationData;

    // Parse ReaderAnnotation from Google format
    if (!parseReaderAnnotationData(p_data, data_len, input_offset,
                                   readerAnnotationData)) {
      NXPLOG_NCIHAL_E(
          "%s: Failed to parse reader annotation data at entry %d, offset %d",
          __func__, entry_idx, input_offset);
      return vector<uint8_t>();  // Return empty vector on error
    }

    // Google Format => NCI Format: Convert entry
    vector<uint8_t> nciFmtReaderAnnoByte;

    // 1. Update Protocol Type (RF Tech Type)
    updateProtocolType(nciFmtReaderAnnoByte, readerAnnotationData);

    // 2. Calculate and add Parameter Length
    uint8_t param_len = calculateParamLength(readerAnnotationData);
    nciFmtReaderAnnoByte.push_back(param_len);

    // 3. Update Control Message
    updateControlMessage(nciFmtReaderAnnoByte, readerAnnotationData);

    // 4. Update Broadcast Poll Position (Annotation Data)
    updateBroadcastPollPosition(nciFmtReaderAnnoByte, readerAnnotationData);

    // 5. Update Transmit Position
    updateTransmitPosition(nciFmtReaderAnnoByte, readerAnnotationData);

    // 6. Update Proprietary Bytes (Extra Settings)
    updateProprietaryBytes(nciFmtReaderAnnoByte, readerAnnotationData);

    // Append converted readerAnnotationData to broadcastPollCmd
    broadcastPollCmd.insert(broadcastPollCmd.end(),
                            nciFmtReaderAnnoByte.begin(),
                            nciFmtReaderAnnoByte.end());
  }

  // Calculate and set the output length (excluding header and length byte
  // itself)
  const uint8_t output_len =
      broadcastPollCmd.size() -
      3;  // Total size minus header (2 bytes) and length byte (1 byte)
  broadcastPollCmd[length_pos] = output_len;

  return broadcastPollCmd;
}

/**
 * Parses broadcast poll command response to extract status.
 *
 * This function parses the response from the broadcast poll command and
 * extracts the status byte. The expected successful response format is
 * "411A0100" where the last byte (0x00) indicates success.
 *
 * Expected response format:
 * - Byte 0: Response header (0x41)
 * - Byte 1: Command identifier (0x1A)
 * - Byte 2: Length (0x01)
 * - Byte 3: Status (0x00 = success, any other value = failure)
 *
 * @param rsp_len Length of the response data buffer
 * @param rsp_data Pointer to the response data buffer
 * @return uint8_t Status byte (0x00 = success, any other value = failure)
 */
uint8_t parseBroadcastPollCommandResponse(uint16_t rsp_len,
                                          const uint8_t* rsp_data) {
  // Default failure status
  uint8_t status = 0xFF;

  // Validate input parameters
  if (rsp_len < 4 || rsp_data == nullptr) {
    return status;
  }

  // Validate response format: "411A0100" for success
  // Check header and command identifier
  if (rsp_data[0] == 0x41 && rsp_data[1] == 0x1A && rsp_data[2] == 0x01) {
    // Extract status byte (last byte)
    status = rsp_data[3];
  }

  return status;
}
