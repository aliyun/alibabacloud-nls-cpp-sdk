# Install script for directory: /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/assertions.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/autolink.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/config.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/features.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/forwards.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/json.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/reader.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/value.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/version.h;/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json/writer.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/include/jsoncpp/json" TYPE FILE FILES
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/assertions.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/autolink.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/config.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/features.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/forwards.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/json.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/reader.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/value.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/version.h"
    "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/common/jsoncpp-prefix/src/jsoncpp/include/json/writer.h"
    )
endif()

