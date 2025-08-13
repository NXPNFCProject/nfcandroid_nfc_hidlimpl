/******************************************************************************
 *
 *  Copyright (C) 2011-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
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

/******************************************************************************
 *
 *  The original Work has been changed by NXP.
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
 *  Copyright 2013-2021, 2023,2025 NXP
 *
 ******************************************************************************/

#include <android-base/properties.h>
#include <errno.h>
#include <log/log.h>
#include <phDnldNfc_Internal.h>
#include <phNxpConfig.h>
#include <phNxpLog.h>
#include <stdio.h>
#include <sys/stat.h>

#include <list>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <set>

#include "sparse_crc32.h"

using std::list;

#if GENERIC_TARGET
const char alternative_config_path[] = "/data/vendor/nfc/";
#else
const char alternative_config_path[] = "";
#endif

#if 1
const char* transport_config_paths[] = {"/odm/etc/", "/vendor/etc/", "/etc/"};
#else
const char* transport_config_paths[] = {"res/"};
#endif
const int transport_config_path_size =
    (sizeof(transport_config_paths) / sizeof(transport_config_paths[0]));

#define config_name "libnfc-nxp.conf"
#define extra_config_base "libnfc-"
#define extra_config_ext ".conf"
#define IsStringValue 0x80000000

typedef enum {
  CONF_FILE_NXP = 0x00,
  CONF_FILE_NXP_RF,
  CONF_FILE_NXP_TRANSIT
} tNXP_CONF_FILE;

const char rf_config_timestamp_path[] =
    "/data/vendor/nfc/libnfc-nxpRFConfigState.bin";
const char tr_config_timestamp_path[] =
    "/data/vendor/nfc/libnfc-nxpTransitConfigState.bin";
const char config_timestamp_path[] =
    "/data/vendor/nfc/libnfc-nxpConfigState.bin";

char nxp_rf_config_path[256] = "/system/vendor/libnfc-nxp_RF.conf";
#if (defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64))
char Fw_Lib_Path[256] = "/vendor/lib64/libsn100u_fw.so";
#else
char Fw_Lib_Path[256] = "/vendor/lib/libsn100u_fw.so";
#endif

const char transit_config_path[] = "/data/vendor/nfc/libnfc-nxpTransit.conf";

void readOptionalConfig(const char* optional);

size_t readConfigFile(const char* fileName, uint8_t** p_data) {
  if (!fileName || !p_data) {
    ALOGE("%s Invalid parameters", __func__);
    return 0;
  }

  FILE* fd = fopen(fileName, "rb");
  if (fd == nullptr) return 0;

  // RAII wrapper for automatic file closure
  struct FileGuard {
    FILE* f;
    FileGuard(FILE* file) : f(file) {}
    ~FileGuard() { if (f) fclose(f); }
  } guard(fd);

  if (fseek(fd, 0L, SEEK_END) != 0) {
    ALOGE("%s Failed to seek to end of file", __func__);
    return 0;
  }

  const long file_size_long = ftell(fd);
  if (file_size_long < 0) {
    ALOGE("%s Invalid file size file_size = %ld", __func__, file_size_long);
    return 0;
  }

  const size_t file_size = static_cast<size_t>(file_size_long);
  rewind(fd);

  // Use smart pointer for automatic cleanup
  std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(file_size + 1);
  if (!buffer) {
    ALOGE("%s Failed to allocate buffer of size %zu", __func__, file_size + 1);
    return 0;
  }

  size_t read = fread(buffer.get(), file_size, 1, fd);
  if (read == 1) {
    buffer[file_size] = '\n';
    *p_data = buffer.release(); // Transfer ownership to caller
    return file_size + 1;
  }

  ALOGE("%s Failed to read file, read=%zu", __func__, read);
  return 0;
}

using std::vector;
using std::string;

class CNfcParam : public string {
 public:
  CNfcParam();
  CNfcParam(const char* name, const string& value);
  CNfcParam(const char* name, unsigned long value);
  virtual ~CNfcParam();

  unsigned long numValue() const { return m_numValue; }
  const char* str_value() const { return m_str_value.c_str(); }
  size_t str_len() const { return m_str_value.length(); }

 private:
  string m_str_value;
  unsigned long m_numValue;
};

class CNfcConfig : public vector<const CNfcParam*> {
 public:
  static CNfcConfig& GetInstance();
  static void DestroyCNfcConfig();
  friend void readOptionalConfig(const char* optional);

  bool isModified(tNXP_CONF_FILE aType);
  void resetModified(tNXP_CONF_FILE aType);
  bool getValue(const char* name, char* pValue, size_t len) const;
  bool getValue(const char* name, unsigned long& rValue) const;
  bool getValue(const char* name, unsigned short& rValue) const;
  bool getValue(const char* name, char* pValue, long len, long* readlen) const;
  const CNfcParam* find(const char* p_name) const;
  void readNxpTransitConfig(const char* fileName) const;
  void readNxpRFConfig(const char* fileName) const;
  void clean();

 private:
  CNfcConfig();
  ~CNfcConfig();
  CNfcConfig(const CNfcConfig&) = delete;
  CNfcConfig& operator=(const CNfcConfig&) = delete;

  bool readConfig(const char* name, bool bResetContent);
  void moveFromList();
  void moveToList();
  void add(const CNfcParam* pParam);
  void dump();
  bool isAllowed(const char* name);

  list<const CNfcParam*> m_list;
  mutable std::recursive_mutex m_config_mutex;
  std::atomic<bool> m_initialized{false};

  static CNfcConfig* theInstance;
  // Use atomic flag for initialization check to avoid repeated work
  static std::mutex initialization_mutex;
  static std::atomic<bool> is_initialized;

  bool mValidFile;
  uint32_t config_crc32_;
  uint32_t config_rf_crc32_;
  uint32_t config_tr_crc32_;
  string mCurrentFile;
  unsigned long state;

  inline bool Is(unsigned long f) { return (state & f) == f; }
  inline void Set(unsigned long f) { state |= f; }
  inline void Reset(unsigned long f) { state &= ~f; }
};

// Static member definitions
CNfcConfig* CNfcConfig::theInstance = nullptr;
std::mutex CNfcConfig::initialization_mutex;
std::atomic<bool> CNfcConfig::is_initialized{false};

/*******************************************************************************
**
** Function:    isPrintable()
**
** Description: determine if 'c' is printable
**
** Returns:     1, if printable, otherwise 0
**
*******************************************************************************/
inline bool isPrintable(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '/' || c == '_' || c == '-' || c == '.';
}

/*******************************************************************************
**
** Function:    isDigit()
**
** Description: determine if 'c' is numeral digit
**
** Returns:     true, if numerical digit
**
*******************************************************************************/
inline bool isDigit(char c, int base) {
  if ('0' <= c && c <= '9') return true;
  if (base == 16) {
    if (('A' <= c && c <= 'F') || ('a' <= c && c <= 'f')) return true;
  }
  return false;
}

/*******************************************************************************
**
** Function:    getDigitValue()
**
** Description: return numerical value of a decimal or hex char
**
** Returns:     numerical value if decimal or hex char, otherwise 0
**
*******************************************************************************/
inline int getDigitValue(char c, int base) {
  if ('0' <= c && c <= '9') return c - '0';
  if (base == 16) {
    if ('A' <= c && c <= 'F')
      return c - 'A' + 10;
    else if ('a' <= c && c <= 'f')
      return c - 'a' + 10;
  }
  return 0;
}

/*******************************************************************************
**
** Function:    findConfigFilePathFromTransportConfigPaths()
**
** Description: find a config file path with a given config name from transport
**              config paths
**
** Returns:     none
**
*******************************************************************************/
bool findConfigFilePathFromTransportConfigPaths(const string& configName,
                                                string& filePath) {
  if (configName.empty()) {
    ALOGE("%s Config name is empty", __func__);
    filePath = "";
    return false;
  }

  for (int i = 0; i < transport_config_path_size - 1; i++) {
    filePath.assign(transport_config_paths[i]);
    filePath += configName;
    struct stat file_stat;
    if (stat(filePath.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
      return true;
    }
  }

  filePath = "";
  return false;
}

/*******************************************************************************
**
** Function:    CNfcConfig::readConfig()
**
** Description: read Config settings and parse them into a linked list
**              move the element from linked list to a array at the end
**
** Returns:     1, if there are any config data, 0 otherwise
**
*******************************************************************************/
bool CNfcConfig::readConfig(const char* name, bool bResetContent) {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (!name) {
    ALOGE("%s Invalid file name parameter", __func__);
    return false;
  }

  enum {
    BEGIN_LINE = 1,
    TOKEN,
    STR_VALUE,
    NUM_VALUE,
    BEGIN_HEX,
    BEGIN_QUOTE,
    END_LINE
  };

  uint8_t* p_config = nullptr;
  size_t config_size = readConfigFile(name, &p_config);
  if (p_config == nullptr) {
    ALOGE("%s Cannot open config file %s", __func__, name);
    if (bResetContent)
      mValidFile = false;
    return false;
  }

  std::unique_ptr<uint8_t[]> config_guard(p_config);
  string token;
  string strValue;
  unsigned long numValue = 0;
  CNfcParam* pParam = NULL;
  int i = 0;
  int base = 0;
  char c;
  int bflag = 0;
  state = BEGIN_LINE;

  ALOGD("readConfig; filename is %s", name);
  if (strcmp(name, nxp_rf_config_path) == 0) {
    config_rf_crc32_ = sparse_crc32(0, (const void*)p_config, (int)config_size);
  } else if (strcmp(name, transit_config_path) == 0) {
    config_tr_crc32_ = sparse_crc32(0, (const void*)p_config, (int)config_size);
  } else {
    config_crc32_ = sparse_crc32(0, (const void*)p_config, (int)config_size);
  }

  mValidFile = true;
  mCurrentFile = name;

  if (size() > 0) {
    if (bResetContent)
      clean();
    else
      moveToList();
  }

  for (size_t offset = 0; offset != config_size; ++offset) {
    c = p_config[offset];
    switch (state & 0xff) {
      case BEGIN_LINE:
        if (c == '#')
          state = END_LINE;
        else if (isPrintable(c)) {
          i = 0;
          token.clear();
          strValue.clear();
          state = TOKEN;
          token.push_back(c);
        }
        break;
      case TOKEN:
        if (c == '=') {
          token.push_back('\0');
          state = BEGIN_QUOTE;
        } else if (isPrintable(c)) {
          token.push_back(c);
        } else {
          state = END_LINE;
        }
        break;
      case BEGIN_QUOTE:
        if (c == '"') {
          state = STR_VALUE;
          base = 0;
        } else if (c == '0') {
          state = BEGIN_HEX;
        } else if (isDigit(c, 10)) {
          state = NUM_VALUE;
          base = 10;
          numValue = getDigitValue(c, base);
          i = 0;
        } else if (c == '{') {
          state = NUM_VALUE;
          bflag = 1;
          base = 16;
          i = 0;
          Set(IsStringValue);
        } else {
          state = END_LINE;
        }
        break;
      case BEGIN_HEX:
        if (c == 'x' || c == 'X') {
          state = NUM_VALUE;
          base = 16;
          numValue = 0;
          i = 0;
          break;
        } else if (isDigit(c, 10)) {
          state = NUM_VALUE;
          base = 10;
          numValue = getDigitValue(c, base);
          break;
        } else if (c != '\n' && c != '\r') {
          state = END_LINE;
          break;
        }
        [[fallthrough]]; // fall through to numValue to handle numValue
      case NUM_VALUE:
        if (isDigit(c, base)) {
          numValue *= base;
          numValue += getDigitValue(c, base);
          ++i;
        } else if (bflag == 1 &&
                   (c == ' ' || c == '\r' || c == '\n' || c == '\t')) {
          break;
        } else if (base == 16 &&
                   (c == ',' || c == ':' || c == '-' || c == ' ' || c == '}')) {
          if (c == '}') {
            bflag = 0;
          }
          if (i > 0) {
            int n = (i + 1) / 2;
            while (n-- > 0) {
              numValue = numValue >> (n * 8);
              unsigned char chVal = (numValue) & 0xFF;
              strValue.push_back(chVal);
            }
          }
          Set(IsStringValue);
          numValue = 0;
          i = 0;
        } else {
          if (c == '\n' || c == '\r') {
            if (bflag == 0) {
              state = BEGIN_LINE;
            }
          } else {
            if (bflag == 0) {
              state = END_LINE;
            }
          }
          if (Is(IsStringValue) && base == 16 && i > 0) {
            int n = (i + 1) / 2;
            while (n-- > 0) {
              strValue.push_back(((numValue >> (n * 8)) & 0xFF));
            }
          }
          if (strValue.length() > 0) {
            pParam = new CNfcParam(token.c_str(), strValue);
          } else {
            pParam = new CNfcParam(token.c_str(), numValue);
          }
          add(pParam);
          pParam = NULL;
          strValue.clear();
          numValue = 0;
          Reset(IsStringValue);
        }
        break;
      case STR_VALUE:
        if (c == '"') {
          strValue.push_back('\0');
          state = END_LINE;
          pParam = new CNfcParam(token.c_str(), strValue);
          add(pParam);
          pParam = NULL;
        } else if (isPrintable(c)) {
          strValue.push_back(c);
        }
        break;
      case END_LINE:
        if (c == '\n' || c == '\r') {
          state = BEGIN_LINE;
        }
        break;
      default:
        break;
    }
  }

  moveFromList();
  return size() > 0;
}

/*******************************************************************************
**
** Function:    CNfcConfig::CNfcConfig()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig::CNfcConfig()
    : m_initialized(false),
      mValidFile(true),
      config_crc32_(0),
      config_rf_crc32_(0),
      config_tr_crc32_(0),
      state(0) {}

/*******************************************************************************
**
** Function:    CNfcConfig::~CNfcConfig()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig::~CNfcConfig() {
  clean();
}

/*******************************************************************************
**
** Function:    CNfcConfig::GetInstance()
**
** Description: get class singleton object
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig& CNfcConfig::GetInstance() {
  // Double-checked locking pattern
  if (!is_initialized.load()) {
    std::lock_guard<std::mutex> lock(initialization_mutex);

    // Check again after acquiring lock
    if (!is_initialized.load()) {
      if (theInstance == nullptr) {
        theInstance = new CNfcConfig();
      }

      if (theInstance->size() == 0 && theInstance->mValidFile) {
        string strPath;
        if (alternative_config_path[0] != '\0') {
          strPath.assign(alternative_config_path);
          strPath += config_name;
          theInstance->readConfig(strPath.c_str(), true);
          if (!theInstance->empty()) {
            is_initialized.store(true);
            return *theInstance;
          }
        }

        if (findConfigFilePathFromTransportConfigPaths(
                android::base::GetProperty("persist.vendor.nfc.config_file_name", ""),
                strPath)) {
          NXPLOG_NCIHAL_D("%s load %s", __func__, strPath.c_str());
        } else if (findConfigFilePathFromTransportConfigPaths(
                     extra_config_base +
                         android::base::GetProperty("ro.boot.product.hardware.sku", "") +
                         extra_config_ext,
                     strPath)) {
          NXPLOG_NCIHAL_D("%s load %s", __func__, strPath.c_str());
        } else {
          findConfigFilePathFromTransportConfigPaths(config_name, strPath);
        }

        theInstance->readConfig(strPath.c_str(), true);
        theInstance->readNxpRFConfig(nxp_rf_config_path);
        theInstance->readNxpTransitConfig(transit_config_path);
      }
      is_initialized.store(true);  // Mark as initialized
    }
  }
  return *theInstance;
}

void CNfcConfig::DestroyCNfcConfig() {
  std::lock_guard<std::mutex> lock(initialization_mutex);
  if (theInstance != nullptr) {
    delete theInstance;
    theInstance = nullptr;
  }
  is_initialized.store(false);
}
/*******************************************************************************
**
** Function:    CNfcConfig::getValue()
**
** Description: get a string value of a setting
**
** Returns:     true if setting exists
**              false if setting does not exist
**
*******************************************************************************/
bool CNfcConfig::getValue(const char* name, char* pValue, size_t len) const {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (!name || !pValue || len == 0) {
    ALOGE("%s Invalid parameters: name=%p, pValue=%p, len=%zu", __func__, name, pValue, len);
    return false;
  }

  const CNfcParam* pParam = find(name);
  if (pParam == NULL) {
    ALOGE("%s Parameter %s not found", __func__, name);
    return false;
  }

  if (pParam->str_len() > 0) {
    size_t copy_len = std::min(pParam->str_len(), len - 1);
    memset(pValue, 0, len);
    memcpy(pValue, pParam->str_value(), copy_len);
    pValue[copy_len] = '\0';
    return true;
  }
  return false;
}


bool CNfcConfig::getValue(const char* name, char* pValue, long len,
                          long* readlen) const {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (!name || !pValue || !readlen || len <= 0) {
    ALOGE("%s Invalid parameters: name=%p, pValue=%p, readlen=%p, len=%ld",
          __func__, name, pValue, readlen, len);
    if (readlen) *readlen = -1;
    return false;
  }

  const CNfcParam* pParam = find(name);
  if (pParam == NULL) {
    *readlen = -1;
    return false;
  }

  if (pParam->str_len() > 0) {
    if (pParam->str_len() <= (unsigned long)len) {
      memset(pValue, 0, len);
      memcpy(pValue, pParam->str_value(), pParam->str_len());
      *readlen = pParam->str_len();
    } else {
      *readlen = -1;
    }
    return true;
  }
  *readlen = -1;
  return false;
}

/*******************************************************************************
**
** Function:    CNfcConfig::getValue()
**
** Description: get a long numerical value of a setting
**
** Returns:     true if setting exists
**              false if setting does not exist
**
*******************************************************************************/
bool CNfcConfig::getValue(const char* name, unsigned long& rValue) const {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (!name) {
    ALOGE("%s Invalid parameter: name is null", __func__);
    return false;
  }
  const CNfcParam* pParam = find(name);
  if (pParam == NULL) return false;

  if (pParam->str_len() == 0) {
    rValue = static_cast<unsigned long>(pParam->numValue());
    return true;
  }
  return false;
}

/*******************************************************************************
**
** Function:    CNfcConfig::getValue()
**
** Description: get a short numerical value of a setting
**
** Returns:     true if setting exists
**              false if setting does not exist
**
*******************************************************************************/
bool CNfcConfig::getValue(const char* name, unsigned short& rValue) const {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (!name) {
    ALOGE("%s Invalid parameter: name is null", __func__);
    return false;
  }

  const CNfcParam* pParam = find(name);
  if (pParam == NULL) return false;

  if (pParam->str_len() == 0) {
    unsigned long numVal = pParam->numValue();
    if (numVal <= USHRT_MAX) {
      rValue = static_cast<unsigned short>(numVal);
      return true;
    } else {
      ALOGE("%s Parameter %s value %lu exceeds unsigned short range",
            __func__, name, numVal);
      return false;
    }
  }

  return false;
}

/*******************************************************************************
**
** Function:    CNfcConfig::find()
**
** Description: search if a setting exist in the setting array
**
** Returns:     pointer to the setting object
**
*******************************************************************************/
const CNfcParam* CNfcConfig::find(const char* p_name) const {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (!p_name) {
    ALOGE("%s Invalid parameter: p_name is null", __func__);
    return NULL;
  }

  if (size() == 0) {
    ALOGE("%s No parameters loaded", __func__);
    return NULL;
  }

  for (const_iterator it = begin(), itEnd = end(); it != itEnd; ++it) {
    if (!*it) continue;

    const char* param_name = (*it)->c_str();
    if (!param_name) continue;

    int cmp = strcmp(param_name, p_name);
    if (cmp < 0) {
      continue;
    } else if (cmp == 0) {
      if ((*it)->str_len() > 0) {
        NXPLOG_NCIHAL_D("%s found %s=%s", __func__, p_name,
                       (*it)->str_value());
      } else {
        NXPLOG_NCIHAL_D("%s found %s=(0x%lx)", __func__, p_name,
                       (*it)->numValue());
      }
      return *it;
    } else {
      break;
    }
  }
  return NULL;
}

/*******************************************************************************
**
** Function:    CNfcConfig::readNxpTransitConfig()
**
** Description: read Config settings from transit conf file
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::readNxpTransitConfig(const char* fileName) const {
  ALOGD("readNxpTransitConfig-Enter..Reading %s", fileName);
  const_cast<CNfcConfig*>(this)->readConfig(fileName, false);
}

/*******************************************************************************
**
** Function:    CNfcConfig::readNxpRFConfig()
**
** Description: read Config settings from RF conf file
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::readNxpRFConfig(const char* fileName) const {
  ALOGD("readNxpRFConfig-Enter..Reading %s", fileName);
  const_cast<CNfcConfig*>(this)->readConfig(fileName, false);
}

/*******************************************************************************
**
** Function:    CNfcConfig::clean()
**
** Description: reset the setting array
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::clean() {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  ALOGD("%s Cleaning up all configuration data", __func__);
  // Create a set to track unique objects and avoid double-deletion
  std::set<const CNfcParam*> unique_objects;
  if (size() > 0) {
    for (iterator it = begin(), itEnd = end(); it != itEnd; ++it) {
      if (*it) {
        unique_objects.insert(*it);
      }
    }
    clear();  // Clear vector without deleting
  }
  // Collect all unique objects from list
  if (m_list.size() > 0) {
    for (list<const CNfcParam*>::iterator it = m_list.begin();
         it != m_list.end(); ++it) {
      if (*it) {
        unique_objects.insert(*it);
      }
    }
    m_list.clear();  // Clear list without deleting
  }
  // Now delete each unique object exactly once
  for (std::set<const CNfcParam*>::iterator it = unique_objects.begin();
       it != unique_objects.end(); ++it) {
    delete *it;
  }
  unique_objects.clear();
}

/*******************************************************************************
**
** Function:    CNfcConfig::add()
**
** Description: add a setting object to the list
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::add(const CNfcParam* pParam) {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (!pParam) {
    ALOGE("%s Invalid parameter: pParam is null", __func__);
    return;
  }

  const char* param_name = pParam->c_str();
  if (!param_name) {
    ALOGE("%s Parameter has null name", __func__);
    delete pParam;
    return;
  }
  if (m_list.size() == 0) {
    m_list.push_back(pParam);
    return;
  }

  if ((mCurrentFile.find("libnfc-nci-update.conf") != std::string::npos) &&
      !isAllowed(param_name)) {
    ALOGE("%s Token %s restricted. Returning", __func__, param_name);
    delete pParam;
    return;
  }

  for (auto it = m_list.begin(), itEnd = m_list.end(); it != itEnd; ++it) {
    if (!*it) continue;

    const char* existing_name = (*it)->c_str();
    if (!existing_name) continue;

    int cmp = strcmp(existing_name, param_name);
    if (cmp < 0) {
      continue;
    } else if (cmp == 0) {
      const CNfcParam* oldParam = *it;
      m_list.insert(m_list.erase(it), pParam);
      delete oldParam;
    } else {
      m_list.insert(it, pParam);
    }
    return;
  }

  m_list.push_back(pParam);
}
/*******************************************************************************
**
** Function:    CNfcConfig::dump()
**
** Description: prints all elements in the list
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::dump() {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  ALOGD("%s Enter", __func__);

  for (list<const CNfcParam*>::iterator it = m_list.begin(),
                                        itEnd = m_list.end();
       it != itEnd; ++it) {
    if ((*it)->str_len() > 0)
      ALOGD("%s %s \t= %s", __func__, (*it)->c_str(), (*it)->str_value());
    else
      ALOGD("%s %s \t= (0x%0lX)\n", __func__, (*it)->c_str(),
            (*it)->numValue());
  }
}
/*******************************************************************************
**
** Function:    CNfcConfig::isAllowed()
**
** Description: checks if token update is allowed
**
** Returns:     true if allowed else false
**
*******************************************************************************/
bool CNfcConfig::isAllowed(const char* name) {
  string token(name);
  bool stat = false;
  if ((token.find("HOST_LISTEN_TECH_MASK") != std::string::npos) ||
      (token.find("UICC_LISTEN_TECH_MASK") != std::string::npos) ||
      (token.find("NXP_ESE_LISTEN_TECH_MASK") != std::string::npos) ||
      (token.find("POLLING_TECH_MASK") != std::string::npos) ||
      (token.find("NXP_RF_CONF_BLK") != std::string::npos) ||
      (token.find("NXP_CN_TRANSIT_BLK_NUM_CHECK_ENABLE") !=
       std::string::npos) ||
      (token.find("NXP_FWD_FUNCTIONALITY_ENABLE") != std::string::npos) ||
      (token.find("NXP_MIFARE_NACK_TO_RATS_ENABLE") != std::string::npos))

  {
    stat = true;
  }
  return stat;
}
/*******************************************************************************
**
** Function:    CNfcConfig::moveFromList()
**
** Description: move the setting object from list to array
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::moveFromList() {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (m_list.size() == 0) return;

  reserve(size() + m_list.size());
  for (auto it = m_list.begin(), itEnd = m_list.end(); it != itEnd; ++it) {
    if (*it) push_back(*it);
  }
  m_list.clear();  // Just clear pointers, don't delete objects
}

/*******************************************************************************
**
** Function:    CNfcConfig::moveToList()
**
** Description: move the setting object from array to list
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::moveToList() {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  if (size() == 0) return;
  if (m_list.size() != 0) {
    for (list<const CNfcParam*>::iterator it = m_list.begin();
         it != m_list.end(); ++it) {
      delete *it;  // DELETE objects before clearing
    }
    m_list.clear();
  }

  for (iterator it = begin(), itEnd = end(); it != itEnd; ++it) {
    if (*it) {
      m_list.push_back(*it);
    }
  }
  clear();  // Just clear pointers, don't delete objects
}

bool CNfcConfig::isModified(tNXP_CONF_FILE aType) {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  const char* timestamp_path = nullptr;
  uint32_t current_crc32 = 0;
  const char* file_type_name = nullptr;
  // Determine file paths and CRC values based on type
  switch (aType) {
    case CONF_FILE_NXP:
      timestamp_path = config_timestamp_path;
      current_crc32 = config_crc32_;
      file_type_name = "NXP config";
      break;
    case CONF_FILE_NXP_RF:
      timestamp_path = rf_config_timestamp_path;
      current_crc32 = config_rf_crc32_;
      file_type_name = "NXP RF config";
      break;
    case CONF_FILE_NXP_TRANSIT:
      timestamp_path = tr_config_timestamp_path;
      current_crc32 = config_tr_crc32_;
      file_type_name = "NXP Transit config";
      break;
    default:
      ALOGE("%s Invalid conf file type: %d", __func__, aType);
      return false;
  }

  if (!timestamp_path) {
    ALOGE("%s Timestamp path is null for file type %d", __func__, aType);
    return true;
  }

  FILE* fd = fopen(timestamp_path, "r");
  if (fd == nullptr) {
    ALOGE("%s Unable to open timestamp file %s, assume modified (errno=%d: %s)",
          __func__, timestamp_path, errno, strerror(errno));
    return true;
  }

  struct FileGuard {
    FILE* f;
    FileGuard(FILE* file) : f(file) {}
    ~FileGuard() { if (f) fclose(f); }
  } guard(fd);

  uint32_t stored_crc32 = 0;
  size_t read_count = fread(&stored_crc32, sizeof(uint32_t), 1, fd);
  if (read_count != 1) {
    ALOGE("%s File read failed for %s: read %zu items, errno=%d: %s",
          __func__, timestamp_path, read_count, errno, strerror(errno));
    return true;  // Assume modified if we can't read
  }

  return (stored_crc32 != current_crc32);
}

void CNfcConfig::resetModified(tNXP_CONF_FILE aType) {
  std::lock_guard<std::recursive_mutex> lock(m_config_mutex);

  const char* timestamp_path = nullptr;
  uint32_t current_crc32 = 0;
  const char* file_type_name = nullptr;
  // Determine file paths and CRC values based on type
  switch (aType) {
    case CONF_FILE_NXP:
      timestamp_path = config_timestamp_path;
      current_crc32 = config_crc32_;
      file_type_name = "NXP config";
      break;
    case CONF_FILE_NXP_RF:
      timestamp_path = rf_config_timestamp_path;
      current_crc32 = config_rf_crc32_;
      file_type_name = "NXP RF config";
      break;
    case CONF_FILE_NXP_TRANSIT:
      timestamp_path = tr_config_timestamp_path;
      current_crc32 = config_tr_crc32_;
      file_type_name = "NXP Transit config";
      break;
    default:
      ALOGE("%s Invalid conf file type: %d", __func__, aType);
      return;
  }

  ALOGD("resetModified enter; conf file type %d (%s)", aType, file_type_name);
  if (!timestamp_path) {
    ALOGE("%s Timestamp path is null for file type %d", __func__, aType);
    return;
  }

  FILE* fd = fopen(timestamp_path, "w");
  if (fd == nullptr) {
    ALOGE("%s Unable to open timestamp file %s for writing (errno=%d: %s)",
          __func__, timestamp_path, errno, strerror(errno));
    return;
  }

  struct FileGuard {
    FILE* f;
    FileGuard(FILE* file) : f(file) {}
    ~FileGuard() { if (f) fclose(f); }
  } guard(fd);

  size_t write_count = fwrite(&current_crc32, sizeof(uint32_t), 1, fd);
  if (write_count != 1) {
    ALOGE("%s Failed to write CRC32 to %s: wrote %zu items (errno=%d: %s)",
          __func__, timestamp_path, write_count, errno, strerror(errno));
    return;
  }

  if (fflush(fd) != 0) {
    ALOGE("%s Failed to flush %s (errno=%d: %s)",
          __func__, timestamp_path, errno, strerror(errno));
    return;
  }
  if (fsync(fileno(fd)) != 0) {
    ALOGE("%s Failed to sync %s to disk (errno=%d: %s)",
          __func__, timestamp_path, errno, strerror(errno));
  }
}

/*******************************************************************************
**
** Function:    CNfcParam::CNfcParam()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcParam::CNfcParam() : string(), m_str_value(), m_numValue(0) {}

/*******************************************************************************
**
** Function:    CNfcParam::~CNfcParam()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
CNfcParam::~CNfcParam() {}

/*******************************************************************************
**
** Function:    CNfcParam::CNfcParam()
**
** Description: class copy constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcParam::CNfcParam(const char* name, const string& value)
    : string(name ? name : ""), m_str_value(value), m_numValue(0) {}


/*******************************************************************************
**
** Function:    CNfcParam::CNfcParam()
**
** Description: class copy constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcParam::CNfcParam(const char* name, unsigned long value)
    : string(name ? name : ""), m_str_value(), m_numValue(value) {}

/*******************************************************************************
**
** Function:    readOptionalConfig()
**
** Description: read Config settings from an optional conf file
**
** Returns:     none
**
*******************************************************************************/
void readOptionalConfig(const char* extra) {
  string strPath;
  string configName(extra_config_base);
  configName += extra;
  configName += extra_config_ext;

  if (alternative_config_path[0] != '\0') {
    strPath.assign(alternative_config_path);
    strPath += configName;
  } else {
    findConfigFilePathFromTransportConfigPaths(configName, strPath);
  }

  CNfcConfig::GetInstance().readConfig(strPath.c_str(), false);
}

/*******************************************************************************
**
** Function:    GetStrValue
**
** Description: API function for getting a string value of a setting
**
** Returns:     True if found, otherwise False.
**
*******************************************************************************/
extern "C" int GetNxpStrValue(const char* name, char* pValue,
                              unsigned long len) {
  CNfcConfig& rConfig = CNfcConfig::GetInstance();
  bool result = rConfig.getValue(name, pValue, static_cast<size_t>(len));
  return result ? 1 : 0;
}


/*******************************************************************************
**
** Function:    GetByteArrayValue()
**
** Description: Read byte array value from the config file.
**
** Parameters:
**              name - name of the config param to read.
**              pValue  - pointer to input buffer.
**              bufflen - input buffer length.
**              len - out parameter to return the number of bytes read from
**                    config file, return -1 in case bufflen is not enough.
**
** Returns:     TRUE[1] if config param name is found in the config file, else
**              FALSE[0]
**
*******************************************************************************/
extern "C" int GetNxpByteArrayValue(const char* name, char* pValue,
                                    long bufflen, long* len) {

  CNfcConfig& rConfig = CNfcConfig::GetInstance();
  bool result = rConfig.getValue(name, pValue, bufflen, len);
  return result ? 1 : 0;
}


/*******************************************************************************
**
** Function:    GetNumValue
**
** Description: API function for getting a numerical value of a setting
**
** Returns:     true, if successful
**
*******************************************************************************/
extern "C" int GetNxpNumValue(const char* name, void* pValue, unsigned long len) {
  if (!name || !pValue) {
    ALOGE("%s Invalid parameters: name=%p, pValue=%p", __func__, name, pValue);
    return 0;
  }

  const CNfcParam* pParam = CNfcConfig::GetInstance().find(name);
  if (pParam == NULL) return 0;

  unsigned long v = pParam->numValue();
  if (v == 0 && pParam->str_len() > 0 && pParam->str_len() < 4) {
    const unsigned char* p = (const unsigned char*)pParam->str_value();
    if (!p) {
      ALOGE("%s Parameter %s has null string value", __func__, name);
      return 0;
    }
    v = 0;
    for (size_t i = 0; i < pParam->str_len(); ++i) {
      v = (v << 8) | (*p++);
    }
  }

  switch (len) {
    case sizeof(unsigned long):
      *(static_cast<unsigned long*>(pValue)) = v;
      break;
    case sizeof(unsigned short):
      *(static_cast<unsigned short*>(pValue)) = static_cast<unsigned short>(v);
      break;
    case sizeof(unsigned char):
      *(static_cast<unsigned char*>(pValue)) = static_cast<unsigned char>(v);
      break;
    default:
      ALOGE("%s Unsupported length %lu", __func__, len);
      return 0;
  }

  return 1;
}

/*******************************************************************************
**
** Function:    setNxpRfConfigPath
**
** Description: sets the path of the NXP RF config file
**
** Returns:     none
**
*******************************************************************************/
extern "C" void setNxpRfConfigPath(const char* name) {
  if (!name) {
    ALOGE("%s Invalid parameter: name is null", __func__);
    return;
  }

  size_t name_len = strlen(name);
  if (name_len == 0) {
    ALOGE("%s Empty path provided", __func__);
    return;
  }

  if (name_len >= sizeof(nxp_rf_config_path)) {
    ALOGE("%s Path too long: %zu >= %zu", __func__, name_len,
          sizeof(nxp_rf_config_path));
    return;
  }
  memset(nxp_rf_config_path, 0, sizeof(nxp_rf_config_path));
  strlcpy(nxp_rf_config_path, name, sizeof(nxp_rf_config_path));
}

/*******************************************************************************
**
** Function:    setNxpFwConfigPath
**
** Description: sets the path of the NXP FW library
**
** Returns:     none
**
*******************************************************************************/
extern "C" void setNxpFwConfigPath() {
  unsigned long fwType = FW_FORMAT_SO;
  if (GetNxpNumValue(NAME_NXP_FW_TYPE, &fwType, sizeof(fwType))) {
    ALOGD("firmware type from conf file: %lu", fwType);
  }

  memset(Fw_Lib_Path, 0, sizeof(Fw_Lib_Path));
  std::string fw_file_path;
  if (fwType == FW_FORMAT_BIN) {
    fw_file_path = nfcFL._FW_BIN_PATH;
  } else {
    fw_file_path = nfcFL._FW_LIB_PATH;
  }
  if (fw_file_path.length() >= sizeof(Fw_Lib_Path)) {
    ALOGE("%s fw_file path too long: %zu >= %zu", __func__,
          fw_file_path.length(), sizeof(Fw_Lib_Path));
    return;
  }
  strlcpy(Fw_Lib_Path, fw_file_path.c_str(), sizeof(Fw_Lib_Path));
  ALOGD("Fw_Lib_Path=%s", Fw_Lib_Path);
}

/*******************************************************************************
**
** Function:    resetConfig
**
** Description: reset settings array
**
** Returns:     none
**
*******************************************************************************/
extern "C" void resetNxpConfig() {
  ALOGD("%s Resetting NXP configuration", __func__);
  CNfcConfig::DestroyCNfcConfig();
}

/*******************************************************************************
**
** Function:    isNxpConfigModified()
**
** Description: check if config file has modified
**
** Returns:     0 if not modified, 1 otherwise.
**
*******************************************************************************/
extern "C" int isNxpConfigModified() {
  bool modified = CNfcConfig::GetInstance().isModified(CONF_FILE_NXP);
  return modified ? 1 : 0;
}

/*******************************************************************************
**
** Function:    isNxpRFConfigModified()
**
** Description: check if config file has modified
**
** Returns:     0 if not modified, 1 otherwise.
**
*******************************************************************************/
extern "C" int isNxpRFConfigModified() {
  CNfcConfig& rConfig = CNfcConfig::GetInstance();

  int retRF = rConfig.isModified(CONF_FILE_NXP_RF) ? 1 : 0;
  int retTransit = rConfig.isModified(CONF_FILE_NXP_TRANSIT) ? 1 : 0;
  int ret = retRF | retTransit;
  ALOGD("%s RF config modification: RF=%s, Transit=%s, Combined=%s",
        __func__,
        retRF ? "MODIFIED" : "NOT MODIFIED",
        retTransit ? "MODIFIED" : "NOT MODIFIED",
        ret ? "MODIFIED" : "NOT MODIFIED");

  return ret;
}
/*******************************************************************************
**
** Function:    updateNxpConfigTimestamp()
**
** Description: update if config file has modified
**
** Returns:     0 if not modified, 1 otherwise.
**
*******************************************************************************/
extern "C" int updateNxpConfigTimestamp() {
  CNfcConfig::GetInstance().resetModified(CONF_FILE_NXP);
  return 0;
}

/*******************************************************************************
**
** Function:    updateNxpConfigTimestamp()
**
** Description: update if config file has modified
**
** Returns:     0 if not modified, 1 otherwise.
**
*******************************************************************************/
extern "C" int updateNxpRfConfigTimestamp() {
  CNfcConfig& rConfig = CNfcConfig::GetInstance();
  rConfig.resetModified(CONF_FILE_NXP_RF);
  rConfig.resetModified(CONF_FILE_NXP_TRANSIT);
  return 0;
}
