
include(ExternalProject)
include(GNUInstallDirs)

#后期处理开源，此文件需要由CMakeLists.txt生成
include(${CMAKE_SOURCE_DIR}/config/nls.thirdparty.include.cmake)

#加载编译选项
if (ENABLE_BUILD_IOS)
  include(${CMAKE_SOURCE_DIR}/config/ios.thirdparty.cmake)
elseif (ENABLE_BUILD_LINUX)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(${CMAKE_SOURCE_DIR}/config/linux.thirdparty.debug.cmake)
  else ()
    include(${CMAKE_SOURCE_DIR}/config/linux.thirdparty.release.cmake)
  endif ()
elseif (ENABLE_BUILD_ANDROID)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(${CMAKE_SOURCE_DIR}/config/android.thirdparty.debug.cmake)
  else ()
    include(${CMAKE_SOURCE_DIR}/config/android.thirdparty.release.cmake)
  endif ()
elseif (ENABLE_BUILD_WINDOWS)
  include(${CMAKE_SOURCE_DIR}/config/windows.thirdparty.cmake)
elseif (ENABLE_BUILD_MACOS)
  include(${CMAKE_SOURCE_DIR}/config/macOS.thirdparty.cmake)
endif ()

#编译第三方依赖库
######uuid使用libevent生成######
if (UUID_ENABLE)
  ExternalProject_Add(uuid ${UUID_EXTERNAL_COMPILER_FLAGS})
endif ()

if (OGG_ENABLE)
  ExternalProject_Add(ogg ${OGG_EXTERNAL_COMPILER_FLAGS})
endif ()

if (OPUS_ENABLE)
  ExternalProject_Add(opus ${OPUS_EXTERNAL_COMPILER_FLAGS}
      DEPENDS ogg)
endif ()

if (OPENSSL_ENABLE)
  ExternalProject_Add(openssl ${OPENSSL_EXTERNAL_COMPILER_FLAGS})
endif ()

if (LOG4CPP_ENABLE)
  ExternalProject_Add(log4cpp ${LOG4CPP_EXTERNAL_COMPILER_FLAGS})
endif ()

if (LIBEVENT_ENABLE)
  ExternalProject_Add(libevent ${LIBEVENT_EXTERNAL_COMPILER_FLAGS}
      DEPENDS openssl)
endif ()

if (JSONCPP_ENABLE)
  ExternalProject_Add(jsoncpp ${JSONCPP_EXTERNAL_COMPILER_FLAGS})
endif ()

if (CURL_ENABLE)
  ExternalProject_Add(curl ${CURL_EXTERNAL_COMPILER_FLAGS}
      DEPENDS openssl)
endif ()

include(${CMAKE_SOURCE_DIR}/config/install.thirdparty.cmake)

installThirdpartySdkFiles()

