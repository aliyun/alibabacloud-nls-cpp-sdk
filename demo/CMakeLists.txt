cmake_minimum_required(VERSION 2.6)
project(speech-demo)

set(SDK_HEADER_DIR ${HEADER_DIR}/nlsCppSdk)

include_directories(${SDK_HEADER_DIR})
include_directories(${SDK_HEADER_DIR}/thirdparty)
include_directories(${SDK_HEADER_DIR}/transport)
include_directories(${SDK_HEADER_DIR}/framework)
include_directories(${SDK_HEADER_DIR}/framework/feature)
include_directories(${SDK_HEADER_DIR}/framework/feature/sr)
include_directories(${SDK_HEADER_DIR}/framework/feature/st)
include_directories(${SDK_HEADER_DIR}/framework/feature/sy)

link_directories(${HEADER_DIR}/lib/linux)

add_executable(srDemo speechRecognizerDemo.cpp)
target_link_libraries(srDemo nlsCppSdk ssl crypto opus uuid pthread)

add_executable(stDemo speechTranscriberDemo.cpp)
target_link_libraries(stDemo nlsCppSdk ssl crypto opus uuid pthread)

add_executable(syDemo speechSynthesizerDemo.cpp)
target_link_libraries(syDemo nlsCppSdk ssl crypto opus uuid pthread)
