/******************************************************************************
 *
 *  Copyright 2022 NXP
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

package vendor.nxp.nxpnfc_aidl;

import vendor.nxp.nxpnfc_aidl.NxpNfcHalEseState;

@VintfStability
interface INxpNfc {
    // Adding return type to method instead of out param String value since there is only one return value.
    /**
     * Gets vendor params values whose Key has been provided.
     *
     * @param string
     * @param out output data as string
     */
    String getVendorParam(in String key);

    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * api to check jcop update is required or not
     *
     * @param none
     * @return as a boolean, true if JCOP update required, false if not required.
     */
    boolean isJcopUpdateRequired();

    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * api to check LS update is required or not
     *
     * @param none
     * @return as a boolean, true if LS update required, false if not required.
     */
    boolean isLsUpdateRequired();

    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * reset the ese based on resettype
     *
     * @param uint64_t to specify resetType
     * @return as a boolean, true if success, false if failed
     */
    boolean resetEse(in long resetType);

    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * updates ese with current state and notifies upper layer
     *
     * @param input current ese state to set
     * @return as a boolean, true if success, false if failed
     */
    boolean setEseUpdateState(in NxpNfcHalEseState eSEState);

    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * Sets Transit config value
     *
     * @param string transit config value
     * @return as a boolean, true if success, false if failed
     */
    boolean setNxpTransitConfig(in String transitConfValue);

    // Adding return type to method instead of out param boolean status since there is only one return value.
    /**
     * Saves the vendor params provided as key-value pair
     *
     * @param string key string value
     * @return as a boolean, true if success, false if failed
     */
    boolean setVendorParam(in String key, in String value);
}
