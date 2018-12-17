
#if(${ENABLE_BUILD_LINUX} OR ${ENABLE_BUILD_ANDROID})
if(${ENABLE_BUILD_LINUX})
    cmake_policy(SET CMP0048 NEW)

    ExternalProject_Get_Property(openssl INSTALL_DIR)
    set(openssl_install_dir ${INSTALL_DIR})

    ExternalProject_Get_Property(uuid INSTALL_DIR)
    set(uuid_install_dir ${INSTALL_DIR})

    ExternalProject_Get_Property(jsoncpp INSTALL_DIR)
    set(jsoncpp_install_dir ${INSTALL_DIR})

    include_directories(${openssl_install_dir}/include)
    include_directories(${uuid_install_dir}/include)
    include_directories(${jsoncpp_install_dir}/include)
endif()

message(STATUS "Build : ${CMAKE_SOURCE_DIR}")

#include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/include)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature/sr)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature/st)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature/sy)

set(SDK_DEMO_DIRECTORY PublicCloud)

set(nlsTokenLib
        ${CMAKE_SOURCE_DIR}/lib/linux/libnlsCommonSdk.so
        ${CMAKE_SOURCE_DIR}/lib/linux/libcurl.so)

include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/thirdparty)

if(${ENABLE_BUILD_LINUX})
    if(${ENABLE_BUILD_SHARED_LIBS})
        message(STATUS "Using : libnlsCppSdk.so")
        set(nlsSdkLib ${CMAKE_BINARY_DIR}/lib/libnlsCppSdk.so)
    else()
        message(STATUS "Using : libnlsCppSdk.a")
        set(nlsSdkLib ${CMAKE_BINARY_DIR}/lib/libnlsCppSdk.a)
    endif()

    set(DEMO_THIRD_LIB_LIST
            ${nlsSdkLib}
            ${openssl_install_dir}/lib/libssl.so
            ${openssl_install_dir}/lib/libcrypto.so
            ${uuid_install_dir}/lib/libuuid.so
            pthread
            ${jsoncpp_install_dir}/lib/libjsoncpp.so
            ${nlsTokenLib})
endif()

if(${ENABLE_BUILD_ASR})
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

if(${ENABLE_BUILD_REALTIME})
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
endif()

if(${ENABLE_BUILD_TTS})
    add_executable(syDemo ${SDK_DEMO_DIRECTORY}/speechSynthesizerDemo.cpp)
    set_target_properties(syDemo
            PROPERTIES
            LINKER_LANGUAGE CXX
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo)
    target_link_libraries(syDemo ${DEMO_THIRD_LIB_LIST})
endif()
