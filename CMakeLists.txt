cmake_minimum_required(VERSION 3.0)

#project
file(STRINGS "${CMAKE_SOURCE_DIR}/version" version)
project(nlsCppSdk2.0 VERSION ${version})
message(STATUS "Project version: ${PROJECT_VERSION}")

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "ANDROID_PLATFORM_LEVEL: ${ANDROID_PLATFORM_LEVEL}")

set_property(GLOBAL PROPERTY TARGET_OUTPUT_NAME "alibabacloud-idst-speech")

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if(CMAKE_MAC_TYPE MATCHES "MacOS")
        message("This operation platform is MACOS")
        set_property(GLOBAL PROPERTY PLATFORM_MACOS ON)
    else()
        message("This operation platform is APPLE")
        set_property(GLOBAL PROPERTY PLATFORM_IOS ON)
    endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    message("This operation platform is Linux")
    set_property(GLOBAL PROPERTY PLATFORM_LINUX ON)
elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
    message("This operation platform is ANDROID.")
    set_property(GLOBAL PROPERTY PLATFORM_ANDROID ON)
    add_definitions(-D__ANDRIOD__)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    message("This operation platform is Windows.")
    set_property(GLOBAL PROPERTY PLATFORM_WINDOWS ON)
endif()

#获取平台配置项
get_property(NLS_SDK_OUTPUT_NAME GLOBAL PROPERTY TARGET_OUTPUT_NAME)

get_property(ENABLE_BUILD_IOS GLOBAL PROPERTY PLATFORM_IOS)
get_property(ENABLE_BUILD_LINUX GLOBAL PROPERTY PLATFORM_LINUX)
get_property(ENABLE_BUILD_ANDROID GLOBAL PROPERTY PLATFORM_ANDROID)
get_property(ENABLE_BUILD_WINDOWS GLOBAL PROPERTY PLATFORM_WINDOWS)
get_property(ENABLE_BUILD_MACOS GLOBAL PROPERTY PLATFORM_MACOS)

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

#配置项解析
function(getConfigValue line)
    #编译动态库
    if(${line} MATCHES "BuildShared=True")
        set_property(GLOBAL PROPERTY BUILD_SHARED_LIBS ON)
        message(STATUS "BuildShared: ON")
    endif()

    #专有云
    if(${line} MATCHES "BuildVipServer=True")
        set_property(GLOBAL PROPERTY BUILD_PRIVATE_SDK ON)
        message(STATUS "BuildVipServer: ON")
    endif()

    #启用一句话识别
    if(${line} MATCHES "SpeechRecognizer=True")
        set_property(GLOBAL PROPERTY BUILD_ASR ON)
        message(STATUS "BUILD_ASR: ON")
    endif()

    #启用实时转写
    if(${line} MATCHES "SpeechTranscriber=True")
        set_property(GLOBAL PROPERTY BUILD_REALTIME ON)
        message(STATUS "BUILD_REALTIME: ON")
    endif()

    #启用语音合成
    if(${line} MATCHES "SpeechSynthesizer=True")
        set_property(GLOBAL PROPERTY BUILD_TTS ON)
        message(STATUS "BUILD_TTS: ON")
    endif()

    #启用语音助手
    if(${line} MATCHES "SpeechDialogAssistant=True")
        set_property(GLOBAL PROPERTY BUILD_UDS ON)
        message(STATUS "BUILD_UDS: ON")
    endif()
endfunction()

#读取配置文件
function(sdkConfigFunc config_file_path)
    file(READ ${config_file_path} config_content)

    STRING(REGEX REPLACE "\n" ";" config_content_list ${config_content})
    message(STATUS "ContentList: ${config_content_list}")

    foreach(line ${config_content_list})
        #        message("Line: ${line}")
        getConfigValue(${line})
    endforeach()
endfunction()

#获取SDK编译选项
sdkConfigFunc("nlsSdkConfig.conf")

#获取输出类型
get_property(BUILD_TYPE GLOBAL PROPERTY BUILD_LIB_TYPE)

#获取语音功能配置项
get_property(ENABLE_BUILD_SHARED_LIBS GLOBAL PROPERTY BUILD_SHARED_LIBS)
get_property(ENABLE_BUILD_PRIVATE_SDK GLOBAL PROPERTY BUILD_PRIVATE_SDK)
get_property(ENABLE_BUILD_ASR GLOBAL PROPERTY BUILD_ASR)
get_property(ENABLE_BUILD_REALTIME GLOBAL PROPERTY BUILD_REALTIME)
get_property(ENABLE_BUILD_TTS GLOBAL PROPERTY BUILD_TTS)
get_property(ENABLE_BUILD_UDS GLOBAL PROPERTY BUILD_UDS)

#子工程目录
if(ENABLE_BUILD_WINDOWS)
	message(STATUS "Common: Can n't build Common.")
else()
	add_subdirectory(common)
	message(STATUS "Common: Build Common.")
endif()

#if(ENABLE_BUILD_MACOS)
#    add_subdirectory(nlsCppSdk/nlsOpu)
#else()
#    add_subdirectory(nlsCppSdk)
#endif()

if(ENABLE_BUILD_MACOS)
    add_subdirectory(nlsCppSdk/nlsOpu)
endif()

add_subdirectory(nlsCppSdk)

if(ENABLE_BUILD_LINUX)
    add_subdirectory(demo)
#    add_subdirectory(unitTest)
endif()
