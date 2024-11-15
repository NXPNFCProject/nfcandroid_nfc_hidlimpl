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

package com.nxp.nfc.vendor.qtag;

public enum QTagMode {
  DISABLE_QTAG_MODE(0),
  ENABLE_QTAG_ONLY_MODE(1),
  APPEND_QTAG_MODE(2),
  INVALID_MODE(3);

  private final int mValue;

  QTagMode(int value) { this.mValue = (int)value; }

  public int getValue() { return mValue; }

  public static QTagMode fromValue(int value) {
    for (QTagMode mode : values()) {
      if (mode.getValue() == value)
        return mode;
    }
    return INVALID_MODE;
  }
}
