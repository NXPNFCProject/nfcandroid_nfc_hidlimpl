/******************************************************************************
 *
 *  Copyright 2022-2025 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <stdint.h>

#include <string>

#include "phNxpConfig.h"
#ifndef NXP_FEATURES_H
#define NXP_FEATURES_H

#define STRMAX_2 100

/*FW ROM CODE VERSION*/
#define FW_MOBILE_ROM_VERSION_PN551 0x10
#define FW_MOBILE_ROM_VERSION_PN553 0x11
#define FW_MOBILE_ROM_VERSION_PN557 0x12
#define FW_MOBILE_ROM_VERSION_SN100U 0x01
#define FW_MOBILE_ROM_VERSION_SN220U 0x01
#define FW_MOBILE_ROM_VERSION_SN300U 0x02

/*FW Major VERSION Code*/
#define FW_MOBILE_MAJOR_NUMBER_PN553 0x01
#define FW_MOBILE_MAJOR_NUMBER_PN551 0x05
#define FW_MOBILE_MAJOR_NUMBER_PN48AD 0x01
#define FW_MOBILE_MAJOR_NUMBER_PN81A 0x02
#define FW_MOBILE_MAJOR_NUMBER_PN557 0x01
#define FW_MOBILE_MAJOR_NUMBER_PN557_V2 0x21
#define FW_MOBILE_MAJOR_NUMBER_SN100U 0x010
#define FW_MOBILE_MAJOR_NUMBER_SN220U 0x01
#define FW_MOBILE_MAJOR_NUMBER_SN300U 0x20

#define NCI_CMD_RSP_SUCCESS_SW1 0x60
#define NCI_CMD_RSP_SUCCESS_SW2 0x00
#define FW_DL_RSP_FIRST_BYTE 0x00

/* Supported EE */
#define EE_T4T_SUPPORTED 1
#define EE_UICC1_SUPPORTED 1
#define EE_UICC2_SUPPORTED 1
#define EE_UICC3_SUPPORTED 1
#define EE_ESE_SUPPORTED 1
#define EE_EUICC1_SUPPORTED 1
#define EE_EUICC2_SUPPORTED 1

#define JCOP_VER_3_3 3
#define JCOP_VER_4_0 4
#ifndef FW_LIB_ROOT_DIR
#if (defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64))
#define FW_LIB_ROOT_DIR "/vendor/lib64/"
#else
#define FW_LIB_ROOT_DIR "/vendor/lib/"
#endif
#endif
#ifndef FW_BIN_ROOT_DIR
#define FW_BIN_ROOT_DIR "/vendor/firmware/"
#endif
#ifndef FW_LIB_EXTENSION
#define FW_LIB_EXTENSION ".so"
#endif
#ifndef FW_BIN_EXTENSION
#define FW_BIN_EXTENSION ".bin"
#endif

enum tNFCC_DnldType_enum : std::uint8_t {
  NFCC_DWNLD_WITH_VEN_RESET,
  NFCC_DWNLD_WITH_NCI_CMD
};
enum tNFC_HWVersion_enum : std::uint8_t {
  HW_PN553_A0 = 0x40,
  HW_PN553_B0 = 0x41,
  HW_PN80T_A0 = 0x50,
  HW_PN80T_B0 = 0x51,  // PN553 B0 + P73
  HW_PN551 = 0x98,
  HW_PN67T_A0 = 0xA8,
  HW_PN67T_B0 = 0x08,
  HW_PN548C2_A0 = 0x28,
  HW_PN548C2_B0 = 0x48,
  HW_PN66T_A0 = 0x18,
  HW_PN66T_B0 = 0x58,
  HW_SN100U_A0 = 0xA0,
  HW_SN100U_A2 = 0xA2,
  HW_PN560_V1 = 0xCA,
  HW_PN560_V2 = 0xCB,
  HW_SN300U = 0xD3
};
enum tNFC_chipType_enum : std::uint8_t {
  DEFAULT_CHIP_TYPE = 0x00,
  pn547C2,
  pn65T,
  pn548C2,
  pn66T,
  pn551,
  pn67T,
  pn553,
  pn80T,
  pn557,
  pn81T,
  sn100u,
  sn220u,
  pn560,
  sn300u
};

using tNFCC_DnldType = tNFCC_DnldType_enum;
using tNFC_HWVersion = tNFC_HWVersion_enum;
using tNFC_chipType = tNFC_chipType_enum;

typedef struct {
  /*Flags common to all chip types*/
  uint8_t _NFCC_I2C_READ_WRITE_IMPROVEMENT : 1;
  uint8_t _NFCC_MIFARE_TIANJIN : 1;
  uint8_t _NFCC_SPI_FW_DOWNLOAD_SYNC : 1;
  uint8_t _NFCEE_REMOVED_NTF_RECOVERY : 1;
  uint8_t _NFCC_FORCE_FW_DOWNLOAD : 1;
  uint8_t _NFCC_DWNLD_MODE : 1;
  uint8_t _NFCC_4K_FW_SUPPORT : 1;
} tNfc_nfccFeatureList;

typedef struct {
  uint8_t id;
  uint8_t len;
  uint8_t val;
} tNfc_capability;

typedef struct {
  tNfc_capability OBSERVE_MODE;
  tNfc_capability POLLING_FRAME_NOTIFICATION;
  tNfc_capability POWER_SAVING;
  tNfc_capability AUTOTRANSACT_PLF;
  tNfc_capability NO_OF_EXIT_FRAMES_PLF;
} tNfc_nfccCapability;

typedef struct {
  uint8_t nfcNxpEse : 1;
  tNFC_chipType chipType;
  std::string _FW_LIB_PATH;
  std::string _FW_BIN_PATH;
  uint16_t _PHDNLDNFC_USERDATA_EEPROM_OFFSET;
  uint16_t _PHDNLDNFC_USERDATA_EEPROM_LEN;
  uint8_t _FW_MOBILE_MAJOR_NUMBER;
  tNfc_nfccFeatureList nfccFL;
  tNfc_nfccCapability nfccCap;
} tNfc_featureList;

extern tNfc_featureList nfcFL;
#define GET_FW_ROM_VERSION_NCI_RESP(msg, msg_len) ((msg)[(msg_len) - 3])
#define GET_FW_MAJOR_VERSION_NCI_RESP(msg, msg_len) ((msg)[(msg_len) - 2])
#define GET_HW_VERSION_NCI_RESP(msg, msg_len) ((msg)[(msg_len) - 4])
#define IS_CHIP_TYPE_GE(cType) (nfcFL.chipType >= (cType))
#define IS_CHIP_TYPE_EQ(cType) (nfcFL.chipType == (cType))
#define IS_CHIP_TYPE_LE(cType) (nfcFL.chipType <= (cType))
#define IS_CHIP_TYPE_L(cType) (nfcFL.chipType < (cType))
#define IS_CHIP_TYPE_NE(cType) (nfcFL.chipType != (cType))
#define IS_4K_SUPPORT (nfcFL.nfccFL._NFCC_4K_FW_SUPPORT == true)

#define CONFIGURE_4K_SUPPORT(value)             \
  {                                             \
    nfcFL.nfccFL._NFCC_4K_FW_SUPPORT = (value); \
  }

#define CONFIGURE_FEATURELIST(chipType)               \
  {                                                   \
    nfcFL.chipType = chipType;                        \
    switch (chipType) {                               \
      case pn81T:                                     \
        nfcFL.chipType = pn557;                       \
        nfcFL.nfcNxpEse = true;                       \
        CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) \
        break;                                        \
      case pn80T:                                     \
        nfcFL.chipType = pn553;                       \
        nfcFL.nfcNxpEse = true;                       \
        CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) \
        break;                                        \
      case pn67T:                                     \
        nfcFL.chipType = pn551;                       \
        nfcFL.nfcNxpEse = true;                       \
        CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) \
        break;                                        \
      case pn66T:                                     \
        nfcFL.chipType = pn548C2;                     \
        nfcFL.nfcNxpEse = true;                       \
        CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) \
        break;                                        \
      case pn65T:                                     \
        nfcFL.chipType = pn547C2;                     \
        nfcFL.nfcNxpEse = true;                       \
        CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) \
        break;                                        \
      case sn100u:                                    \
        nfcFL.chipType = sn100u;                      \
        nfcFL.nfcNxpEse = true;                       \
        CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) \
        break;                                        \
      case sn220u:                                    \
        nfcFL.chipType = sn220u;                      \
        nfcFL.nfcNxpEse = true;                       \
        CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) \
        break;                                        \
      case sn300u:                                    \
        nfcFL.chipType = sn300u;                      \
        nfcFL.nfcNxpEse = true;                       \
        CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) \
        break;                                        \
      default:                                        \
        nfcFL.nfcNxpEse = false;                      \
        CONFIGURE_FEATURELIST_NFCC(chipType)          \
    }                                                 \
  }

#define CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType)   \
  {                                                     \
    switch (chipType) {                                 \
      case pn81T:                                       \
        CONFIGURE_FEATURELIST_NFCC(pn557)               \
        nfcFL.nfccFL._NFCC_SPI_FW_DOWNLOAD_SYNC = true; \
        break;                                          \
      case sn100u:                                      \
        CONFIGURE_FEATURELIST_NFCC(sn100u)              \
        nfcFL.nfccFL._NFCC_SPI_FW_DOWNLOAD_SYNC = true; \
        break;                                          \
      case sn220u:                                      \
        CONFIGURE_FEATURELIST_NFCC(sn220u)              \
        nfcFL.nfccFL._NFCC_SPI_FW_DOWNLOAD_SYNC = true; \
        break;                                          \
      case sn300u:                                      \
        CONFIGURE_FEATURELIST_NFCC(sn300u)              \
        nfcFL.nfccFL._NFCC_SPI_FW_DOWNLOAD_SYNC = true; \
        break;                                          \
      default:                                          \
        break;                                          \
    }                                                   \
  }

#define CONFIGURE_FEATURELIST_NFCC(chipType)                           \
  {                                                                    \
    nfcFL._PHDNLDNFC_USERDATA_EEPROM_OFFSET = 0x023CU;                 \
    nfcFL._PHDNLDNFC_USERDATA_EEPROM_LEN = 0x0C80U;                    \
    nfcFL._FW_MOBILE_MAJOR_NUMBER = FW_MOBILE_MAJOR_NUMBER_PN48AD;     \
    nfcFL.nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_VEN_RESET;         \
    nfcFL.nfccFL._NFCC_4K_FW_SUPPORT = false;                          \
    UPDATE_NFCC_CAPABILITY()                                           \
    switch (chipType) {                                                \
      case pn557:                                                      \
        nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT = true;          \
        STRCPY_FW("libpn557_fw")                                       \
        STRCPY_FW_BIN("pn557")                                         \
        break;                                                         \
      case sn100u:                                                     \
        nfcFL.nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_NCI_CMD;       \
        nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT = true;          \
        nfcFL.nfccFL._NFCC_MIFARE_TIANJIN = false;                     \
        nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD = true;                   \
        nfcFL._FW_MOBILE_MAJOR_NUMBER = FW_MOBILE_MAJOR_NUMBER_SN100U; \
        STRCPY_FW("libsn100u_fw")                                      \
        STRCPY_FW_BIN("sn100u")                                        \
        break;                                                         \
      case sn220u:                                                     \
        nfcFL.nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_NCI_CMD;       \
        nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT = true;          \
        nfcFL.nfccFL._NFCC_MIFARE_TIANJIN = false;                     \
        nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD = true;                   \
        nfcFL._FW_MOBILE_MAJOR_NUMBER = FW_MOBILE_MAJOR_NUMBER_SN220U; \
        STRCPY_FW("libsn220u_fw")                                      \
        STRCPY_FW_BIN("sn220u")                                        \
        break;                                                         \
      case pn560:                                                      \
        nfcFL.nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_NCI_CMD;       \
        nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT = true;          \
        nfcFL.nfccFL._NFCC_MIFARE_TIANJIN = false;                     \
        nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD = true;                   \
        nfcFL._FW_MOBILE_MAJOR_NUMBER = FW_MOBILE_MAJOR_NUMBER_SN220U; \
        STRCPY_FW("libpn560_fw")                                       \
        STRCPY_FW_BIN("pn560")                                         \
        break;                                                         \
      case sn300u:                                                     \
        nfcFL.nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_NCI_CMD;       \
        nfcFL.nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT = true;          \
        nfcFL.nfccFL._NFCC_MIFARE_TIANJIN = false;                     \
        nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD = true;                   \
        nfcFL._FW_MOBILE_MAJOR_NUMBER = FW_MOBILE_MAJOR_NUMBER_SN300U; \
        STRCPY_FW("libsn300u_fw")                                      \
        STRCPY_FW_BIN("sn300u")                                        \
        break;                                                         \
      default:                                                         \
        nfcFL.nfccFL._NFCC_FORCE_FW_DOWNLOAD = true;                   \
        break;                                                         \
    }                                                                  \
  }

#define STRCPY_FW_BIN(str)                       \
  {                                              \
    nfcFL._FW_BIN_PATH.clear();                  \
    nfcFL._FW_BIN_PATH.append(FW_BIN_ROOT_DIR);  \
    nfcFL._FW_BIN_PATH.append(str);              \
    nfcFL._FW_BIN_PATH.append(FW_BIN_EXTENSION); \
  }
#define STRCPY_FW(str1)                          \
  {                                              \
    nfcFL._FW_LIB_PATH.clear();                  \
    nfcFL._FW_LIB_PATH.append(FW_LIB_ROOT_DIR);  \
    nfcFL._FW_LIB_PATH.append(str1);             \
    nfcFL._FW_LIB_PATH.append(FW_LIB_EXTENSION); \
  }

#define CAP_OBSERVE_MODE_ID 0x00
#define CAP_POLL_FRAME_NTF_ID 0x01
#define CAP_POWER_SAVING_MODE_ID 0x02
#define CAP_AUTOTRANSACT_PLF_ID 0x03
#define CAP_NUMBER_OF_EXIT_FRAMES_PLF_ID 0x04
#define OBSERVE_MODE_WITHOUT_RF_DEACTIVATE 0x02

#define UPDATE_NFCC_CAPABILITY()                                               \
  {                                                                            \
    nfcFL.nfccCap.OBSERVE_MODE.id = CAP_OBSERVE_MODE_ID;                       \
    nfcFL.nfccCap.OBSERVE_MODE.len = 0x01;                                     \
    nfcFL.nfccCap.OBSERVE_MODE.val = 0x00;                                     \
    nfcFL.nfccCap.POLLING_FRAME_NOTIFICATION.id = CAP_POLL_FRAME_NTF_ID;       \
    nfcFL.nfccCap.POLLING_FRAME_NOTIFICATION.len = 0x01;                       \
    nfcFL.nfccCap.POLLING_FRAME_NOTIFICATION.val = 0x00;                       \
    nfcFL.nfccCap.POWER_SAVING.id = CAP_POWER_SAVING_MODE_ID;                  \
    nfcFL.nfccCap.POWER_SAVING.len = 0x01;                                     \
    nfcFL.nfccCap.POWER_SAVING.val = 0x00;                                     \
    nfcFL.nfccCap.AUTOTRANSACT_PLF.id = CAP_AUTOTRANSACT_PLF_ID;               \
    nfcFL.nfccCap.AUTOTRANSACT_PLF.len = 0x01;                                 \
    nfcFL.nfccCap.AUTOTRANSACT_PLF.val = 0x00;                                 \
    nfcFL.nfccCap.NO_OF_EXIT_FRAMES_PLF.id = CAP_NUMBER_OF_EXIT_FRAMES_PLF_ID; \
    nfcFL.nfccCap.NO_OF_EXIT_FRAMES_PLF.len = 0x01;                            \
    nfcFL.nfccCap.NO_OF_EXIT_FRAMES_PLF.val = 0x00;                            \
    uint8_t extended_field_mode = 0x00;                                        \
    if (IS_CHIP_TYPE_GE(sn100u) &&                                             \
        GetNxpNumValue(NAME_NXP_EXTENDED_FIELD_DETECT_MODE,                    \
                       &extended_field_mode, sizeof(extended_field_mode))) {   \
      if (extended_field_mode == 0x03) {                                       \
        nfcFL.nfccCap.OBSERVE_MODE.val = OBSERVE_MODE_WITHOUT_RF_DEACTIVATE;   \
      }                                                                        \
    }                                                                          \
    uint8_t max_exit_frames = 0x00;                                            \
    uint8_t no_of_exit_frames_supported = 0x00;                                \
    if (IS_CHIP_TYPE_GE(sn100u) &&                                             \
        GetNxpNumValue(NAME_NXP_MAX_EXIT_FRAMES_SUPPORTED, &max_exit_frames,   \
                       sizeof(max_exit_frames)) &&                             \
        GetNxpNumValue(NAME_NXP_NUMBER_OF_EXIT_FRAMES_SUPPORTED,               \
                       &no_of_exit_frames_supported,                           \
                       sizeof(no_of_exit_frames_supported))) {                 \
      if (no_of_exit_frames_supported > 0 &&                                   \
          no_of_exit_frames_supported <= max_exit_frames) {                    \
        nfcFL.nfccCap.AUTOTRANSACT_PLF.val = 0x01;                             \
        nfcFL.nfccCap.NO_OF_EXIT_FRAMES_PLF.val = no_of_exit_frames_supported; \
      }                                                                        \
    }                                                                          \
    unsigned long num = 0;                                                     \
    if ((GetNxpNumValue(NAME_NXP_DEFAULT_ULPDET_MODE, &num, sizeof(num)))) {   \
      if ((uint8_t)num > 0) {                                                  \
        nfcFL.nfccCap.POWER_SAVING.val = 0x01;                                 \
      }                                                                        \
    }                                                                          \
  }
#endif
