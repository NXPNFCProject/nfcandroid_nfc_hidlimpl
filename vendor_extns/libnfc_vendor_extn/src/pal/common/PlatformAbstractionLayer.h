/**
 *
 *  Copyright 2024-2026 NXP
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
 **/

#ifndef PLATFORM_ABSTRACTION_LAYER_H
#define PLATFORM_ABSTRACTION_LAYER_H

#include "PlatformBase.h"
#include "PlatformExtension.h"
#include "phNxpConfig.h"
#include "vector"
#include <map>
#include <string>

class PlatformAbstractionLayer : public PlatformBase {
public:
  PlatformAbstractionLayer(const PlatformAbstractionLayer &) = delete;
  PlatformAbstractionLayer &
  operator=(const PlatformAbstractionLayer &) = delete;

  /**
   * @brief Get the singleton of this object.
   * @return Reference to this object.
   *
   */
  static PlatformAbstractionLayer *getInstance();

  /**
   *
   * @brief           This function write the data to NFCC through physical
   *                  interface (e.g. I2C) using the PN7220 driver interface.
   *                  It enqueues the data and writer thread picks and process
   * it. Before sending the data to NFCC, phEMVCoHal_write_ext is called to
   * check if there is any extension processing is required for the NCI packet
   * being sent out.
   *
   * @param[in]       data_len length of the data to be written
   * @param[in]       p_data actual data to be written
   *
   * @return          uint16_t - returns the number of bytes written to
   * controller
   *
   */
  uint16_t palenQueueWrite(const uint8_t *pBuffer, uint16_t wLength);

  /**
   *
   * @brief           This function helps to send the response and notification
   *                  asynchronously to upper layer
   *
   * @param[in]       data_len length of the data to be written
   * @param[in]       p_data actual data to be written
   *
   * @return          uint16_t - SUCCESS or FAILURE
   *
   */
  uint16_t palenQueueRspNtf(const uint8_t *pBuffer, uint16_t wLength);

  /**
   * @brief This function can be used by HAL to request control of
   *        NFCC to libnfc-nci. When control is provided to HAL it is
   *        notified through phNxpNciHal_control_granted.
   * @return void
   *
   */
  void palRequestHalControl();

  /**
   * @brief This function can be used by HAL to release the control of
   *        NFCC back to libnfc-nci.
   * @return void
   *
   */
  void palReleaseHalControl();

  /**
   * @brief sends the NCI packet to upper layer
   * @param  data_len length of the NCI packet
   * @param  p_data data buffer pointer
   * @return void
   *
   */
  void palSendNfcDataCallback(uint16_t dataLen, const uint8_t *pData);

  /**
   * @brief Read byte array value from the config file.
   * @param name - name of the config param to read.
   * @param pValue - pointer to input buffer.
   * @param bufflen - input buffer length.
   * @param len - out parameter to return the number of bytes read from
   *                      config file, return -1 in case bufflen is not enough.
   * @return 1 if config param name is found in the config file, else 0
   *
   */
  uint8_t palGetNxpByteArrayValue(const char *name, char *pValue, long bufflen,
                                  long *len);

  /**
   * @brief API function for getting a numerical value of a setting
   * @param name - name of the config param to read.
   * @param pValue - pointer to input buffer.
   * @param len - sizeof required pValue
   * @return 1 if config param name is found in the config file, else 0
   */
  uint8_t palGetNxpNumValue(const char *name, void *pValue, unsigned long len);

  /**
   * @brief This function can be called to enable/disable the log levels
   * @param enable 0x01 to enable for 0x00 to disable
   * @return NFASTATUS_SUCCESS if success else NFCSTATUS_FAILED
   */
  uint8_t palEnableDisableDebugLog(uint8_t enable);

  /**
   * @brief responsible for setting cover attached system propertires
   * @return None
   *
   */
  void coverAttached(std::string state, std::string type);

  /**
   * @brief required config values can be overridden
   * @return None
   *
   */
  void updateHalConfig(std::map<std::string, ConfigValue>* pConfigMap);

  /**
   * @brief Saves the vendor params provided as key-value pair
   * by invoking the NxpExtns::setVendorParam
   * @param paramKey key of the vendorParam
   * @param paramValue value of the vendorParam
   * @return True if setting vendorParam is success else false
   *
   */
  bool setVendorParam(const std::string& paramKey,
                         const std::string& paramValue);

  /**
   * @brief Invoke the NxpExtns::getVendorParam
   * @param paramKey key of the vendorParam
   * @return value of the paramKey if found else empty string
   *
   */
  std::string getVendorParam(const std::string& paramKey);

  /**
   * @brief send Hal events to the libnfc-nci asynchronously
   * @param evt hal event
   * @param evtStatus status of the hal event
   * @return None
   *
   */
  void palEnQueueEvt(uint8_t evt, uint8_t evtStatus);

  /**
  * @brief Retrieve a legacy config name from lookup table.
  * @param key The configuration key to look up.
  * @param found Output parameter that is set to true
  *        if the key has a legacy mapping, false otherwise.
  * @return The legacy config value, otherwise default value
  *        from lookup table.
  */
  uint32_t palGetLibNciConfig(const char* key, bool& found);

  /**
   * @brief It will write the data/cmd synchronously to i2c channel.
   *        Notifies upper layer using callback mechanism.
   * @param  data to be sent
   * @param  length of data buffer
   * @return Returns NFCSTATUS_SUCCESS if sending cmd is successful and
   *         response is received.
   *
   */
  NFCSTATUS palNfcTmlWrite(uint8_t *pBuffer, uint16_t wLength);

  /**
   * @brief Returns the status to create HCI pipe.
   * @return True or False.
   */
  bool palIsHciPipeRequireToCreate();

  /**
   * @brief Retrieves the default NFCC discovery command.
   *
   * This function constructs and returns the discovery command that is sent to
   * the NFCC (NFC Controller) to initiate the discovery process. The returned
   * buffer contains the complete command frame formatted according to the
   * platform/NFCC requirements.
   *
   * @return A vector of bytes representing the discovery command.
   */
  std::vector<uint8_t> palGetDiscoveryCommand();

  /**
   * @brief Sends an extension command to the NFCC and receives the response.
   *
   * @param cmd_len   Length of the command buffer in bytes.
   * @param p_cmd     Pointer to the command buffer.
   * @param rsp_len   Input: size of the response buffer.
   *                  Output: actual number of response bytes received.
   * @param p_rsp     Pointer to the response buffer where NFCC response will be
   * stored.
   *
   * @return NFCSTATUS_SUCCESS on success, or an appropriate NFCSTATUS error
   * code on failure.
   */
  NFCSTATUS palNfcSendExtCmd(uint16_t cmd_len, uint8_t *p_cmd,
                             uint16_t *rsp_len, uint8_t *p_rsp);

  /**
   * @brief Get the status of observer mode status.
   * @return True or False.
   */
  bool palGetObserveModeStatus();
  /**
   * @brief Release all resources.
   * @return None
   *
   */
  static inline void finalize() {
    if (sPlatformAbstractionLayer != nullptr) {
      sPlatformAbstractionLayer.reset();
    }
  }

private:
  std::string COVER_ID_PROP = "nfc.cover.cover_id";
  std::string COVER_STATE_PROP = "nfc.cover.state";
  static std::unique_ptr<PlatformAbstractionLayer> sPlatformAbstractionLayer; // singleton object
  /**
   * @brief Initialize member variables.
   * @return None
   *
   */
  PlatformAbstractionLayer();

  /**
   * @brief Release all resources.
   * @return None
   *
   */
  ~PlatformAbstractionLayer();

  /**
   * @brief get the nxpnfc hidl/aidl service instance
   * @return None
   *
   */
  void getNxpNfcHal();

  /**
   * @brief send Hal events to the libnfc-nci
   * @param evt hal event
   * @param evtStatus status of the hal event
   * @return None
   *
   */
  void palSendNfcEventCallback(uint8_t evt, uint8_t evtStatus);
  friend struct std::default_delete<PlatformAbstractionLayer>;
};
/** @}*/
#endif // PLATFORM_ABSTRACTION_LAYER_H
