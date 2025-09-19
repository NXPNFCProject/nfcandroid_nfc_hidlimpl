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

#ifndef NFC_WRITER_H
#define NFC_WRITER_H

#include <phNxpNciHal_utils.h>
#include <phTmlNfc.h>

/******************************************************************************
 * Class         NfcWriter
 *
 * Description   The NfcWriter class is a singleton class responsible for
 *               writing data to an NFC Controller (NFCC) through a physical
 *               interface (e.g., I2C) using the NFCC driver interface.
 *               It ensures that only one instance of the class exists and
 *               provides various methods to handle the writing process,
 *including direct writes, enqueued writes, and handling write callbacks.
 *
 ******************************************************************************/

class NfcWriter {
 public:
  NfcWriter(const NfcWriter&) = delete;
  NfcWriter& operator=(const NfcWriter&) = delete;

  /******************************************************************************
   * Function         getInstance
   *
   * Description      This function is to provide the instance of NfcWriter
   *                  singleton class
   *
   * Returns          It returns instance of NfcWriter singleton class.
   *
   ******************************************************************************/
  static NfcWriter& getInstance();

  /******************************************************************************
   * Function         write
   *
   * Description      This function write the data to NFCC through physical
   *                  interface (e.g. I2C) using the NFCC driver interface.
   *                  Before sending the data to NFCC, phNxpNciHal_write_ext
   *                  is called to check if there is any extension processing
   *                  is required for the NCI packet being sent out.
   *
   * Returns          It returns number of bytes successfully written to NFCC.
   *
   ******************************************************************************/
  int write(uint16_t data_len, const uint8_t* p_data);

  /******************************************************************************
   * Function         direct_write
   *
   * Description      This function write the data to NFCC through physical
   *                  interface (e.g. I2C) using the NFCC driver interface.
   *                  Before sending the data to NFCC, phNxpNciHal_write_ext
   *                  is called to check if there is any extension processing
   *                  is required for the NCI packet being sent out.
   *
   * Returns          It returns number of bytes successfully written to NFCC.
   *
   ******************************************************************************/
  int direct_write(uint16_t data_len, const uint8_t* p_data);

  /******************************************************************************
   * Function         write_unlocked
   *
   * Description      This is the actual function which is being called by
   *                  write. This function writes the data to NFCC.
   *                  It waits till write callback provide the result of write
   *                  process.
   *
   * Returns          It returns number of bytes successfully written to NFCC.
   *
   ******************************************************************************/
  int write_unlocked(uint16_t data_len, const uint8_t* p_data, int origin);

  /******************************************************************************
   * Function         check_ncicmd_write_window
   *
   * Description      This function is called to check the write synchronization
   *                  status if write already acquired then wait for
   corresponding read to complete.
   *
   * Returns          return 0 on success and -1 on fail.
   *
   ******************************************************************************/
  int check_ncicmd_write_window(uint16_t cmd_len, uint8_t* p_cmd);

 private:
  /******************************************************************************
   * Function         Default constructor
   *
   * Description      Default constructor of NfcWriter singleton class
   *
   ******************************************************************************/
  NfcWriter();
};

#endif  // NFC_WRITER_H
