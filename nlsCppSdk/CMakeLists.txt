
#加载编译选项
if(ENABLE_BUILD_IOS)
    include(ios.nlsCppSdk.cmake)
elseif(ENABLE_BUILD_LINUX)
    include(linux.nlsCppSdk.cmake)
elseif(ENABLE_BUILD_ANDROID)
    include(android.nlsCppSdk.cmake)
elseif(ENABLE_BUILD_WINDOWS)
    include(windows.nlsCppSdk.cmake)
endif()

message(STATUS "NlsSdk CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "NlsSdk CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "NlsSdk CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "NlsSdk CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "NlsSdk CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "NlsSdk CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "NlsSdk CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "NlsSdk CMAKE_CXX_LINK_FLAGS: ${CMAKE_CXX_LINK_FLAGS}")

#配置库类型
if(ENABLE_BUILD_SHARED_LIBS)
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif()
message(STATUS "Lib Type: ${LIB_TYPE}")

#版本号
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/framework/Config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/framework/Config.h @ONLY)

#根据配置过滤nlsClient.*s
function(filterNlsClient source_file target_file)
    #    file(REMOVE ${target_file})

    file(READ ${source_file} source_file_content)

    #    message(STATUS "CT: ${source_file_content}")
    if (ENABLE_BUILD_PRIVATE_SDK)
        message(STATUS "ENABLE_BUILD_VIPSERVER ON")
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP1_BEGIN:.?\n" "" middle_content ${source_file_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP1_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP2_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP2_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP3_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP3_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP4_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP4_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP5_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP5_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_VIPSERVER_HEADER1_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_HEADER1_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_VIPSERVER_HEADER2_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_HEADER2_END\n" "" middle_content ${middle_content})
    else()
        message(STATUS "ENABLE_BUILD_VIPSERVER OFF")
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP1_BEGIN:.*_ENABLE_VIPSERVER_CPP1_END.?\n" "" middle_content ${source_file_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP2_BEGIN:.*_ENABLE_VIPSERVER_CPP2_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP3_BEGIN:.*_ENABLE_VIPSERVER_CPP3_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP4_BEGIN:.*_ENABLE_VIPSERVER_CPP4_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_CPP5_BEGIN:.*_ENABLE_VIPSERVER_CPP5_END.?\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_VIPSERVER_HEADER1_BEGIN:.*_ENABLE_VIPSERVER_HEADER1_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_VIPSERVER_HEADER2_BEGIN:.*_ENABLE_VIPSERVER_HEADER2_END.?\n" "" middle_content ${middle_content})
    endif()

    if (ENABLE_BUILD_ASR)
        string(REGEX REPLACE "_ENABLE_ASR_CLASS_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_ASR_CLASS_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_ASR_BODY_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_ASR_BODY_END\n" "" middle_content ${middle_content})
    else()
        string(REGEX REPLACE "_ENABLE_ASR_CLASS_BEGIN:.*_ENABLE_ASR_CLASS_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_ASR_BODY_BEGIN:.*_ENABLE_ASR_BODY_END.?\n" "" middle_content ${middle_content})
    endif()

    if (ENABLE_BUILD_REALTIME)
        string(REGEX REPLACE "_ENABLE_REALTIME_CLASS_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_REALTIME_CLASS_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_REALTIME_BODY_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_REALTIME_BODY_END\n" "" middle_content ${middle_content})
    else()
        string(REGEX REPLACE "_ENABLE_REALTIME_CLASS_BEGIN:.*_ENABLE_REALTIME_CLASS_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_REALTIME_BODY_BEGIN:.*_ENABLE_REALTIME_BODY_END.?\n" "" middle_content ${middle_content})
    endif()

    if (ENABLE_BUILD_TTS)
        string(REGEX REPLACE "_ENABLE_TTS_CLASS_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_TTS_CLASS_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_TTS_BODY_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_TTS_BODY_END\n" "" middle_content ${middle_content})
    else()
        string(REGEX REPLACE "_ENABLE_TTS_CLASS_BEGIN:.*_ENABLE_TTS_CLASS_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_TTS_BODY_BEGIN:.*_ENABLE_TTS_BODY_END.?\n" "" middle_content ${middle_content})
    endif()

    if (ENABLE_BUILD_UDS)
        string(REGEX REPLACE "_ENABLE_UDS_CLASS_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_UDS_CLASS_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_UDS_BODY_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_UDS_BODY_END\n" "" middle_content ${middle_content})
    else()
        string(REGEX REPLACE "_ENABLE_UDS_CLASS_BEGIN:.*_ENABLE_UDS_CLASS_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_UDS_BODY_BEGIN:.*_ENABLE_UDS_BODY_END.?\n" "" middle_content ${middle_content})
    endif()

    string(REGEX REPLACE "##" "\\\;" middle_content ${middle_content})

    file(WRITE ${target_file} ${middle_content})
endfunction()

filterNlsClient(${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.cpp.in
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.cpp)
filterNlsClient(${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.h.in
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.h)

#源文件-util
set(UTIL_HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/util/error.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/utility.h
        )

set(TRANSPORT_HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/webSocketTcp.h
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/connectNode.h
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/eventNetWork.h
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/workThread.h
        )

set(FRAMEWORK_HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsGlobal.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/Config.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/webSocketFrameHandleBase.h
#        ${CMAKE_CURRENT_SOURCE_DIR}/framework/iWebSocketFrameResultConverter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsEvent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequestParam.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequestListener.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/nlsRequestParamInfo.h
        )

set(UTIL_CPP_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/util/log.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/util/utility.cpp
        )

set(TRANSPORT_CPP_FILE_LIST
#        ${CMAKE_CURRENT_SOURCE_DIR}/transport/baseSocket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/webSocketTcp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/connectNode.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/eventNetWork.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/workThread.cpp
        )

if(ENABLE_BUILD_IOS)
    set(TRANSPORT_HEADER_FILE_LIST
            ${TRANSPORT_HEADER_FILE_LIST}
            ${CMAKE_CURRENT_SOURCE_DIR}/transport/iosSsl.h
            )
    set(TRANSPORT_CPP_FILE_LIST
            ${TRANSPORT_CPP_FILE_LIST}
            ${CMAKE_CURRENT_SOURCE_DIR}/transport/iosSsl.cpp
            )
else()
    set(TRANSPORT_HEADER_FILE_LIST
            ${TRANSPORT_HEADER_FILE_LIST}
            ${CMAKE_CURRENT_SOURCE_DIR}/transport/commonSsl.h
            )
    set(TRANSPORT_CPP_FILE_LIST
            ${TRANSPORT_CPP_FILE_LIST}
            ${CMAKE_CURRENT_SOURCE_DIR}/transport/commonSsl.cpp
            )
endif()

set(FRAMEWORK_CPP_FILE_LIST
#        ${CMAKE_CURRENT_SOURCE_DIR}/framework/iWebSocketFrameResultConverter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsEvent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequestParam.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequestListener.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/nlsRequestParamInfo.cpp
        )

#源文件-功能
if(ENABLE_BUILD_ASR)
    message(STATUS "Build Asr.....")

    ADD_DEFINITIONS(-D_ENABLE_ASR_)

    set(SPEECHRECOGNIZER_HEADER_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerListener.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerRequest.h
#            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerSyncRequest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerParam.h
            )

    set(SPEECHRECOGNIZER_CPP_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerParam.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerRequest.cpp
#            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerSyncRequest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerListener.cpp
            )
endif()

if(ENABLE_BUILD_TTS)
    message(STATUS "Build Tts.....")

    ADD_DEFINITIONS(-D_ENABLE_TTS_)
    set(SPEECHSYNTHESIZER_HEADER_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerListener.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerRequest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerParam.h
            )

    set(SPEECHSYNTHESIZER_CPP_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerListener.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerParam.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerRequest.cpp
            )
endif()

if(ENABLE_BUILD_REALTIME)
    message(STATUS "Build Realtime.....")

    ADD_DEFINITIONS(-D_ENABLE_REALTIME_)
    set(SPEECHTRANSCRIBER_HEADER_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberListener.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberRequest.h
#            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberSyncRequest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberParam.h
            )

    set(SPEECHTRANSCRIBER_CPP_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberParam.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberRequest.cpp
#            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberSyncRequest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberListener.cpp
            )
endif()

if(ENABLE_BUILD_UDS)
    message(STATUS "Build Uds.....")

    ADD_DEFINITIONS(-D_ENABLE_UDS_)
    set(DIALOGASSISTENT_CPP_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantListener.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantParam.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantRequest.cpp
            )

    set(DIALOGASSISTENT_HEADER_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantListener.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantParam.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantRequest.h
            )
endif()


#配置目标源码
set(NLS_SDK_BASE_FILE_LIST
        ${UTIL_HEADER_FILE_LIST}
        ${FRAMEWORK_HEADER_FILE_LIST}
        ${TRANSPORT_HEADER_FILE_LIST}
        ${DIALOGASSISTENT_HEADER_FILE_LIST}
        ${SPEECHSYNTHESIZER_HEADER_FILE_LIST}
        ${SPEECHRECOGNIZER_HEADER_FILE_LIST}
        ${SPEECHTRANSCRIBER_HEADER_FILE_LIST}
        ${UTIL_CPP_FILE_LIST}
        ${FRAMEWORK_CPP_FILE_LIST}
        ${TRANSPORT_CPP_FILE_LIST}
        ${DIALOGASSISTENT_CPP_FILE_LIST}
        ${SPEECHSYNTHESIZER_CPP_FILE_LIST}
        ${SPEECHRECOGNIZER_CPP_FILE_LIST}
        ${SPEECHTRANSCRIBER_CPP_FILE_LIST}
        ${VIPSERVER_HEADER_FILE_LIST}
        )

ADD_DEFINITIONS(-D_NLS_SDK_SHARED_)

if(ENABLE_BUILD_IOS)
    add_library(${NLS_SDK_OUTPUT_NAME} ${LIB_TYPE} ${NLS_SDK_BASE_FILE_LIST} ${IOS_SSL_FILE_LIST} ${NLS_OPU_FILE_LIST})

    if(ENABLE_BUILD_SHARED_LIBS)
        set(PUBLIC_HEADERS
                ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsGlobal.h
                ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.h
                ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsEvent.h
                ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequest.h
                ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerRequest.h
#                ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerSyncRequest.h
                ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberRequest.h
 #               ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberSyncRequest.h
                ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerRequest.h
                ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantRequest.h)

        set_property(TARGET ${NLS_SDK_OUTPUT_NAME} PROPERTY FRAMEWORK ON)
        set_property(TARGET ${NLS_SDK_OUTPUT_NAME} PROPERTY PUBLIC_HEADER ${PUBLIC_HEADERS})
        set_property(TARGET ${NLS_SDK_OUTPUT_NAME} PROPERTY MACOSX_FRAMEWORK_IDENTIFIER com.skylight.SKYObject)
        set_property(TARGET ${NLS_SDK_OUTPUT_NAME} PROPERTY MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_SOURCE_DIR}/toolchain/Info.plist)
    endif()
elseif(ENABLE_BUILD_LINUX)
    set(NLS_OPU_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.c
            ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.h
            )

    add_library(${NLS_SDK_OUTPUT_NAME}
            SHARED
            ${NLS_OPU_FILE_LIST}
            ${NLS_SDK_BASE_FILE_LIST}
            )
    add_library(${NLS_SDK_OUTPUT_NAME}-a
            STATIC
            ${NLS_OPU_FILE_LIST}
            ${NLS_SDK_BASE_FILE_LIST}
            )
elseif(ENABLE_BUILD_ANDROID)
    add_library(${NLS_SDK_OUTPUT_NAME}
            SHARED
            ${NLS_SDK_BASE_FILE_LIST}
            ${NLS_OPU_FILE_LIST}
            ${JNI_FILE_LIST}
            )

#    add_library(${NLS_SDK_OUTPUT_NAME}-a
#            STATIC
#            ${NLS_SDK_BASE_FILE_LIST}
#            ${NLS_OPU_FILE_LIST}
#            ${JNI_FILE_LIST}
#            )

    add_custom_command(TARGET ${NLS_SDK_OUTPUT_NAME}
            POST_BUILD
            COMMAND $ENV{STRIP} ${SDK_BASE_OUTPUT_PATH}/lib${NLS_SDK_OUTPUT_NAME}.so
            )
elseif(ENABLE_BUILD_WINDOWS)
    set(NLS_OPU_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.c
			${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.h
            )

    add_library(${NLS_SDK_OUTPUT_NAME}
            SHARED
            ${NLS_OPU_FILE_LIST}
            ${NLS_SDK_BASE_FILE_LIST}
            )			
endif()

#sdk输出路径
if(NOT ENABLE_BUILD_WINDOWS) 
set_target_properties(${NLS_SDK_OUTPUT_NAME}
        PROPERTIES
        LINKER_LANGUAGE CXX
        ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
        LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
        OUTPUT_NAME ${NLS_SDK_OUTPUT_NAME})
endif()
		
#if(ENABLE_BUILD_LINUX OR ENABLE_BUILD_ANDROID)
if(ENABLE_BUILD_LINUX)
    set_target_properties(${NLS_SDK_OUTPUT_NAME}-a
            PROPERTIES
            LINKER_LANGUAGE CXX
            ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
            LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
            OUTPUT_NAME ${NLS_SDK_OUTPUT_NAME})
endif()

#编译依赖库设置
if(ENABLE_BUILD_LINUX)
    target_include_directories(${NLS_SDK_OUTPUT_NAME}
            PRIVATE
            ${LINUX_HEDER_FILE_LIST}
            )
    target_link_libraries(${NLS_SDK_OUTPUT_NAME} ${LINUX_LIB_FILE_LIST})

    target_include_directories(${NLS_SDK_OUTPUT_NAME}-a
            PRIVATE
            ${LINUX_HEDER_FILE_LIST}
            )
    target_link_libraries(${NLS_SDK_OUTPUT_NAME}-a ${LINUX_LIB_FILE_LIST})

    message(STATUS "LINUX_LIB_FILE_LIST: ${LINUX_LIB_FILE_LIST}")

elseif(ENABLE_BUILD_WINDOWS)
	set(WINDOWS_HEDER_FILE_LIST	
		${WINDOWS_HEDER_FILE_LIST}
		${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.h)
    
	target_include_directories(${NLS_SDK_OUTPUT_NAME}
            PRIVATE
            ${WINDOWS_HEDER_FILE_LIST}
            )
    target_link_libraries(${NLS_SDK_OUTPUT_NAME} ${WINDOWS_LIB_FILE_LIST})
elseif(ENABLE_BUILD_ANDROID)
    target_include_directories(${NLS_SDK_OUTPUT_NAME}
            PRIVATE
            ${ANDROID_HEDER_FILE_LIST}
            )
    target_link_libraries(${NLS_SDK_OUTPUT_NAME} ${ANDROID_LIB_A_FILE_LIST} c)

#    target_include_directories(${NLS_SDK_OUTPUT_NAME}-a
#            PRIVATE
#            ${ANDROID_HEDER_FILE_LIST}
#            )
#    target_link_libraries(${NLS_SDK_OUTPUT_NAME}-a ${ANDROID_LIB_A_FILE_LIST})
elseif(ENABLE_BUILD_IOS)
    target_include_directories(${NLS_SDK_OUTPUT_NAME}
            PRIVATE
            ${IOS_HEDER_FILE_LIST}
            )
    target_link_libraries(${NLS_SDK_OUTPUT_NAME} ${IOS_LIB_FILE_LIST})
endif()

#install
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

#install nls header files
function(installHeaderFiles)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsGlobal.h
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.h
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsEvent.h
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequest.h
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

    if(ENABLE_BUILD_ASR)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    if(ENABLE_BUILD_REALTIME)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    if(ENABLE_BUILD_TTS)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    if(ENABLE_BUILD_UDS)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/da/dialogAssistantRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    endif()

endfunction()

#install version
function(installCommentsFiles)
    install(FILES ${CMAKE_SOURCE_DIR}/version DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES ${CMAKE_SOURCE_DIR}/release.log DESTINATION ${CMAKE_INSTALL_PREFIX})
endfunction()

#install all
if(ENABLE_BUILD_IOS)
    installNlsIsoSdkFiles()
elseif(ENABLE_BUILD_LINUX)
    installNlsLinuxSdkFiles()
elseif(ENABLE_BUILD_ANDROID)
    installNlsAndroidSdkFiles()
elseif(ENABLE_BUILD_WINDOWS)
    installNlsWindowsSdkFiles()
endif()
