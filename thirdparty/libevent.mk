#LOCAL_PATH := $(INSTALL_DIR)

include $(CLEAR_VARS)

#include the .mk files

MY_MODULE_DIR       := event

LOCAL_MODULE        := $(MY_MODULE_DIR)

#ARM build
LOCAL_SRC_FILES     := \
    $(LOCAL_PATH)/buffer.c \
    $(LOCAL_PATH)/bufferevent.c \
    $(LOCAL_PATH)/bufferevent_filter.c \
    $(LOCAL_PATH)/bufferevent_pair.c \
    $(LOCAL_PATH)/bufferevent_ratelim.c \
    $(LOCAL_PATH)/bufferevent_sock.c \
    $(LOCAL_PATH)/epoll.c \
    $(LOCAL_PATH)/epoll_sub.c \
    $(LOCAL_PATH)/evdns.c \
    $(LOCAL_PATH)/event.c \
    $(LOCAL_PATH)/event_tagging.c \
    $(LOCAL_PATH)/evmap.c \
    $(LOCAL_PATH)/evrpc.c \
    $(LOCAL_PATH)/evthread.c \
    $(LOCAL_PATH)/evthread_pthread.c \
    $(LOCAL_PATH)/evutil.c \
    $(LOCAL_PATH)/evutil_rand.c\
    $(LOCAL_PATH)/http.c \
    $(LOCAL_PATH)/listener.c \
    $(LOCAL_PATH)/log.c \
    $(LOCAL_PATH)/poll.c \
    $(LOCAL_PATH)/select.c \
    $(LOCAL_PATH)/signal.c \
    $(LOCAL_PATH)/strlcpy.c \
    $(LOCAL_PATH)/evutil_time.c

LOCAL_C_INCLUDES    := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/include \
    $(OTHER_PATH)

# don't disable the float api
LOCAL_CFLAGS        := -fPIC -DHAVE_CONFIG_H -DANDROID -fvisibility=hidden
LOCAL_CPPFLAGS      := -fPIC -DHAVE_CONFIG_H -DANDROID -fvisibility=hidden

include $(BUILD_STATIC_LIBRARY)
