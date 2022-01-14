#LOCAL_PATH := $(INSTALL_DIR)

include $(CLEAR_VARS)

#include the .mk files

MY_MODULE_DIR       := uuid

LOCAL_MODULE        := $(MY_MODULE_DIR)

#ARM build
LOCAL_SRC_FILES     := \
    $(LOCAL_PATH)/clear.c \
    $(LOCAL_PATH)/compare.c \
    $(LOCAL_PATH)/copy.c \
    $(LOCAL_PATH)/gen_uuid.c \
    $(LOCAL_PATH)/isnull.c \
    $(LOCAL_PATH)/pack.c \
    $(LOCAL_PATH)/parse.c \
    $(LOCAL_PATH)/unpack.c \
    $(LOCAL_PATH)/unparse.c \
    $(LOCAL_PATH)/uuid_time.c \
    $(LOCAL_PATH)/randutils.c

LOCAL_C_INCLUDES    := \
    $(LOCAL_PATH)/include

# don't disable the float api
LOCAL_CFLAGS        := -DHAVE_USLEEP -fPIC -fvisibility=hidden
LOCAL_CPPFLAGS      := -DHAVE_USLEEP -fPIC -fvisibility=hidden

include $(BUILD_STATIC_LIBRARY)
