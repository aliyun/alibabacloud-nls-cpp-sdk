cmake_minimum_required(VERSION 3.1)
project(alibabanls-speech-demo)

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/demo/include)

set(sdk_lib_path ${CMAKE_SOURCE_DIR}/lib/$(VisualStudioVersion))
set(token_lib_path ${CMAKE_SOURCE_DIR}/demo/lib/$(VisualStudioVersion))

link_directories(${sdk_lib_path}/$(PlatformTarget))
link_directories(${token_lib_path}/$(PlatformTarget))

add_executable(srDemo demo/speechRecognizerDemo.cpp)
target_link_libraries(srDemo nlsCommonSdk alibabacloud-idst-speech)

add_executable(srSyncDemo demo/speechRecognizerSyncDemo.cpp)
target_link_libraries(srSyncDemo nlsCommonSdk alibabacloud-idst-speech)

add_executable(syDemo demo/speechSynthesizerDemo.cpp)
target_link_libraries(syDemo nlsCommonSdk alibabacloud-idst-speech)

add_executable(stDemo demo/speechTranscriberDemo.cpp)
target_link_libraries(stDemo nlsCommonSdk alibabacloud-idst-speech)

add_executable(stSyncDemo demo/speechTranscriberSyncDemo.cpp)
target_link_libraries(stSyncDemo nlsCommonSdk alibabacloud-idst-speech)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/demo)


