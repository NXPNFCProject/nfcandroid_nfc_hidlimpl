/*
 *
 *  The original Work has been changed by NXP.
 *
 *  Copyright 2024 NXP
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
 */

package com.nxp.nfc.vendor.mpos;

/**
 * Enum representing the action events for SCR NTF (SE Reader).
 */
public enum ScrNtfActionEvent {
  SE_READER_START_RF_DISCOVERY(0x00),         /* Start RF discovery */
  SE_READER_START_DEFAULT_RF_DISCOVERY(0x01), /* Start default RF discovery */
  SE_READER_TAG_DISCOVERY_STARTED(0x02),      /* Tag discovery started */
  SE_READER_TAG_DISCOVERY_START_FAILED(0x03), /* Tag discovery start failed */
  SE_READER_TAG_DISCOVERY_RESTARTED(0x04),    /* Tag discovery restarted */
  SE_READER_TAG_ACTIVATED(0x05),              /* Tag activated */
  SE_READER_STOP_RF_DISCOVERY(0x06),          /* Stop RF discovery */
  SE_READER_STOPPED(0x07),                    /* RF discovery stopped */
  SE_READER_STOP_FAILED(0x08),                /* Failed to stop RF discovery */
  SE_READER_TAG_TIMEOUT(0x09),                /* Tag timeout occurred */
  SE_READER_TAG_REMOVE_TIMEOUT(0x0A),         /* Tag removal timeout occurred */
  SE_READER_MULTIPLE_TAG_DETECTED(0x0B),      /* Multiple tags detected */
  SE_READER_UNKNOWN(0x0C); /* Unknown event. Ignore this event */

  private final byte value;

  ScrNtfActionEvent(int value) { this.value = (byte)value; }

  public byte getValue() { return value; }

  public static ScrNtfActionEvent fromValue(byte value) {
    for (ScrNtfActionEvent event : values()) {
      if (event.getValue() == value) {
        return event;
      }
    }
    return SE_READER_UNKNOWN;
  }
}
