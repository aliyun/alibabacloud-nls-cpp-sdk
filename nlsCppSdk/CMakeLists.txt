cmake_minimum_required(VERSION 2.6)
aux_source_directory(. DIR_LIB_SRCS)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/thirdparty)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/thirdparty/jsoncpp)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/transport)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework)
include_directories(${CMAKE_SOURCE_DIR}/nlsCppSdk/framework/feature)

set(THIRDPARTY_FILE_LIST
	thirdparty/jsoncpp/json_reader.cpp
        thirdparty/jsoncpp/json_value.cpp
        thirdparty/jsoncpp/json_writer.cpp
        )

set(FRAMEWORK_FILE_LIST framework/iWebSocketFrameResultConverter.cpp
        framework/nlsClient.cpp
        framework/nlsEvent.cpp
        framework/feature/iNlsRequestParam.cpp
        framework/feature/nlsSessionBase.cpp
        framework/feature/sr/speechRecognizerSession.cpp
        framework/feature/sr/speechRecognizerParam.cpp
        framework/feature/sr/speechRecognizerRequest.cpp
        framework/feature/sr/speechRecognizerListener.cpp
        framework/feature/st/speechTranscriberSession.cpp
        framework/feature/st/speechTranscriberParam.cpp
        framework/feature/st/speechTranscriberRequest.cpp
        framework/feature/st/speechTranscriberListener.cpp
        framework/feature/sy/speechSynthesizerListener.cpp
        framework/feature/sy/speechSynthesizerParam.cpp
        framework/feature/sy/speechSynthesizerRequest.cpp
        framework/feature/sy/speechSynthesizerSession.cpp
	framework/feature/nlsRequestParamInfo.cpp
        )

set(TRANSPORT_FILE_LIST transport/engine/asyncBase.cpp
        transport/engine/thread.cpp
        transport/engine/webSocketAgent.cpp
        transport/socket.cpp
        transport/webSocketTcp.cpp
        )

set(UTIL_FILE_LIST util/dataStruct.cpp
        util/ensureUtility.cpp
        util/exception.cpp
        util/log.cpp
        util/wstr2str.cpp
        util/ztCodec.cpp
        )

add_library(nlsCppSdk SHARED ${LIB_TYPE} ${THIRDPARTY_FILE_LIST} ${FRAMEWORK_FILE_LIST} ${TRANSPORT_FILE_LIST} ${UTIL_FILE_LIST})

install (TARGETS nlsCppSdk DESTINATION lib)

