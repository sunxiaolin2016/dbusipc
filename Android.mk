LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    src/Command.cpp \
    src/Connection.cpp \
    src/DBusErrorHolder.cpp \
    src/DBusTimeoutWrapper.cpp \
    src/DBusWatchWrapper.cpp \
    src/Dispatcher.cpp \
    src/InterfaceDefs.cpp \
    src/MutexLock.cpp \
    src/NSysDep.cpp \
    src/NUtil.cpp \
    src/Pipe.cpp \
    src/PipeWatch.cpp \
    src/RequestContext.cpp \
    src/ScopedLock.cpp \
    src/Semaphore.cpp \
    src/Semaphore_POSIX.cpp \
    src/ServiceRegistration.cpp \
    src/SignalSubscription.cpp \
    src/svcipc_api.cpp \
    src/svcipc_error.c \
    src/Thread.cpp \
    src/Timeout.cpp \
    src/trace.cpp \
    src/Watch.cpp



LOCAL_C_INCLUDES +=$(LOCAL_PATH)/inc $(LOCAL_PATH)/src/


LOCAL_SHARED_LIBRARIES :=  \
        libcutils \
        libutils \
        liblog

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libdbusipc
LOCAL_MULTILIB := 64

LOCAL_CFLAGS += -Wall -Werror -Wno-unused-parameter -Wunreachable-code -DOS_LINUX -fexceptions
LOCAL_LDLIBS += $(LOCAL_PATH)/libdbus-1.so


include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
