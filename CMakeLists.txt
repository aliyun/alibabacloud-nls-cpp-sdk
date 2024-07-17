cmake_minimum_required(VERSION 3.0)

#project
file(STRINGS "${CMAKE_SOURCE_DIR}/version" version)
string(REPLACE "-" ";" NLS_VERSION ${version})
list(GET NLS_VERSION 0 SDK_MAIN_VERSION)
list(GET NLS_VERSION 1 SDK_PATCH_VERSION)
project(nlsCppSdk3.x VERSION ${SDK_MAIN_VERSION})
message(STATUS "Project version: ${PROJECT_VERSION}")
add_definitions(-DSDK_VERSION="\\"${SDK_MAIN_VERSION}${SDK_PATCH_VERSION}\\"")

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_LIBRARY_ARCHITECTURE: ${CMAKE_LIBRARY_ARCHITECTURE}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  message (
    STATUS "No CMAKE_BUILD_TYPE selected, defaulting to ${CMAKE_BUILD_TYPE}"
  )
else ()
  message (
    STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}"
  )
endif ()

set_property(GLOBAL PROPERTY TARGET_OUTPUT_NAME "alibabacloud-idst-speech")

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  if (CMAKE_MAC_TYPE MATCHES "MacOS")
    message("This operation platform is MACOS")
    set_property(GLOBAL PROPERTY PLATFORM_MACOS ON)
  else ()
    message("This operation platform is APPLE")
    set_property(GLOBAL PROPERTY PLATFORM_IOS ON)
  endif ()
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
  message("This operation platform is Linux")
  set_property(GLOBAL PROPERTY PLATFORM_LINUX ON)
  add_definitions(-D__LINUX__)
elseif (CMAKE_SYSTEM_NAME MATCHES "Android")
  message(STATUS "ANDROID_PLATFORM_LEVEL: ${ANDROID_PLATFORM_LEVEL}")
  message("This operation platform is ANDROID.")
  set_property(GLOBAL PROPERTY PLATFORM_ANDROID ON)
  add_definitions(-D__ANDRIOD__)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
  message("This operation platform is Windows.")
  set_property(GLOBAL PROPERTY PLATFORM_WINDOWS ON)
endif ()

#获取平台配置项
get_property(NLS_SDK_OUTPUT_NAME GLOBAL PROPERTY TARGET_OUTPUT_NAME)

get_property(ENABLE_BUILD_IOS GLOBAL PROPERTY PLATFORM_IOS)
get_property(ENABLE_BUILD_LINUX GLOBAL PROPERTY PLATFORM_LINUX)
get_property(ENABLE_BUILD_ANDROID GLOBAL PROPERTY PLATFORM_ANDROID)
get_property(ENABLE_BUILD_WINDOWS GLOBAL PROPERTY PLATFORM_WINDOWS)
get_property(ENABLE_BUILD_MACOS GLOBAL PROPERTY PLATFORM_MACOS)

message(STATUS "root CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

message(STATUS "ENABLE_BUILD_IOS: ${ENABLE_BUILD_IOS}")
message(STATUS "ENABLE_BUILD_LINUX: ${ENABLE_BUILD_LINUX}")
message(STATUS "ENABLE_BUILD_ANDROID: ${ENABLE_BUILD_ANDROID}")
message(STATUS "ENABLE_BUILD_WINDOWS: ${ENABLE_BUILD_WINDOWS}")
message(STATUS "ENABLE_BUILD_MACOS: ${ENABLE_BUILD_MACOS}")

#nlsSDK配置文件解析, 功能开关
function(getConfigValue line)
  #专有云
  if (${line} MATCHES "BuildVipServer=True")
    set_property(GLOBAL PROPERTY BUILD_PRIVATE_SDK ON)
    message(STATUS "BuildVipServer: ON")
  endif ()

  #启用一句话识别
  if (${line} MATCHES "SpeechRecognizer=True")
    set_property(GLOBAL PROPERTY BUILD_ASR ON)
    message(STATUS "BUILD_ASR: ON")
  endif ()

  #启用实时转写
  if (${line} MATCHES "SpeechTranscriber=True")
    set_property(GLOBAL PROPERTY BUILD_REALTIME ON)
    message(STATUS "BUILD_REALTIME: ON")
  endif ()

  #启用语音合成
  if (${line} MATCHES "SpeechSynthesizer=True")
    set_property(GLOBAL PROPERTY BUILD_TTS ON)
    message(STATUS "BUILD_TTS: ON")
  endif ()

  #启用语音助手
  if (${line} MATCHES "SpeechDialogAssistant=True")
    set_property(GLOBAL PROPERTY BUILD_UDS ON)
    message(STATUS "BUILD_UDS: ON")
  endif ()
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
sdkConfigFunc("config/nlsSdkConfig.conf")

#设置Git信息
find_package(Git QUIET)
if(GIT_FOUND)
  exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "log --format='[sha1]:%h [author]:%cn [time]:%ci [commit]:%s [branch]:%d' -1"
    OUTPUT_VARIABLE VERSION_SHA1 )
  add_definitions(-DGIT_SHA1="\\"${VERSION_SHA1}\\"")
else()
  add_definitions(-DGIT_SHA1="\\"${SDK_MAIN_VERSION}${SDK_PATCH_VERSION}\\"")
endif()

#获取输出类型
get_property(BUILD_TYPE GLOBAL PROPERTY BUILD_LIB_TYPE)

#获取语音功能配置项
get_property(ENABLE_BUILD_SHARED_LIBS GLOBAL PROPERTY BUILD_SHARED_LIBS)
get_property(ENABLE_BUILD_PRIVATE_SDK GLOBAL PROPERTY BUILD_PRIVATE_SDK)
get_property(ENABLE_BUILD_ASR GLOBAL PROPERTY BUILD_ASR)
get_property(ENABLE_BUILD_REALTIME GLOBAL PROPERTY BUILD_REALTIME)
get_property(ENABLE_BUILD_TTS GLOBAL PROPERTY BUILD_TTS)
get_property(ENABLE_BUILD_UDS GLOBAL PROPERTY BUILD_UDS)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -fPIC -pg -g -ggdb")
  elseif (CMAKE_SYSTEM_NAME MATCHES "Android")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -fPIC -g -ggdb")
  endif ()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  add_definitions(-DENABLE_NLS_DEBUG)
else ()
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -fPIC")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif ()
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,-Bsymbolic")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Bsymbolic")

#default is public_cloud sdk. PRIVATE_CLOUD setting from script.
if (NOT DEFINED PRIVATE_CLOUD)
  message(STATUS "Use default PRIVATE_CLOUD 0")
  set(PRIVATE_CLOUD 0)
endif ()

#default forbidden C++11, -D_GLIBCXX_USE_CXX11_ABI=0
if (NOT DEFINED CXX11_ABI)
  message(STATUS "Use default CXX11_ABI 0")
  set(CXX11_ABI 0)
endif ()
if (CXX11_ABI)
  message(STATUS "NlsSdk support std=c++11")
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
else ()
  add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif ()

#启用OPUS(OPUS模式默认已启动)
add_definitions(-DENABLE_OGGOPUS)
#启用高效模式, 降低CPU占用率
add_definitions(-DENABLE_HIGH_EFFICIENCY)
#启动请求过程记录
add_definitions(-DENABLE_REQUEST_RECORDING)
#增加DNS IP缓存机制, 以降低DNS资源占用
add_definitions(-DENABLE_DNS_IP_CACHE)
#增加运行中断自动重连的功能
add_definitions(-DENABLE_CONTINUED)

#添加VipServer相关配置
message(STATUS "PRIVATE_CLOUD: ${PRIVATE_CLOUD}")
message(STATUS "ENABLE_BUILD_PRIVATE_SDK: ${ENABLE_BUILD_PRIVATE_SDK}")
if (PRIVATE_CLOUD AND ENABLE_BUILD_LINUX AND ENABLE_BUILD_PRIVATE_SDK)
  message(STATUS "Enable Private Cloud.")
  add_definitions(-DENABLE_VIPSERVER)
else ()
  message(STATUS "Only Enable Public Cloud.")
endif ()

#编译nlsCppSdk依赖的第三方库
if (ENABLE_BUILD_WINDOWS)
  message(STATUS "Cannot build thirdparty.")
else ()
  add_subdirectory(thirdparty)
  message(STATUS "Build thirdparty...")
endif ()

#编译nlsCppSdk主体
add_subdirectory(nlsCppSdk)

#编译测试工程
# build_linux.sh中进行编译

