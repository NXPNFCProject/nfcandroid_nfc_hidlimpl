/*
 * Copyright 2024 NXP
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

package com.nxp.nfc;

/**
 * @class INxpNfcExtras
 * @brief Interface to perform the OEM Extension functionality.
 *
 */
public interface INxpNfcExtras {

    /**
     * @brief Updates the RF configuration when cover state is changed
     *
     * @param attached
     * @param coverType
     *
     * @return void
     */
    void coverAttached(boolean attached, int coverType);

    /**
     * @brief starts the cover auth
     *
     * @return response
     */
    byte[] startCoverAuth();

    /**
     * @brief stops the cover auth
     *
     * @return true on success else false
     */
    boolean stopCoverAuth();

    /**
     * @brief trans the passed  data
     *
     * @param data
     *
     * @return response
     */
    byte[] transceiveAuthData(byte[] data);
}
