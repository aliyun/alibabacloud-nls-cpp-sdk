
if(${ENABLE_BUILD_LINUX})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fPIC -fvisibility=hidden -fvisibility-inlines-hidden")

    #forbidden C++11
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

    set (CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,-Bsymbolic")
    set (CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Bsymbolic")
endif()

message(STATUS "NlsSdk CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "NlsSdk CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "NlsSdk CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "NlsSdk CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "NlsSdk CMAKE_CXX_LINK_FLAGS: ${CMAKE_CXX_LINK_FLAGS}")

#版本号
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/framework/Config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/framework/Config.h @ONLY)

#根据配置过滤nlsClient.*s
function(filterNlsClient source_file target_file)
    #    file(REMOVE ${target_file})

    file(READ ${source_file} source_file_content)

    #    message(STATUS "CT: ${source_file_content}")

    if (${ENABLE_BUILD_ASR})
        string(REGEX REPLACE "_ENABLE_ASR_CLASS_BEGIN:.?\n" "" middle_content ${source_file_content})
        string(REGEX REPLACE "_ENABLE_ASR_CLASS_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_ASR_BODY_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_ASR_BODY_END\n" "" middle_content ${middle_content})
    else()
        string(REGEX REPLACE "_ENABLE_ASR_CLASS_BEGIN:.*_ENABLE_ASR_CLASS_END.?\n" "" middle_content ${source_file_content})
        string(REGEX REPLACE "_ENABLE_ASR_BODY_BEGIN:.*_ENABLE_ASR_BODY_END.?\n" "" middle_content ${middle_content})
    endif()

    if (${ENABLE_BUILD_REALTIME})
        string(REGEX REPLACE "_ENABLE_REALTIME_CLASS_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_REALTIME_CLASS_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_REALTIME_BODY_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_REALTIME_BODY_END\n" "" middle_content ${middle_content})
    else()
        string(REGEX REPLACE "_ENABLE_REALTIME_CLASS_BEGIN:.*_ENABLE_REALTIME_CLASS_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_REALTIME_BODY_BEGIN:.*_ENABLE_REALTIME_BODY_END.?\n" "" middle_content ${middle_content})
    endif()

    if (${ENABLE_BUILD_TTS})
        string(REGEX REPLACE "_ENABLE_TTS_CLASS_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_TTS_CLASS_END\n" "" middle_content ${middle_content})

        string(REGEX REPLACE "_ENABLE_TTS_BODY_BEGIN:.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_TTS_BODY_END\n" "" middle_content ${middle_content})
    else()
        string(REGEX REPLACE "_ENABLE_TTS_CLASS_BEGIN:.*_ENABLE_TTS_CLASS_END.?\n" "" middle_content ${middle_content})
        string(REGEX REPLACE "_ENABLE_TTS_BODY_BEGIN:.*_ENABLE_TTS_BODY_END.?\n" "" middle_content ${middle_content})
    endif()

    string(REGEX REPLACE "##" "\\\;" middle_content ${middle_content})

    #    message(STATUS "CCCT: ${middle_content}")

    file(WRITE ${target_file} ${middle_content})
endfunction()

filterNlsClient(${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.cpp.in
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.cpp)
filterNlsClient(${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.h.in
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.h)

#源文件
set(UTIL_HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/util/dataStruct.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/errorHandlingUtility.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/targetOs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/ensureUtility.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/exception.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/smartHandle.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util/wstr2str.h
        )

set(TRANSPORT_HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/socket.h
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/webSocketTcp.h
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/engine/asyncBase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/engine/thread.h
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/engine/webSocketAgent.h
        )

set(FRAMEWORK_HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsGlobal.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/Config.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/webSocketFrameHandleBase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/iWebSocketFrameResultConverter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsEvent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequestParam.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/nlsSessionBase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequestListener.h
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/nlsRequestParamInfo.h
        )

set(UTIL_CPP_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/util/dataStruct.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/util/ensureUtility.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/util/exception.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/util/log.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/util/wstr2str.cpp
        )

set(TRANSPORT_CPP_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/engine/asyncBase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/engine/thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/engine/webSocketAgent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transport/webSocketTcp.cpp
        )

set(FRAMEWORK_CPP_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/iWebSocketFrameResultConverter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsClient.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/nlsEvent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequestParam.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/nlsSessionBase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/iNlsRequestListener.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/nlsRequestParamInfo.cpp
        )

if(${ENABLE_BUILD_ASR})
    message(STATUS "Build Asr.....")

    ADD_DEFINITIONS(-D_ENABLE_ASR_)

    set(SPEECHRECOGNIZER_HEADER_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerListener.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerRequest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerSyncRequest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerParam.h
            )

    set(SPEECHRECOGNIZER_CPP_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerParam.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerRequest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerSyncRequest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerListener.cpp
            )
endif()

if(${ENABLE_BUILD_TTS})
    message(STATUS "Build Tts.....")

    ADD_DEFINITIONS(-D_ENABLE_TTS_)
    set(SPEECHSYNTHESIZER_HEADER_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerListener.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerRequest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerParam.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerSession.h
            )

    set(SPEECHSYNTHESIZER_CPP_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerListener.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerParam.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerRequest.cpp
            )
endif()

if(${ENABLE_BUILD_REALTIME})
    message(STATUS "Build Realtime.....")

    ADD_DEFINITIONS(-D_ENABLE_REALTIME_)
    set(SPEECHTRANSCRIBER_HEADER_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberListener.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberRequest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberSyncRequest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberParam.h
            )

    set(SPEECHTRANSCRIBER_CPP_FILE_LIST
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberParam.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberRequest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberSyncRequest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberListener.cpp
            )
endif()

#配置库类型
if(${ENABLE_BUILD_SHARED_LIBS})
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif()

message(STATUS "Lib Type: ${LIB_TYPE}")

#配置目标源码
set(NLS_SDK_BASE_FILE_LIST
        ${UTIL_HEADER_FILE_LIST}
        ${FRAMEWORK_HEADER_FILE_LIST}
        ${TRANSPORT_HEADER_FILE_LIST}
        ${SPEECHSYNTHESIZER_HEADER_FILE_LIST}
        ${SPEECHRECOGNIZER_HEADER_FILE_LIST}
        ${SPEECHTRANSCRIBER_HEADER_FILE_LIST}
        ${UTIL_CPP_FILE_LIST}
        ${FRAMEWORK_CPP_FILE_LIST}
        ${TRANSPORT_CPP_FILE_LIST}
        ${SPEECHSYNTHESIZER_CPP_FILE_LIST}
        ${SPEECHRECOGNIZER_CPP_FILE_LIST}
        ${SPEECHTRANSCRIBER_CPP_FILE_LIST})

if(${ENABLE_BUILD_LINUX})
    add_library(nlsCppSdk ${LIB_TYPE} ${NLS_SDK_BASE_FILE_LIST})

    ADD_DEFINITIONS(-D_NLS_SDK_SHARED_)
    set(SDK_BASE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

elseif(${ENABLE_BUILD_ANDROID})
    add_library(nlsCppSdk ${LIB_TYPE} ${NLS_SDK_BASE_FILE_LIST})
elseif(${ENABLE_BUILD_WINDOWS})
    add_library(nlsCppSdk ${LIB_TYPE} ${NLS_SDK_BASE_FILE_LIST})
endif()

#sdk输出路径
set_target_properties(nlsCppSdk
        PROPERTIES
        LINKER_LANGUAGE CXX
        ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
        LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
        OUTPUT_NAME nlsCppSdk)

message(STATUS "Sdk output path: SDK_BASE_OUTPUT_PATH")

#编译依赖库设置
if(${ENABLE_BUILD_LINUX})
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/build_linux_sdk/install/CppSdk2.X)

    ExternalProject_Get_Property(jsoncpp INSTALL_DIR)
    set(jsoncpp_install_dir ${INSTALL_DIR})
    message(STATUS "jsoncpp install path: ${jsoncpp_install_dir}")

    ExternalProject_Get_Property(openssl INSTALL_DIR)
    set(openssl_install_dir ${INSTALL_DIR})
    message(STATUS "openssl install path: ${openssl_install_dir}")

    ExternalProject_Get_Property(uuid INSTALL_DIR)
    set(uuid_install_dir ${INSTALL_DIR})
    message(STATUS "uuid install path: ${uuid_install_dir}")

    target_include_directories(nlsCppSdk PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/util
            ${CMAKE_CURRENT_SOURCE_DIR}/transport
            ${CMAKE_CURRENT_SOURCE_DIR}/framework
            ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature
            ${jsoncpp_install_dir}/include/jsoncpp
            ${openssl_install_dir}/include
            ${uuid_install_dir}/include)

    target_link_libraries(nlsCppSdk
            ${openssl_install_dir}/lib/libssl.so
            ${openssl_install_dir}/lib/libcrypto.so
            ${uuid_install_dir}/lib/libuuid.so
            pthread
            ${jsoncpp_install_dir}/lib/libjsoncpp.so)
elseif(${ENABLE_BUILD_WINDOWS})
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/build_windows_sdk/install/CppSdk2.x)
endif()

#安装设置
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

#头文件
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

    if(${ENABLE_BUILD_ASR})
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sr/speechRecognizerSyncRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    if(${ENABLE_BUILD_REALTIME})
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/st/speechTranscriberSyncRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    if(${ENABLE_BUILD_TTS})
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature/sy/speechSynthesizerRequest.h
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    endif()
endfunction()

#version等文件
function(installCommentsFiles)
    install(FILES ${CMAKE_SOURCE_DIR}/version DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES ${CMAKE_SOURCE_DIR}/release.log DESTINATION ${CMAKE_INSTALL_PREFIX})
#    install(FILES ${CMAKE_SOURCE_DIR}/readme.md DESTINATION ${CMAKE_INSTALL_PREFIX})
endfunction()

#基础依赖库文件
function(installCommonFiles)
    #openssl
    install(DIRECTORY
            ${openssl_install_dir}/lib/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_PLATFORM}/${CMAKE_INSTALL_LIBDIR}
            FILES_MATCHING PATTERN "lib*.so*"
            PATTERN "engines" EXCLUDE
            PATTERN "pkgconfig" EXCLUDE)

    install(DIRECTORY
            ${openssl_install_dir}/include/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

    #uuid
    install(DIRECTORY
            ${uuid_install_dir}/lib/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_PLATFORM}/${CMAKE_INSTALL_LIBDIR}
            FILES_MATCHING PATTERN "lib*.so*"
            PATTERN "pkgconfig" EXCLUDE)

    install(DIRECTORY
            ${uuid_install_dir}/include/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

    #jsoncpp
    install(DIRECTORY
            ${jsoncpp_install_dir}/lib/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_PLATFORM}/${CMAKE_INSTALL_LIBDIR}
            FILES_MATCHING PATTERN "lib*.so*")

    install(DIRECTORY
            ${jsoncpp_install_dir}/include/jsoncpp/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
endfunction()

#demo
set(SDK_DEMO_DIRECTORY PublicCloud)

function(installDemoFiles)
    if(${ENABLE_BUILD_ASR})
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/demo
                FILES_MATCHING
                PATTERN "speechRecognizer*.cpp")
    endif()

    if(${ENABLE_BUILD_REALTIME})
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/demo
                FILES_MATCHING
                PATTERN "speechTranscriber*.cpp")
    endif()

    if(${ENABLE_BUILD_TTS})
        install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/speechSynthesizerDemo.cpp
                DESTINATION
                ${CMAKE_INSTALL_PREFIX}/demo)
    endif()

    install(DIRECTORY
            ${CMAKE_SOURCE_DIR}/demo/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/demo
            FILES_MATCHING
            PATTERN "test*.wav")
endfunction()

#NlsCommonSdk
function(installNlsCommonSdkFiles)
    #curl
    install(DIRECTORY
            ${CMAKE_SOURCE_DIR}/lib/linux/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_PLATFORM}/${CMAKE_INSTALL_LIBDIR}
            FILES_MATCHING PATTERN "libcurl.*")

    install(DIRECTORY
            ${CMAKE_SOURCE_DIR}/nlsCppSdk/thirdparty/curl
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/curl)

    #libnlsCommonSdk
    install(DIRECTORY
            ${CMAKE_SOURCE_DIR}/lib/linux/
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_PLATFORM}/${CMAKE_INSTALL_LIBDIR}
            FILES_MATCHING PATTERN "libnlsCommonSdk.*")

    install(DIRECTORY
            ${CMAKE_SOURCE_DIR}/nlsCppSdk/thirdparty/nlsCommonSdk
            DESTINATION
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/nlsCommonSdk)
endfunction()

#安装过程
#头文件
installHeaderFiles()

#version等文件
installCommentsFiles()
#readme.md仅使用与linux, windows
install(FILES ${CMAKE_SOURCE_DIR}/readme.md DESTINATION ${CMAKE_INSTALL_PREFIX})

#nlsCppSdk
if(${ENABLE_BUILD_LINUX})
    install(TARGETS nlsCppSdk
                ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_PLATFORM}/${CMAKE_INSTALL_LIBDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_PLATFORM}/${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_PLATFORM}/${CMAKE_INSTALL_BINDIR})
endif()

#依赖库，头文件
installCommonFiles()

#demo
installDemoFiles()

#nlsCommonSdk
installNlsCommonSdkFiles()

