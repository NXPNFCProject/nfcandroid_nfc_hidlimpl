#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libnxp_nciparser

LOCAL_PRELINK_MODULE := false

OSAL   := $(LOCAL_PATH)/osal

PARSER := $(LOCAL_PATH)/parser

ifeq (true,$(TARGET_IS_64_BIT))
LOCAL_MULTILIB := 64
else
LOCAL_MULTILIB := 32
endif

LOCAL_SHARED_LIBRARIES := libcutils liblog libbase libutils

LOCAL_C_INCLUDES += $(OSAL)/inc $(PARSER)/inc

LOCAL_SRC_FILES := $(call all-subdir-cpp-files) $(call all-subdir-c-files)

LOCAL_CFLAGS += -Wall -Wextra -Wno-unused-parameter

include $(BUILD_SHARED_LIBRARY)
