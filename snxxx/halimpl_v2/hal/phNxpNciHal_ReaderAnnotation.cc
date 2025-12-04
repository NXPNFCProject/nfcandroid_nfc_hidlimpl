/*
 * Copyright 2025 NXP
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

#include <cstdint>
#include <vector>

using namespace std;

/**
 * Converts annotation data to broadcast poll command format.
 *
 * This function parses input annotation data and converts it into a broadcast
 * poll command format suitable for NFC communication. The input data contains
 * annotation information, RF technology parameters, and extra settings that
 * are reformatted into the expected output structure.
 *
 * Input format:
 * - Bytes 0-1: Command header (0x2F, 0x0C)
 * - Byte 2: Length
 * - Byte 3: Operation code (should be 0x09)
 * - Byte 4: Number of entries
 * - For each entry:
 *   - Byte N: Position type (RF technology type identifier)
 *   - Byte N+1: Annotation length + 1
 *   - Byte N+2: Waiting time
 *   - Next bytes: Annotation data
 *   - Next byte: Extra settings length
 *   - Next bytes: Extra settings data
 *
 * Output format:
 * - Bytes 0-1: Command header (0x21, 0x1A)
 * - Byte 2: Output length (single byte)
 * - Byte 3: Number of entries
 * - For each entry:
 *   - Byte N: RF technology type (derived from position_type)
 *   - Byte N+1: Parameter length
 *   - Byte N+2: Control message flags
 *   - Byte N+3: Annotation data length
 *   - Next bytes: Annotation data
 *   - Next byte: Proprietary value length
 *   - Next bytes: Proprietary value data
 *
 * @param data_len Length of the input data buffer
 * @param p_data Pointer to the input data buffer containing annotation data
 * @return vector<uint8_t> Formatted broadcast poll command data, empty if input
 * is invalid
 */
vector<uint8_t> covertAnnotationToBrodcastPollCommand(uint16_t data_len,
                                                     const uint8_t* p_data) {
  vector<uint8_t> result;
  if (data_len < 5 || p_data == nullptr) {
    return result;
  }

  // Validate command header
  if (p_data[0] != 0x2F || p_data[1] != 0x0C) {
    return result;
  }

  // Parse input data
  const uint8_t num_entries = p_data[4];

  // Build output format
  result.push_back(0x21);
  result.push_back(0x1A);

  // Reserve space for length (will be filled later)
  const size_t length_pos = result.size();
  result.push_back(0x00);  // Placeholder for length

  // Number of entries
  result.push_back(num_entries);

  if (num_entries == 0) {
    // Disable broadcast poll command case
    const uint8_t output_len = 1;  // Only num_entries byte
    result[length_pos] = output_len;
    return result;
  }

  uint16_t input_offset = 5;  // Start after header + opcode + num_entries
  // Process each entry
  for (uint8_t entry = 0; entry < num_entries; entry++) {
    // Check if we have enough data for this entry header
    if (input_offset + 3 > data_len) {
      return vector<uint8_t>();  // Return empty vector on error
    }

    const uint8_t position_type = p_data[input_offset];
    const uint8_t annotation_plus_wait_length = p_data[input_offset + 1];
    const uint8_t waiting_time = p_data[input_offset + 2];

    input_offset += 3;

    // Calculate annotation data length
    const uint8_t annotation_len =
        annotation_plus_wait_length > 0 ? annotation_plus_wait_length - 1 : 0;

    // Check if we have enough data for annotation + extra settings length
    if (input_offset + annotation_len + 1 > data_len) {
      return vector<uint8_t>();  // Return empty vector on error
    }

    const uint8_t* annotation_data = &p_data[input_offset];
    input_offset += annotation_len;

    const uint8_t proprietary_setting_length = p_data[input_offset];
    input_offset += 1;

    // Check if we have enough data for extra settings
    if (input_offset + proprietary_setting_length > data_len) {
      return vector<uint8_t>();  // Return empty vector on error
    }

    const uint8_t* proprietary_setting = &p_data[input_offset];
    input_offset += proprietary_setting_length;

    // RF Tech type - extract from position_type (position is upper nibble)
    const uint8_t rf_tech_type =
        (position_type & 0x0F);  // Convert position to tech type
    result.push_back(rf_tech_type);

    // Param length
    const uint8_t param_len = 1 + 1 + annotation_len + 1 + proprietary_setting_length;
    result.push_back(param_len);

    // Control message - Bit 0 for annotation data, Bit 7 for proprietary TLVs
    uint8_t control_msg = 0x80;  // Bit 7 set for proprietary TLVs
    if (annotation_len > 0) {
      control_msg |= 0x01;  // Bit 0 set for annotation data
    }
    result.push_back(control_msg);

    // Length of annotation data
    result.push_back(annotation_len);

    // Annotation data
    for (int i = 0; i < annotation_len; i++) {
      result.push_back(annotation_data[i]);
    }

    // Proprietary Value Len
    result.push_back(proprietary_setting_length);

    // Proprietary Value (Extra settings data)
    for (int i = 0; i < proprietary_setting_length; i++) {
      result.push_back(proprietary_setting[i]);
    }
  }

  // Calculate and set the output length (excluding header and length byte
  // itself)
  const uint8_t output_len =
      result.size() -
      3;  // Total size minus header (2 bytes) and length byte (1 byte)
  result[length_pos] = output_len;

  return result;
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
