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

#include <stdint.h>

/******************************************************************************
 * Function         phNxpNciHal_getVendorProp_int32
 *
 * Description      This function will read and return property
 *                  value of input key as integer.
 * Parameters       key - property string for which vaue to be read.
 *                  default_value - default value to be return if property not
 *                  set.
 *
 * Returns          integer value of key from vendor properties if set else
 *                  return the input default_value.
 *
 ******************************************************************************/
int32_t phNxpNciHal_getVendorProp_int32(const char* key, int32_t default_value);

/******************************************************************************
 * Function         phNxpNciHal_setVendorProp
 *
 * Description      This function will set the value for input property.
 *
 * Parameters       key - property string for which vaue to be set.
 *                  value - value of key property be set.
 *
 * Returns          returns 0 on success and, < 0 on failure
 *
 ******************************************************************************/
int phNxpNciHal_setVendorProp(const char* key, const char* value);
