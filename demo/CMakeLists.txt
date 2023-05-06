
cmake_policy(SET CMP0048 NEW)

message(STATUS "Build : ${CMAKE_SOURCE_DIR}")

include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature/sr)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature/st)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature/sy)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature/da)

set(NLS_COMMON_LIB_PATH "${CMAKE_SOURCE_DIR}/lib/linux")
set(NLS_SDK_LIB_PATH "${CMAKE_BINARY_DIR}/lib")

#专有云或公有云
set(SDK_DEMO_DIRECTORY Linux_Demo)

ExternalProject_Get_Property(jsoncpp INSTALL_DIR)
set(jsoncpp_install_dir ${INSTALL_DIR})

set(nlsCommonLib ${NLS_COMMON_LIB_PATH}/libalibabacloud-idst-common.so)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/thirdparty)

if(ENABLE_BUILD_LINUX)
    message(STATUS "Using : libalibabacloud-idst-speech.so")
    set(nlsSdkLib ${NLS_SDK_LIB_PATH}/libalibabacloud-idst-speech.so)

    set(DEMO_THIRD_LIB_LIST
            ${nlsSdkLib}
            ${nlsCommonLib}
            pthread)
endif()

if(ENABLE_BUILD_ASR)
    add_executable(srDemo ${SDK_DEMO_DIRECTORY}/speechRecognizerDemo.cpp)
    set_target_properties(srDemo
            PROPERTIES
            LINKER_LANGUAGE CXX
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo)
    target_link_libraries(srDemo ${DEMO_THIRD_LIB_LIST})

    add_executable(srSyncDemo ${SDK_DEMO_DIRECTORY}/speechRecognizerSyncDemo.cpp)
    set_target_properties(srSyncDemo
            PROPERTIES
            LINKER_LANGUAGE CXX
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo)
    target_link_libraries(srSyncDemo ${DEMO_THIRD_LIB_LIST})

endif()

if(ENABLE_BUILD_REALTIME)
    add_executable(stDemo ${SDK_DEMO_DIRECTORY}/speechTranscriberDemo.cpp)
    set_target_properties(stDemo
            PROPERTIES
            LINKER_LANGUAGE CXX
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo)
    target_link_libraries(stDemo ${DEMO_THIRD_LIB_LIST})

    add_executable(stSyncDemo ${SDK_DEMO_DIRECTORY}/speechTranscriberSyncDemo.cpp)
    set_target_properties(stSyncDemo
            PROPERTIES
            LINKER_LANGUAGE CXX
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo)
    target_link_libraries(stSyncDemo ${DEMO_THIRD_LIB_LIST})

    add_executable(stMultDemo ${SDK_DEMO_DIRECTORY}/speechTranscriberMultDemo.cpp)
    set_target_properties(stMultDemo
            PROPERTIES
            LINKER_LANGUAGE CXX
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo)
    target_link_libraries(stMultDemo ${DEMO_THIRD_LIB_LIST})
endif()

if(ENABLE_BUILD_TTS)
    add_executable(syDemo ${SDK_DEMO_DIRECTORY}/speechSynthesizerDemo.cpp)
    set_target_properties(syDemo
            PROPERTIES
            LINKER_LANGUAGE CXX
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo)
    target_link_libraries(syDemo ${DEMO_THIRD_LIB_LIST})
endif()

if (ENABLE_BUILD_PRIVATE_SDK)
add_executable(vipServerDemo ${SDK_DEMO_DIRECTORY}/vipServerDemo.cpp)
set_target_properties(vipServerDemo
        PROPERTIES
        LINKER_LANGUAGE CXX
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo)
target_link_libraries(vipServerDemo
        ${DEMO_THIRD_LIB_LIST}
        )
endif()
