LOCAL_PATH := $(call my-dir)

# make dbus-daemon
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := dbus-daemon
LOCAL_INIT_RC := dbus-daemon.rc
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# make dbus-launch
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := dbus-launch
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# make envars.sh
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := envars.sh
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# make session.conf
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := session.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)

# make libdbus-1.so
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libdbus-1.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := libdbus-1.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_PREBUILT)

# make libexpat.so.1
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libexpat.so.1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := libexpat.so.1
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_PREBUILT)

