cmake_minimum_required(VERSION 3.1)

#project
file(STRINGS "${CMAKE_SOURCE_DIR}/version" version)
project(nlsCppSdk2.0 VERSION ${version})
message(STATUS "Project version: ${PROJECT_VERSION}")

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    message("This operation platform is Linux")
    set_property(GLOBAL PROPERTY PLATFORM_LINUX ON)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    message("This operation platform is Windows.")
    set_property(GLOBAL PROPERTY PLATFORM_WINDOWS ON)
endif()

#获取平台配置项
get_property(ENABLE_BUILD_LINUX GLOBAL PROPERTY PLATFORM_LINUX)
get_property(ENABLE_BUILD_WINDOWS GLOBAL PROPERTY PLATFORM_WINDOWS)

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

#配置项解析
function(getConfigValue line)
    #编译动态库
    if(${line} MATCHES "BuildShared=True")
        set_property(GLOBAL PROPERTY BUILD_SHARED_LIBS ON)
        message(STATUS "BuildShared: ON")
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
get_property(ENABLE_BUILD_ASR GLOBAL PROPERTY BUILD_ASR)
get_property(ENABLE_BUILD_REALTIME GLOBAL PROPERTY BUILD_REALTIME)
get_property(ENABLE_BUILD_TTS GLOBAL PROPERTY BUILD_TTS)

#子工程目录
add_subdirectory(common)
add_subdirectory(nlsCppSdk)

if(${ENABLE_BUILD_LINUX})
    add_subdirectory(demo)
endif()
