
message(STATUS "NlsSdk CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "NlsSdk CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "NlsSdk CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "NlsSdk CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "NlsSdk CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "NlsSdk CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "NlsSdk CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "NlsSdk CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "NlsSdk CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "NlsSdk CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "NlsSdk CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "NlsSdk CMAKE_CXX_LINK_FLAGS: ${CMAKE_CXX_LINK_FLAGS}")
message(STATUS "NlsSdk CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "NlsSdk CMAKE_CXX_EXTENSIONS: ${CMAKE_CXX_EXTENSIONS}")
message(STATUS "NlsSdk CMAKE_CXX_STANDARD_REQUIRED: ${CMAKE_CXX_STANDARD_REQUIRED}")
message(STATUS "NlsSdk CXX11_ABI: ${CXX11_ABI}")

#编译依赖库设置
ExternalProject_Get_Property(jsoncpp INSTALL_DIR)
set(jsoncpp_install_dir ${INSTALL_DIR})
message(STATUS "jsoncpp install path: ${jsoncpp_install_dir}")
set(NLS_SDK_HEADER_LIST
    ${NLS_SDK_HEADER_LIST}
    ${jsoncpp_install_dir}/include
    ${jsoncpp_install_dir}/include/jsoncpp
    )

ExternalProject_Get_Property(opus INSTALL_DIR)
set(opus_install_dir ${INSTALL_DIR})
message(STATUS "opus install path: ${opus_install_dir}")
set(NLS_SDK_HEADER_LIST
    ${NLS_SDK_HEADER_LIST}
    ${opus_install_dir}/include
    )

ExternalProject_Get_Property(ogg INSTALL_DIR)
set(ogg_install_dir ${INSTALL_DIR})
message(STATUS "ogg install path: ${ogg_install_dir}")
set(NLS_SDK_HEADER_LIST
    ${NLS_SDK_HEADER_LIST}
    ${ogg_install_dir}/include
    )

ExternalProject_Get_Property(openssl INSTALL_DIR)
set(openssl_install_dir ${INSTALL_DIR})
message(STATUS "openssl install path: ${openssl_install_dir}")
set(NLS_SDK_HEADER_LIST
    ${NLS_SDK_HEADER_LIST}
    ${openssl_install_dir}/include
    )

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  ExternalProject_Get_Property(log4cpp INSTALL_DIR)
  set(log4cpp_install_dir ${INSTALL_DIR})
  message(STATUS "log4cpp install path: ${log4cpp_install_dir}")
  set(NLS_SDK_HEADER_LIST
      ${NLS_SDK_HEADER_LIST}
      ${log4cpp_install_dir}/include
      )
endif ()

ExternalProject_Get_Property(libevent INSTALL_DIR)
set(libevent_install_dir ${INSTALL_DIR})
message(STATUS "libevent install path: ${libevent_install_dir}")
set(NLS_SDK_HEADER_LIST
    ${NLS_SDK_HEADER_LIST}
    ${libevent_install_dir}/include
    )

ExternalProject_Get_Property(uuid INSTALL_DIR)
set(uuid_install_dir ${INSTALL_DIR})
message(STATUS "uuid install path: ${uuid_install_dir}")
set(NLS_SDK_HEADER_LIST
    ${NLS_SDK_HEADER_LIST}
    ${uuid_install_dir}/include
    )

ExternalProject_Get_Property(curl INSTALL_DIR)
set(curl_install_dir ${INSTALL_DIR})
message(STATUS "curl install path: ${curl_install_dir}")
set(NLS_SDK_HEADER_LIST
    ${NLS_SDK_HEADER_LIST}
    ${curl_install_dir}/include
    )


#版本号
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/framework/common/Config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/framework/common/Config.h @ONLY)


#======================================#
#源文件-utils
set(UTILS_INCLUDE_DIR
    ${UTILS_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    )
set(UTILS_SOURCE_DIR
    ${UTILS_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/nlog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/utility.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/text_utils.cpp
    )

#源文件-transport
set(UTILS_INCLUDE_DIR
    ${UTILS_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/transport
    )
set(UTILS_SOURCE_DIR
    ${UTILS_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/connectNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/nlsEventNetWork.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/SSLconnect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/webSocketTcp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/nodeManager.cpp
    )

#源文件-event
set(UTILS_INCLUDE_DIR
    ${UTILS_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/event
    )
set(UTILS_SOURCE_DIR
    ${UTILS_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/event/workThread.cpp
    )

#源文件-encoder
set(UTILS_INCLUDE_DIR
    ${UTILS_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder
    )
set(UTILS_SOURCE_DIR
    ${UTILS_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/nlsEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/oggopusEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/oggopusHeader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/oggopusAudioIn.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/lpc.cpp
    )

#源文件-framework
set(UTILS_INCLUDE_DIR
    ${UTILS_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/common
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/item
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/fss
    )
set(UTILS_SOURCE_DIR
    ${UTILS_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/common/nlsClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/common/nlsClientImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/common/nlsEvent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/item/iNlsRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/item/iNlsRequestParam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/item/iNlsRequestListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerParam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantParam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberParam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerParam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/fss/flowingSynthesizerRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/fss/flowingSynthesizerParam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/fss/flowingSynthesizerListener.cpp
    )

#源文件-token
set(UTILS_INCLUDE_DIR
    ${UTILS_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/token/include
    ${CMAKE_CURRENT_SOURCE_DIR}/token/include/internal
    )
set(UTILS_SOURCE_DIR
    ${UTILS_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/nlsToken.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/CommonClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/CommonResponse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/CommonRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/ServiceRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/NetworkProxy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/HttpMessage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/HttpResponse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/HttpRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/HttpClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/Signer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/HmacSha1Signer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/CoreClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/Credentials.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/SimpleCredentialsProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/CurlHttpClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/ClientConfiguration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/Url.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/Error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/Utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token/src/FileTrans.cpp
    )

#头文件-vipServerClient
if (PRIVATE_CLOUD AND ENABLE_BUILD_LINUX AND ENABLE_BUILD_PRIVATE_SDK)
  set(UTILS_INCLUDE_DIR
      ${UTILS_INCLUDE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/vipServerClient
      )
endif ()


#配置目标源码和头文件
set(NLS_SDK_SOURCE_LIST
    ${UTILS_SOURCE_DIR}
    )
set(NLS_SDK_HEADER_LIST
    ${NLS_SDK_HEADER_LIST}
    ${UTILS_INCLUDE_DIR}
    )
include_directories(${NLS_SDK_HEADER_LIST})

#编译
message(STATUS "nlssdk output name: ${NLS_SDK_OUTPUT_NAME}")
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  if (CXX11_ABI)
    message(STATUS "NlsCppSdk support std=c++11")
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
  endif ()

  set(NLS_SDK_OUTPUT_PATH
      ${CMAKE_SOURCE_DIR}/build/nlsCppSdk/lib${NLS_SDK_OUTPUT_NAME}.a
      )
  add_library(${NLS_SDK_OUTPUT_NAME} STATIC ${NLS_SDK_SOURCE_LIST})

  install(FILES ${NLS_SDK_OUTPUT_PATH}
          DESTINATION
          ${CMAKE_SOURCE_DIR}/build/nlsCppSdk)

elseif (CMAKE_SYSTEM_NAME MATCHES "Android")

  set(NLS_SDK_OUTPUT_PATH
      ${CMAKE_SOURCE_DIR}/build/build_${ANDROID_ABI}/nlsCppSdk/lib${NLS_SDK_OUTPUT_NAME}.a
      )
  add_library(${NLS_SDK_OUTPUT_NAME} STATIC ${NLS_SDK_SOURCE_LIST})

  install(FILES ${NLS_SDK_OUTPUT_PATH}
          DESTINATION
          ${CMAKE_SOURCE_DIR}/build/build_${ANDROID_ABI}/nlsCppSdk)

endif ()
