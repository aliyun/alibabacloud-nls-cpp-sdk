# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if("/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/jsoncpp-0.y.z.zip" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/jsoncpp-0.y.z.zip")
  message(FATAL_ERROR "File not found: /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/jsoncpp-0.y.z.zip")
endif()

if("MD5" STREQUAL "")
  message(WARNING "File will not be verified since no URL_HASH specified")
  return()
endif()

if("8da4bafedec6d31886cb9d9c6606638f" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(STATUS "verifying file...
     file='/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/jsoncpp-0.y.z.zip'")

file("MD5" "/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/jsoncpp-0.y.z.zip" actual_value)

if(NOT "${actual_value}" STREQUAL "8da4bafedec6d31886cb9d9c6606638f")
  message(FATAL_ERROR "error: MD5 hash of
  /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/common/jsoncpp-0.y.z.zip
does not match expected value
  expected: '8da4bafedec6d31886cb9d9c6606638f'
    actual: '${actual_value}'
")
endif()

message(STATUS "verifying file... done")
