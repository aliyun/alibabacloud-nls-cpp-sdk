# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if("/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/opus-1.2.1.tar.gz" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/opus-1.2.1.tar.gz")
  message(FATAL_ERROR "File not found: /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/opus-1.2.1.tar.gz")
endif()

if("MD5" STREQUAL "")
  message(WARNING "File will not be verified since no URL_HASH specified")
  return()
endif()

if("54bc867f13066407bc7b95be1fede090" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(STATUS "verifying file...
     file='/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/opus-1.2.1.tar.gz'")

file("MD5" "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/opus-1.2.1.tar.gz" actual_value)

if(NOT "${actual_value}" STREQUAL "54bc867f13066407bc7b95be1fede090")
  message(FATAL_ERROR "error: MD5 hash of
  /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/opus-1.2.1.tar.gz
does not match expected value
  expected: '54bc867f13066407bc7b95be1fede090'
    actual: '${actual_value}'
")
endif()

message(STATUS "verifying file... done")
