cmake_minimum_required(VERSION 3.0)
project(speech-demo)

set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_C_FLAGS "-O2 -fexceptions -fPIC -MMD")
set(CMAKE_C_FLAGS "-O0 -fexceptions -fPIC -MMD -pg -g -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

MESSAGE("c flag is ${CMAKE_C_FLAGS}")
MESSAGE("os is ${CMAKE_SYSTEM}")

MESSAGE("build speech-demo enter..")
MESSAGE("PROFECT_SOURCE_DIR " ${PROJECT_SOURCE_DIR})
MESSAGE("CMAKE_SOURCE_DIR " ${CMAKE_SOURCE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/../../build/install/NlsSdk3.X_LINUX/include)
link_directories(${CMAKE_SOURCE_DIR}/../../build/install/NlsSdk3.X_LINUX/lib)

# 一句话识别
add_executable(srDemo profile_scan.cpp speechRecognizerDemo.cpp)
target_link_libraries(srDemo
    alibabacloud-idst-speech
    pthread z dl)

# 实时识别
add_executable(stDemo profile_scan.cpp speechTranscriberDemo.cpp)
target_link_libraries(stDemo
    alibabacloud-idst-speech
    pthread z dl)

# 转写
add_executable(syDemo profile_scan.cpp speechSynthesizerDemo.cpp)
target_link_libraries(syDemo
    alibabacloud-idst-speech
    pthread z dl)

# 对话助手
add_executable(daDemo dialogAssistantDemo.cpp)
target_link_libraries(daDemo
    alibabacloud-idst-speech
    pthread z dl)



