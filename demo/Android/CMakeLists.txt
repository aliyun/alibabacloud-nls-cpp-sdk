cmake_minimum_required(VERSION 3.0)
project(speech-demo)

set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_C_FLAGS "-O2 -fexceptions -fPIC -MMD")
set(CMAKE_C_FLAGS "-O0 -fexceptions -fPIC -g -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

MESSAGE("c flag is ${CMAKE_C_FLAGS}")
MESSAGE("os is ${CMAKE_SYSTEM}")

MESSAGE("build speech-demo enter..")
MESSAGE("PROFECT_SOURCE_DIR " ${PROJECT_SOURCE_DIR})
MESSAGE("CMAKE_SOURCE_DIR " ${CMAKE_SOURCE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/../../build/install/NlsSdk3.X_${ANDROID_ABI}/include)
link_directories(${CMAKE_SOURCE_DIR}/../../build/install/NlsSdk3.X_${ANDROID_ABI}/lib)

# 一句话识别
add_executable(srDemo speechRecognizerDemo.cpp)
target_link_libraries(srDemo
    alibabacloud-idst-speech_${ANDROID_ABI}
    android log z dl c++_shared)

# 实时识别
add_executable(stDemo speechTranscriberDemo.cpp)
target_link_libraries(stDemo
    alibabacloud-idst-speech_${ANDROID_ABI}
    android log z dl c++_shared)

# 转写
add_executable(syDemo speechSynthesizerDemo.cpp)
target_link_libraries(syDemo
    alibabacloud-idst-speech_${ANDROID_ABI}
    android log z dl c++_shared)

# 对话助手
add_executable(daDemo dialogAssistantDemo.cpp)
target_link_libraries(daDemo
    alibabacloud-idst-speech_${ANDROID_ABI}
    android log z dl c++_shared)



