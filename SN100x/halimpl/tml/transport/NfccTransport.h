/******************************************************************************
 *
 *  Copyright 2020 NXP
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

#pragma once
#include <phNfcTypes.h>
#include <phTmlNfc.h>

enum NfccResetType : long {
  MODE_POWER_OFF = 0x00,
  MODE_POWER_ON,
  MODE_FW_DWNLD_WITH_VEN,
  MODE_ISO_RST,
  MODE_FW_DWND_HIGH,
  MODE_POWER_RESET,
  MODE_FW_GPIO_LOW
};

enum EseResetType : long {
  MODE_ESE_POWER_ON = 0,
  MODE_ESE_POWER_OFF,
  MODE_ESE_POWER_STATE,
  MODE_ESE_COLD_RESET
};

extern phTmlNfc_i2cfragmentation_t fragmentation_enabled;

class NfccTransport {
 public:
  /*****************************************************************************
   **
   ** Function         Close
   **
   ** Description      Closes PN54X device
   **
   ** Parameters       pDevHandle - device handle
   **
   ** Returns          None
   **
   *****************************************************************************/
  virtual void Close(void *pDevHandle) = 0;

  /*****************************************************************************
   **
   ** Function         OpenAndConfigure
   **
   ** Description      Open and configure PN54X device and transport layer
   **
   ** Parameters       pConfig     - hardware information
   **                  pLinkHandle - device handle
   **
   ** Returns          NFC status:
   **                  NFCSTATUS_SUCCESS - open_and_configure operation success
   **                  NFCSTATUS_INVALID_DEVICE - device open operation failure
   **
   ****************************************************************************/
  virtual NFCSTATUS OpenAndConfigure(pphTmlNfc_Config_t pConfig,
                                     void **pLinkHandle) = 0;

  /*****************************************************************************
   **
   ** Function         Read
   **
   ** Description      Reads requested number of bytes from PN54X device into
   **                 given buffer
   **
   ** Parameters       pDevHandle       - valid device handle
   **                  pBuffer          - buffer for read data
   **                  nNbBytesToRead   - number of bytes requested to be read
   **
   ** Returns          numRead   - number of successfully read bytes
   **                  -1        - read operation failure
   **
   ****************************************************************************/
  virtual int Read(void *pDevHandle, uint8_t *pBuffer, int nNbBytesToRead) = 0;

  /*****************************************************************************
   **
   ** Function         Write
   **
   ** Description      Writes requested number of bytes from given buffer into
   **                  PN54X device
   **
   ** Parameters       pDevHandle       - valid device handle
   **                  pBuffer          - buffer for read data
   **                  nNbBytesToWrite  - number of bytes requested to be
   *written
   **
   ** Returns          numWrote   - number of successfully written bytes
   **                  -1         - write operation failure
   **
   *****************************************************************************/
  virtual int Write(void *pDevHandle, uint8_t *pBuffer,
                    int nNbBytesToWrite) = 0;

  /*****************************************************************************
   **
   ** Function         Reset
   **
   ** Description      Reset PN54X device, using VEN pin
   **
   ** Parameters       pDevHandle     - valid device handle
   **                  eType          - NfccResetType
   **
   ** Returns           0   - reset operation success
   **                  -1   - reset operation failure
   **
   ****************************************************************************/
  virtual int NfccReset(void *pDevHandle, NfccResetType eType);

  /*****************************************************************************
   **
   ** Function         EseReset
   **
   ** Description      Request NFCC to reset the eSE
   **
   ** Parameters       pDevHandle     - valid device handle
   **                  eType          - EseResetType
   **
   ** Returns           0   - reset operation success
   **                  else - reset operation failure
   **
   ****************************************************************************/
  virtual int EseReset(void *pDevHandle, EseResetType eType);

  /*****************************************************************************
   **
   ** Function         EseGetPower
   **
   ** Description      Request NFCC to reset the eSE
   **
   ** Parameters       pDevHandle     - valid device handle
   **                  level          - reset level
   **
   ** Returns           0   - reset operation success
   **                  else - reset operation failure
   **
   ****************************************************************************/
  virtual int EseGetPower(void *pDevHandle, long level);

  /*****************************************************************************
   **
   ** Function         GetPlatform
   **
   ** Description      Get platform interface type (i2c or i3c) for common mw
   **
   ** Parameters       pDevHandle     - valid device handle
   **
   ** Returns           0   - i2c
   **                   1   - i3c
   **
   ****************************************************************************/
  virtual int GetPlatform(void *pDevHandle);

  /*****************************************************************************
   **
   ** Function         EnableFwDnldMode
   **
   ** Description      updates the state to Download mode
   **
   ** Parameters       True/False
   **
   ** Returns          None
   ****************************************************************************/
  virtual void EnableFwDnldMode(bool mode);

  /*****************************************************************************
   **
   ** Function         IsFwDnldModeEnabled
   **
   ** Description      Returns the current mode
   **
   ** Parameters       none
   **
   ** Returns          Current mode download/NCI
   ****************************************************************************/
  virtual bool_t IsFwDnldModeEnabled(void);

  /*****************************************************************************
   **
   ** Function         ~NfccTransport
   **
   ** Description      TransportLayer destructor
   **
   ** Parameters       none
   **
   ** Returns          None
   ****************************************************************************/
  virtual ~NfccTransport(){};
};