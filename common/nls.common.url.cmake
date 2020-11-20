
#jsoncpp
set(JSONCPP_URL  ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp-0.y.z.zip)
set(JSONCPP_URL_HASH 8da4bafedec6d31886cb9d9c6606638f)

#opus
set(OPUS_URL ${CMAKE_CURRENT_SOURCE_DIR}/opus-1.2.1.tar.gz)
set(OPUS_URL_HASH 54bc867f13066407bc7b95be1fede090)

#uuid
set(UUID_URL ${CMAKE_CURRENT_SOURCE_DIR}/libuuid-1.0.3.tar.gz)
set(UUID_URL_HASH d44d866d06286c08ba0846aba1086d68)

#openssl
if(ENABLE_BUILD_ANDROID)
set(OPENSSL_URL ${CMAKE_CURRENT_SOURCE_DIR}/openssl-1.0.2j.tar.gz)
set(OPENSSL_URL_HASH 96322138f0b69e61b7212bc53d5e912b)
else()
set(OPENSSL_URL ${CMAKE_CURRENT_SOURCE_DIR}/openssl-OpenSSL_1_1_0j.zip)
set(OPENSSL_URL_HASH 8c52672e2b808ca829226f9afff177d7)
endif()

#log4cpp
set(LOG4CPP_URL ${CMAKE_CURRENT_SOURCE_DIR}/log4cpp-1.1.3.tar.gz)
set(LOG4CPP_URL_HASH b9e2cee932da987212f2c74b767b4d8b)

#libevent
if(ENABLE_BUILD_WINDOWS)
set(LIBEVENT_URL ${CMAKE_CURRENT_SOURCE_DIR}/libevent-release-2.1.8-stable.zip)
set(LIBEVENT_URL_HASH 94335d8468382f2debaf70c7377652d8)
elseif(ENABLE_BUILD_ANDROID)
set(LIBEVENT_URL ${CMAKE_CURRENT_SOURCE_DIR}/libevent-release-2.1.8-stable-android.zip)
set(LIBEVENT_URL_HASH 0fa3aebca46228ea4ca054db446c6472)
elseif(ENABLE_BUILD_IOS)
#set(LIBEVENT_URL ${CMAKE_CURRENT_SOURCE_DIR}/libevent-release-2.1.8-stable.zip)
#set(LIBEVENT_URL_HASH 94335d8468382f2debaf70c7377652d8)
else()
set(LIBEVENT_URL ${CMAKE_CURRENT_SOURCE_DIR}/libevent-2.1.8-stable.tar.gz)
set(LIBEVENT_URL_HASH f3eeaed018542963b7d2416ef1135ecc)
endif()

#gtest
set(GTEST_URL ${CMAKE_CURRENT_SOURCE_DIR}/googletest-release-1.8.0.zip)
set(GTEST_URL_HASH adfafc8512ab65fd3cf7955ef0100ff5)