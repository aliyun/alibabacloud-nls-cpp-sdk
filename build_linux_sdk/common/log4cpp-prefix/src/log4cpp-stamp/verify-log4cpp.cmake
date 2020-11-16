# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if("/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/log4cpp-1.1.3.tar.gz" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/log4cpp-1.1.3.tar.gz")
  message(FATAL_ERROR "File not found: /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/log4cpp-1.1.3.tar.gz")
endif()

if("MD5" STREQUAL "")
  message(WARNING "File will not be verified since no URL_HASH specified")
  return()
endif()

if("b9e2cee932da987212f2c74b767b4d8b" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(STATUS "verifying file...
     file='/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/log4cpp-1.1.3.tar.gz'")

file("MD5" "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/log4cpp-1.1.3.tar.gz" actual_value)

if(NOT "${actual_value}" STREQUAL "b9e2cee932da987212f2c74b767b4d8b")
  message(FATAL_ERROR "error: MD5 hash of
  /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/log4cpp-1.1.3.tar.gz
does not match expected value
  expected: 'b9e2cee932da987212f2c74b767b4d8b'
    actual: '${actual_value}'
")
endif()

message(STATUS "verifying file... done")
