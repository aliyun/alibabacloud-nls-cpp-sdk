cmake_minimum_required(VERSION 3.0)
project(speech-demo)

set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_C_FLAGS "-O2 -fexceptions -fPIC -MMD")
set(CMAKE_C_FLAGS "-O0 -fexceptions -fPIC -MMD -pg -g -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=${CXX11_ABI})
if (CXX11_ABI)
  message("demo support std=c++11")
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

message("DEMO CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("DEMO CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
MESSAGE("DEMO c flag is ${CMAKE_C_FLAGS}")
MESSAGE("DEMO os is ${CMAKE_SYSTEM}")

MESSAGE("build speech-demo enter..")
MESSAGE("PROFECT_SOURCE_DIR " ${PROJECT_SOURCE_DIR})
MESSAGE("CMAKE_SOURCE_DIR " ${CMAKE_SOURCE_DIR})

set(NLS_DEMO_LIST
    ${CMAKE_SOURCE_DIR}/../../build/install/NlsSdk3.X_LINUX/include
    ${CMAKE_SOURCE_DIR}/../../nlsCppSdk/utils
    )
include_directories(${NLS_DEMO_LIST})
link_directories(${CMAKE_SOURCE_DIR}/../../build/install/NlsSdk3.X_LINUX/lib)

set(NLS_DEMO_EXT_FLAG pthread)
# set(NLS_DEMO_EXT_FLAG ${NLS_DEMO_EXT_FLAG} ldap)

# 一句话识别
add_executable(srDemo profile_scan.cpp speechRecognizerDemo.cpp)
target_link_libraries(srDemo
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

add_executable(srMT profile_scan.cpp speechRecognizerMonkeyTest.cpp)
target_link_libraries(srMT
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

# 实时识别
add_executable(stDemo profile_scan.cpp speechTranscriberDemo.cpp)
target_link_libraries(stDemo
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

add_executable(stMT profile_scan.cpp speechTranscriberMonkeyTest.cpp)
target_link_libraries(stMT
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

# 转写
add_executable(syDemo profile_scan.cpp speechSynthesizerDemo.cpp)
target_link_libraries(syDemo
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

add_executable(syMT speechSynthesizerMonkeyTest.cpp)
target_link_libraries(syMT
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

# 流式文本输入语音合成
add_executable(fsDemo profile_scan.cpp flowingSynthesizerDemo.cpp)
target_link_libraries(fsDemo
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

# 对话助手
add_executable(daDemo dialogAssistantDemo.cpp)
target_link_libraries(daDemo
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

# 录音文件识别
add_executable(ftDemo fileTransferDemo.cpp)
target_link_libraries(ftDemo
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

# Token生成
add_executable(gtDemo generateTokenDemo.cpp)
target_link_libraries(gtDemo
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

# log4cpp单元测试
add_executable(logUnitTest log4cppUnitTest.cpp)
target_link_libraries(logUnitTest
    alibabacloud-idst-speech ${NLS_DEMO_EXT_FLAG})

