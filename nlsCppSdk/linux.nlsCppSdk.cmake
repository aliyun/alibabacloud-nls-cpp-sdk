
#编译选项
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -Wall -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -fPIC -fvisibility=hidden -fvisibility-inlines-hidden")

#forbidden C++11
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
#set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Bsymbolic")

#SDK生成路径
set(SDK_BASE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(SDK_FOLDER NlsSdk3.X_LINUX)

#安装目录
set(CMAKE_INSTALL_TMP_PREFIX ${CMAKE_SOURCE_DIR}/build_linux_sdk/install/${SDK_FOLDER}/tmp)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/build_linux_sdk/install/${SDK_FOLDER})
set(CMAKE_INSTALL_LIB_PREFIX ${CMAKE_SOURCE_DIR}/build_linux_sdk/install/${SDK_FOLDER}/lib)

set(CMAKE_INSTALL_DEMO_INCLUDE_PREFIX ${CMAKE_SOURCE_DIR}/build_linux_sdk/install/${SDK_FOLDER}/demo/include)
set(CMAKE_INSTALL_DEMO_LIB_PREFIX ${CMAKE_SOURCE_DIR}/build_linux_sdk/install/${SDK_FOLDER}/demo/lib)
set(CMAKE_INSTALL_DEMO_PREFIX ${CMAKE_SOURCE_DIR}/build_linux_sdk/install/${SDK_FOLDER}/demo)

#专有云或公有云
set(SDK_DEMO_DIRECTORY Linux_Demo)

#编译依赖库设置
ExternalProject_Get_Property(jsoncpp INSTALL_DIR)
set(jsoncpp_install_dir ${INSTALL_DIR})
message(STATUS "jsoncpp install path: ${jsoncpp_install_dir}")

ExternalProject_Get_Property(opus INSTALL_DIR)
set(opus_install_dir ${INSTALL_DIR})
message(STATUS "opus install path: ${opus_install_dir}")

ExternalProject_Get_Property(openssl INSTALL_DIR)
set(openssl_install_dir ${INSTALL_DIR})
message(STATUS "openssl install path: ${openssl_install_dir}")

ExternalProject_Get_Property(log4cpp INSTALL_DIR)
set(log4cpp_install_dir ${INSTALL_DIR})
message(STATUS "log4cpp install path: ${log4cpp_install_dir}")

ExternalProject_Get_Property(libevent INSTALL_DIR)
set(libevent_install_dir ${INSTALL_DIR})
message(STATUS "libevent install path: ${libevent_install_dir}")


######uuid使用libevent生成######
ExternalProject_Get_Property(uuid INSTALL_DIR)
set(uuid_install_dir ${INSTALL_DIR})
message(STATUS "uuid install path: ${uuid_install_dir}")

set(nlsOpu_install_dir ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu)
message(STATUS "uuid install path: ${uuid_install_dir}")

#源文件-nlsOpu
set(NLS_OPU_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.c
        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.h
        )

#nls opu
add_library(nlsCppOpu SHARED ${NLS_OPU_FILE_LIST})
target_include_directories(nlsCppOpu
        PRIVATE
        ${opus_install_dir}/include
        )
target_link_libraries(nlsCppOpu ${opus_install_dir}/lib/libopus.a)

add_library(nlsCppOpu-a STATIC ${NLS_OPU_FILE_LIST})
target_include_directories(nlsCppOpu-a
        PRIVATE
        ${opus_install_dir}/include
        )
target_link_libraries(nlsCppOpu-a ${opus_install_dir}/lib/libopus.a)

set_target_properties(nlsCppOpu
        PROPERTIES
        LINKER_LANGUAGE CXX
        ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
        LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
        OUTPUT_NAME nlsCppOpu
        )

set_target_properties(nlsCppOpu-a
        PROPERTIES
        LINKER_LANGUAGE CXX
        ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
        LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
        OUTPUT_NAME nlsCppOpu
        )

ADD_DEFINITIONS(-D_NLS_OPU_SHARED_)

#源文件-nlsOpuJni
#set(NLS_OPU_JNI_FILE_LIST
#        ${NLS_OPU_FILE_LIST}
#        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/com_alibaba_nls_client_util_OpuCodec.h
#        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/com_alibaba_nls_client_util_OpuCodec.c
#        )
#
#add_library(nlsJniOpu SHARED ${NLS_OPU_JNI_FILE_LIST})
#target_include_directories(nlsJniOpu
#        PRIVATE
##        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu
#        ${opus_install_dir}/include
#        /usr/java/jdk1.8.0_121/include/
#        /usr/java/jdk1.8.0_121/include/linux
#        )
#target_link_libraries(nlsJniOpu ${opus_install_dir}/lib/libopus.a)
#
#add_library(nlsJniOpu-a STATIC ${NLS_OPU_FILE_LIST})
#target_include_directories(nlsJniOpu-a
#        PRIVATE
#        ${opus_install_dir}/include
#        )
#target_link_libraries(nlsJniOpu-a ${opus_install_dir}/lib/libopus.a)
#
#set_target_properties(nlsJniOpu
#        PROPERTIES
#        LINKER_LANGUAGE CXX
#        ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
#        LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
#        OUTPUT_NAME nlsJniOpu
#        )
#
#set_target_properties(nlsJniOpu-a
#        PROPERTIES
#        LINKER_LANGUAGE CXX
#        ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
#        LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
#        OUTPUT_NAME nlsJniOpu
#        )

#header file
set(LINUX_HEDER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/util
        ${CMAKE_CURRENT_SOURCE_DIR}/transport
        ${CMAKE_CURRENT_SOURCE_DIR}/framework
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature
        ${jsoncpp_install_dir}/include/jsoncpp
        ${opus_install_dir}/include
        ${openssl_install_dir}/include
        ${log4cpp_install_dir}/include
        ${libevent_install_dir}/include
        ${uuid_install_dir}/include
        ${nlsOpu_install_dir}
        )

set(LINUX_HEDER_FILE_LIST
        ${LINUX_HEDER_FILE_LIST}
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vipServerClient
        )

#lib file
set(LINUX_LIB_FILE_LIST
        ${openssl_install_dir}/lib/libssl.a
        ${openssl_install_dir}/lib/libcrypto.a
        ${opus_install_dir}/lib/libopus.a
        ${libevent_install_dir}/lib/libevent_core.a
        ${libevent_install_dir}/lib/libevent_extra.a
        ${libevent_install_dir}/lib/libevent_pthreads.a
        ${log4cpp_install_dir}/lib/liblog4cpp.a
        ${jsoncpp_install_dir}/lib/libjsoncpp.a
        ${uuid_install_dir}/lib/libuuid.a
        pthread
        )

#vipserver.lib
if (ENABLE_BUILD_PRIVATE_SDK)
set(LINUX_LIB_FILE_LIST
        ${LINUX_LIB_FILE_LIST}
        ${CMAKE_SOURCE_DIR}/lib/linux/libvsclient.a
        ${CMAKE_SOURCE_DIR}/lib/linux/libcurl.a
        dl
        rt
        z
        )
endif()

#安装基础依赖库文件
function(installCommonFiles)
    #openssl
    install(FILES ${openssl_install_dir}/lib/libcrypto.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

    install(FILES ${openssl_install_dir}/lib/libssl.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

    #opus
    install(FILES ${opus_install_dir}/lib/libopus.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

    #uuid
    install(FILES ${uuid_install_dir}/lib/libuuid.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

    #jsoncpp
    install(FILES ${jsoncpp_install_dir}/lib/libjsoncpp.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

    #log4cpp
    install(FILES ${log4cpp_install_dir}/lib/liblog4cpp.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

    #libevent
    install(FILES ${libevent_install_dir}/lib/libevent_core.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

    install(FILES ${libevent_install_dir}/lib/libevent_extra.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

    install(FILES ${libevent_install_dir}/lib/libevent_pthreads.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})

if (ENABLE_BUILD_PRIVATE_SDK)
    install(FILES ${CMAKE_SOURCE_DIR}/lib/linux/libvsclient.a
            DESTINATION
            ${CMAKE_INSTALL_TMP_PREFIX})
endif()

endfunction()


#demo
function(installDemoFiles)
    if(ENABLE_BUILD_ASR)
        install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/speechRecognizerDemo.cpp
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})

        install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/speechRecognizerSyncDemo.cpp
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})
    endif()

    if(ENABLE_BUILD_REALTIME)
        install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/speechTranscriberDemo.cpp
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})

        install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/speechTranscriberSyncDemo.cpp
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})
    endif()

    if(ENABLE_BUILD_TTS)
        install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/speechSynthesizerDemo.cpp
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})
    endif()

    install(FILES ${CMAKE_SOURCE_DIR}/demo/test0.wav
            DESTINATION
            ${CMAKE_INSTALL_DEMO_PREFIX})

    install(FILES ${CMAKE_SOURCE_DIR}/demo/test1.wav
            DESTINATION
            ${CMAKE_INSTALL_DEMO_PREFIX})

    install(FILES ${CMAKE_SOURCE_DIR}/demo/test2.wav
            DESTINATION
            ${CMAKE_INSTALL_DEMO_PREFIX})

    install(FILES ${CMAKE_SOURCE_DIR}/demo/test3.wav
            DESTINATION
            ${CMAKE_INSTALL_DEMO_PREFIX})

if (ENABLE_BUILD_PRIVATE_SDK)
    install(FILES ${CMAKE_SOURCE_DIR}/demo/Linux_Demo/vipServerDemo.cpp
            DESTINATION
            ${CMAKE_INSTALL_DEMO_PREFIX})

    install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/Private_CMakeLists.txt
            DESTINATION ${CMAKE_INSTALL_PREFIX}/
            RENAME CMakeLists.txt
            )
else()
    install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/Public_CMakeLists.txt
            DESTINATION ${CMAKE_INSTALL_PREFIX}/
            RENAME CMakeLists.txt
            )
endif()

endfunction()

#NlsCommonSdk
function(installNlsCommonSdkFiles)
    #libnlsCommonSdk
    install(FILES ${CMAKE_SOURCE_DIR}/lib/linux/libalibabacloud-idst-common.so
            DESTINATION ${CMAKE_INSTALL_DEMO_LIB_PREFIX}
            )

    install(DIRECTORY
            ${CMAKE_SOURCE_DIR}/nlsCppSdk/thirdparty/nlsCommonSdk/
            DESTINATION
            ${CMAKE_INSTALL_DEMO_INCLUDE_PREFIX}/nlsCommonSdk)
endfunction()

#安装过程
function(installNlsLinuxSdkFiles)
    #头文件
    installHeaderFiles()

    #version等文件
    installCommentsFiles()

    #readme.md仅使用与linux, windows
    install(FILES ${CMAKE_SOURCE_DIR}/demo/readme.md DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES ${CMAKE_SOURCE_DIR}/demo/build.sh DESTINATION ${CMAKE_INSTALL_PREFIX})

    #nlsCppSdk
    install(FILES ${SDK_BASE_OUTPUT_PATH}/lib${NLS_SDK_OUTPUT_NAME}.so DESTINATION ${CMAKE_INSTALL_LIB_PREFIX})
    install(FILES ${SDK_BASE_OUTPUT_PATH}/lib${NLS_SDK_OUTPUT_NAME}.a DESTINATION ${CMAKE_INSTALL_TMP_PREFIX})

    #依赖库，头文件
    installCommonFiles()

    #demo
    installDemoFiles()

    installNlsCommonSdkFiles()

    message(STATUS "CMAKE_INSTALL_INCLUDEDIR: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
    message(STATUS "CMAKE_INSTALL_LIBDIR: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    message(STATUS "CMAKE_INSTALL_BINDIR: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")

endfunction()
