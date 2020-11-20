# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if("/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/libevent-2.1.8-stable.tar.gz" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/libevent-2.1.8-stable.tar.gz")
  message(FATAL_ERROR "File not found: /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/libevent-2.1.8-stable.tar.gz")
endif()

if("MD5" STREQUAL "")
  message(WARNING "File will not be verified since no URL_HASH specified")
  return()
endif()

if("f3eeaed018542963b7d2416ef1135ecc" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(STATUS "verifying file...
     file='/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/libevent-2.1.8-stable.tar.gz'")

file("MD5" "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/libevent-2.1.8-stable.tar.gz" actual_value)

if(NOT "${actual_value}" STREQUAL "f3eeaed018542963b7d2416ef1135ecc")
  message(FATAL_ERROR "error: MD5 hash of
  /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/libevent-2.1.8-stable.tar.gz
does not match expected value
  expected: 'f3eeaed018542963b7d2416ef1135ecc'
    actual: '${actual_value}'
")
endif()

message(STATUS "verifying file... done")
