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

#include "phNxpNciHal_VendorProp.h"
#include <cutils/properties.h>
#include <string>

/******************************************************************************
 * Function         phNxpNciHal_getVendorProp_int32
 *
 * Description      This function will read and return property
 *                  value of input key as integer.
 * Parameters       key - property string for which value to be read.
 *                  default_value - default value to be return if property not
 *                  set.
 *
 * Returns          integer value of key from vendor properties if set else
 *                  return the input default_value.
 *
 ******************************************************************************/
int32_t phNxpNciHal_getVendorProp_int32(const char* key,
                                        int32_t default_value) {
  return property_get_int32(key, default_value);
}

/******************************************************************************
 * Function         phNxpNciHal_setVendorProp
 *
 * Description      This function will set the value for input property.
 *
 * Parameters       key - property string for which value to be set.
 *                  value - value of key property be set.
 *
 * Returns          returns 0 on success and, < 0 on failure
 *
 ******************************************************************************/
int phNxpNciHal_setVendorProp(const char* key, const char* value) {
  return property_set(key, value);
}

/******************************************************************************
 * Function         phNxpNciHal_getVendorProp
 *
 * Description      This function will read and return property
 *                  value of input key as string.
 * Parameters       key - property string for which value to be read.
 *                  value - output value of property.
 *
 * Returns          actual length of the property value.
 *
 ******************************************************************************/
int phNxpNciHal_getVendorProp(const char* key, char* value) {
  return property_get(key, value, NULL);
}

/******************************************************************************
 * Function         phNxpNciHal_getFragmentedVendorProp
 *
 * Description      This function will read and return property
 *                  value of input key which is stored in multiple fragments.
 * Parameters       key - property string for which value to be read.
 *                  value - output value of property.
 *
 * Returns          actual length of the property value.
 *
 ******************************************************************************/
int phNxpNciHal_getFragmentedVendorProp(const char* key, char* value) {
  if (key == NULL || value == NULL) {
    return 0;
  }
  // Check for fragmented property
  for (int i = 1;; i++) {
    std::string propName = key;
    propName.append(std::to_string(i));
    char propValue[PROPERTY_VALUE_MAX];

    if (property_get(propName.c_str(), propValue, NULL) < 1) {
      // Completed reading of all chunks
      break;
    } else {
      strcat(value, propValue);
    }
  }
  return strlen(value);
}

/******************************************************************************
 * Function         phNxpNciHal_setFragmentedVendorProp
 *
 * Description      This function will set the value for input property
 *                  which are large to store in single property hence
 *                  needs to be fragmented.
 *
 * Parameters       key - property string for which value to be set.
 *                  value - value of key property be set.
 *
 * Returns          returns 0 on success and, < 0 on failure
 *
 ******************************************************************************/
int phNxpNciHal_setFragmentedVendorProp(const char* key, const char* value) {
  if (key == NULL) {
    return -1;
  }
  int fragmentSize = PROPERTY_VALUE_MAX - 1;  // One byte for null character
  // Clear all previous fragments
  char previousValue[1024];
  size_t prevLen = phNxpNciHal_getFragmentedVendorProp(key, previousValue);
  for (size_t i = 0; i < prevLen; i += fragmentSize) {
    std::string propName = key;
    propName.append(std::to_string((i / fragmentSize) + 1));
    property_set(propName.c_str(), NULL);
  }

  // Store the property in multiple fragments
  int len = strlen(value);
  for (size_t i = 0; i < (size_t)len; i += fragmentSize) {
    std::string propName = key;
    propName.append(std::to_string((i / fragmentSize) + 1));
    std::string propValue(value + i, fragmentSize);
    int status = property_set(propName.c_str(), propValue.c_str());
    if (status < 0) {
      return status;
    }
  }
  return strlen(value);
}