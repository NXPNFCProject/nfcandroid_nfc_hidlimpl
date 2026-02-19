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

#pragma once
#include "IntervalTimer.h"
#include "phNxpLog.h"
#include "phNxpNciHal_utils.h"
#include "phTmlNfc.h"

#define DNLDV2_PCKT_FRAGMENT_LEN (4142)
#define DNLDV2_PCKT_LEN \
  (4146) /* 2 bytes Header + DNLDV2_PCKT_FRAGMENT_LEN + 2 bytes CRC */
#define DNLDV2_PCKT_START_IDX (0)
#define DNLDV2_HEADER_CRC_LEN (2)
#define DNLDV2_HDLL_OPCODE (3)
#define DNLDV2_TIMER_DURATION (2500)
#define DNLDV2_INTEGRITY_SUCCESS (0x55)
#define DNLDV2_INTEGRITY_FAILED (0xAA)
#define DNLDV2_PG_STATUS_FAILED (0x5A)
#define DNLDV2_PG_STATUS_DEGRADED (0xA5)

#define EDL_FIRST_OFFSET (0x01)
#define EDL_FOURTH_OFFSET (0x04)
#define EDL_THIRD_MSB_OFFSET (0xC0)
#define EDL_THIRD_LSB_OFFSET (0x20)
#define EDL_EIGHT_LSB_OFFSET (0X3F)
#define EDL_FIVE_LSB (0X1F)
#define EDL_HCP_GROUP_OFFSET (0X06)
#define EDL_CHUNK_OFFSET (0X05)
#define EDL_PCKT_BYTE (0XFF)
#define EDL_SHIFT_BYTE (0X08)
/*
 * Enum definition contains HCP types
 */
typedef enum SecureDwnldHcpType {
  HCP_CMD = 0x00,          /* Cmd */
  HCP_RESPONSE = 0x01,     /* Response */
  HCP_NOTIFICATION = 0x02, /* Notification */
  HCP_DATA = 0x03,         /* Data*/
} SecureDwnldHcpType_t;

/*
 * Enum definition contains different HCP groups
 */
typedef enum SecureDwnldHcpGroup {
  HCP_PROTOCOL = 0x01, /* Protocol group, available only in Response packets */
  HCP_GENERIC = 0x02,  /* Generic group */
  HCP_EDL = 0x03,      /* Encrypted Download group */
} SecureDwnldHcpGroup_t;

/*
 * Enum definition contains different HCP Protocol groups
 */
typedef enum SecureDwnldHcpGroupProtocol {
  GROUP_PROTOCOL_HDLL = 0x01,
  /* HDLL protocol */ /* This notification is sent when entering RX POLL to
                         notify the host that Volta is ready to receive a
                         command */
  GROUP_PROTOCOL_HCP = 0x02, /* HCP protocol  */
  GROUP_PROTOCOL_EDL = 0x03, /* Encrypted Download protocol */
} SecureDwnldHcpGroupProtocol_t;

/*
 * Enum definition contains different HCP Generic groups
 */
typedef enum SecureDwnldHcpGroupGeneric {
  GROUP_GENERIC_RESET = 0x01,    /* Reset the IC (Hard Soft Reset)  */
  GROUP_GENERIC_GET_INFO = 0x02, /* Request Chip Information  */
  GROUP_GENERIC_ENTER_DOWNLOAD_MODE =
      0x03, /* Request download mode entry for NXP app  */
  GROUP_GENERIC_ENTER_COMMIT_MODE = 0x05, /* Request commit mode entry  */
  GROUP_GENERIC_NFCC_MEASUREMENTS = 0x10,
  /* NFCC Measurement notification GENERIC_BOOT_MEASUREMENT */ /* Can be ignored
                                                                  in our case*/
} SecureDwnldHcpGroupGeneric_t;

/*
 * Enum definition contains different EDLL opcodes
 */
typedef enum SecureDwnldHcpEdl {
  GROUP_EDL_DOWNLOAD_CERTIFICATE_NXP =
      0x01, /* EDL Certificate Packet Command */
  GROUP_EDL_DOWNLOAD_WRITE_FIRST_NXP = 0x02, /* EDL Write First Command */
  GROUP_EDL_DOWNLOAD_NVM_WRITE_NXP = 0x03,   /* EDL NV Memory Write Command */
  GROUP_EDL_DOWNLOAD_NVM_WRITE_LAST_NXP =
      0x04, /* EDL NV Memory Write Last Command */
} SecureDwnldHcpEdl_t;

/*
 * Enum definition contains Boot status from get info response
 */
typedef enum SecureDwnldBootStatus {
  INIT_NOT_DONE = 0x00,
  INIT_OK = 0x01,
  INIT_WARNING = 0x02,
  INIT_ERROR = 0x04,
  INIT_WARNING_PLUS = 0x06,
} SecureDwnldBootStatus_t;

/*
 * Enum definition contains Protected page status from get info response
 */
typedef enum SecureDwnldProtectedPageStatus {
  PROTECTED_OK =
      0x55, /* Protected is OK & Scratch content = Protected Content  */
  SCRATCH_NOK =
      0xAA, /* Protected is OK & Scratch content != Protected Content */
  PROTECTED_NOK =
      0x5A,        /* Protected is NOK, but Scratch is OK (Commit required)  */
  BOTH_NOK = 0x04, /* Both Protected and Scratch are NOK (Degraded firmware
                      download required) */
} SecureDwnldProtectedPageStatus_t;