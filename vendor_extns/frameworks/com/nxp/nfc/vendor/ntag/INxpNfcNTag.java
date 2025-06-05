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

package com.nxp.nfc.vendor.ntag;

import android.annotation.IntDef;
import android.app.Activity;
import com.nxp.nfc.vendor.ntag.NxpNfcNTag.NxpNTagStatusCallback;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * @class INxpNfcNTag
 * @brief Interface to perform the NFC NTag functionality.
 *
 * @hide
 */
public interface INxpNfcNTag {

  /**
   * Possible mode from {@link #NTagMode}.
   *
   */
  public static final int ENABLE_NTAG = 0x01;
  public static final int DISABLE_NTAG = 0x00;

  public static final int STATUS_SUCCESS = 0x00;
  public static final int STATUS_FAILED = 0x01;

  @IntDef(value =
              {
                  ENABLE_NTAG,
                  DISABLE_NTAG,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface NTagMode {}

  @IntDef(value =
              {
                  STATUS_SUCCESS,
                  STATUS_FAILED,
              })
  @Retention(RetentionPolicy.SOURCE)
  public @interface NTagStatus {}

  /**
   * This is provides the info whether NTag mode is enabled or not
   * <li>This api shall return false if Nfcservice is disabled.
   * </ul>
   * @return TRUE if NTag mode is enabled
   *         FALSE if NTag mode is disabled
   * @throws IOException If a failure occurred during NTag RF mode set or reset
   */
  public boolean isNTagEnabled() throws IOException;

  /**
   * This is the API to be called to enable or disable NTag RF mode.
   * <li>This api shall be called only when Nfcservice is enabled.
   * <li>This api shall be called only when there are no NFC transactions
   * ongoing.
   * <li> Tag DISCOVERED intent will be sent to application once read complete.
   * </ul>
   * @param  activity activity the Activity that requests the adapter to be in
   *     reader mode.
   * @param mNTagStatusCallback the callback to be called when NTAG is
   *     added/removed
   *      UID value and detected status will be to updated when tag is added.
   *      Removed status will be update on tag removal.
   * @param  mode to ENABLE_NTAG.
   *                 DISABLE_NTAG & reset to default discovery.
   * @return whether the update of state is
   *          0x00 - STATUS_SUCCESS,
   *          0x01 - STATUS_FAILED,
   * @throws IOException If a failure occurred during NTag RF mode set or reset
   */
  public @NTagStatus int setNTagMode(Activity activity,
                                     NxpNTagStatusCallback mNTagStatusCallback,
                                     @NTagMode int mode) throws IOException;
}
