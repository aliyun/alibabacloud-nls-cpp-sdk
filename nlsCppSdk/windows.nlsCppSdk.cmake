
#编译选项
#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS}")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")

#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -Wall -fPIC")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -fPIC -fvisibility=hidden -fvisibility-inlines-hidden")

#forbidden C++11
#add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
#set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Bsymbolic")

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
add_definitions(-DWIN32_LEAN_AND_MEAN)

set(SDK_BUILD_DIR build_windows_sdk)
set(SDK_INSTALL_DIR NlsSdk3.X_Windows)

#安装目录
set(CMAKE_INSTALL_TMP_PREFIX ${CMAKE_SOURCE_DIR}/${SDK_BUILD_DIR}/install/${SDK_INSTALL_DIR}/tmp)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/${SDK_BUILD_DIR}/install/${SDK_INSTALL_DIR})
set(CMAKE_INSTALL_LIB_PREFIX ${CMAKE_SOURCE_DIR}/${SDK_BUILD_DIR}/install/${SDK_INSTALL_DIR}/lib)

set(CMAKE_INSTALL_DEMO_INCLUDE_PREFIX ${CMAKE_SOURCE_DIR}/${SDK_BUILD_DIR}/install/${SDK_INSTALL_DIR}/demo/include)
set(CMAKE_INSTALL_DEMO_LIB_PREFIX ${CMAKE_SOURCE_DIR}/${SDK_BUILD_DIR}/install/${SDK_INSTALL_DIR}/demo/lib)
set(CMAKE_INSTALL_DEMO_PREFIX ${CMAKE_SOURCE_DIR}/${SDK_BUILD_DIR}/install/${SDK_INSTALL_DIR}/demo)

#专有云或公有云
set(SDK_DEMO_DIRECTORY Win32_Demo)

#VS版本
if(${MSVC_VERSION} EQUAL 1900) 
	message(STATUS "Build in VS2015")
	set(LIB_MIDDLE_PATH "14.0")
elseif(${MSVC_VERSION} EQUAL 1800)
	message(STATUS "Build in VS2013")
	set(LIB_MIDDLE_PATH "12.0")
elseif(${MSVC_VERSION} EQUAL 1911)
	message(STATUS "Build in VS2017")
endif()

if(ENABLE_Release)
	message(STATUS "Enable ENABLE_Release.")
endif()

if(ENABLE_X64)
	message(STATUS "Enable ENABLE_X64.")
endif()

#Debug or Release
if(${ENABLE_Release})
	set(LIB_MODE_PATH "Release")
	set(SDK_BASE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/nlsCppSdk/Release) #SDK生成路径
else()	
	set(LIB_MODE_PATH "Debug")
	set(SDK_BASE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/nlsCppSdk/Debug) #SDK生成路径
endif()

#x86 or x64
if(${ENABLE_X64})
	set(LIB_CPU_PATH "x64")
else()	
	set(LIB_CPU_PATH "x86")
endif()

set(COMMON_LIB_PATH ${CMAKE_SOURCE_DIR}/lib/windows/${LIB_MIDDLE_PATH}/${LIB_CPU_PATH}/${LIB_MODE_PATH})
message(STATUS "Win Common Lib Path:${COMMON_LIB_PATH}")

#编译依赖库设置
#set(jsoncpp_install_dir ${COMMON_LIB_PATH}/jsoncpp.lib)
#message(STATUS "jsoncpp install path: ${jsoncpp_install_dir}")

#set(opus_install_dir ${COMMON_LIB_PATH}/opus.lib)
#message(STATUS "opus install path: ${opus_install_dir}")

#set(openssl_install_dir ${COMMON_LIB_PATH}/libcrypto.lib)
#set(openssl_install_dir ${COMMON_LIB_PATH}/libssl.lib)
#message(STATUS "openssl install path: ${openssl_install_dir}")

#set(log4cpp_install_dir ${COMMON_LIB_PATH}/log4cpp.lib)
#message(STATUS "log4cpp install path: ${log4cpp_install_dir}")

#set(libevent_install_dir ${COMMON_LIB_PATH}/event.lib)
#set(libevent_install_dir ${COMMON_LIB_PATH}/event_core.lib)
#set(libevent_install_dir ${COMMON_LIB_PATH}/event_extra.lib)
#message(STATUS "libevent install path: ${libevent_install_dir}")

#源文件-nlsOpu
#set(NLS_OPU_FILE_LIST
#        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.c
#        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu/nlsOpuCoder.h
#        )

#nls opu
#add_library(nlsCppOpu SHARED ${NLS_OPU_FILE_LIST})
#target_include_directories(nlsCppOpu
#        PRIVATE
#        ${opus_install_dir}/include
#        )
#target_link_libraries(nlsCppOpu ${opus_install_dir}/lib/libopus.a)

#add_library(nlsCppOpu-a STATIC ${NLS_OPU_FILE_LIST})
#target_include_directories(nlsCppOpu-a
#        PRIVATE
#        ${opus_install_dir}/include
#        )
#target_link_libraries(nlsCppOpu-a ${opus_install_dir}/lib/libopus.a)

#set_target_properties(nlsCppOpu
#        PROPERTIES
#        LINKER_LANGUAGE CXX
#        ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
#        LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
#        OUTPUT_NAME nlsCppOpu
#        )

#set_target_properties(nlsCppOpu-a
#        PROPERTIES
#        LINKER_LANGUAGE CXX
#        ARCHIVE_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
#        LIBRARY_OUTPUT_DIRECTORY ${SDK_BASE_OUTPUT_PATH}
#        OUTPUT_NAME nlsCppOpu
#        )

#ADD_DEFINITIONS(-D_NLS_OPU_SHARED_)

#header file
set(WINDOWS_HEDER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/util
        ${CMAKE_CURRENT_SOURCE_DIR}/transport
        ${CMAKE_CURRENT_SOURCE_DIR}/framework
		${CMAKE_CURRENT_SOURCE_DIR}/nlsOpu
        ${CMAKE_CURRENT_SOURCE_DIR}/framework/feature
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libevent
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/log4cpp
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openssl
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/opus
        )

#lib file
set(WINDOWS_LIB_FILE_LIST
        ${COMMON_LIB_PATH}/libcrypto.lib
        ${COMMON_LIB_PATH}/libssl.lib
        ${COMMON_LIB_PATH}/opus.lib
        ${COMMON_LIB_PATH}/event.lib
        ${COMMON_LIB_PATH}/event_core.lib
        ${COMMON_LIB_PATH}/event_extra.lib
        ${COMMON_LIB_PATH}/log4cpp.lib
        ${COMMON_LIB_PATH}/jsoncpp.lib
		Crypt32.lib
        )
		
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

    if(ENABLE_BUILD_UDS)
        install(FILES ${CMAKE_SOURCE_DIR}/demo/${SDK_DEMO_DIRECTORY}/dialogAssistantDemo.cpp
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})

        install(FILES ${CMAKE_SOURCE_DIR}/demo/dialogAssistant0.wav
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})

        install(FILES ${CMAKE_SOURCE_DIR}/demo/dialogAssistant1.wav
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})

        install(FILES ${CMAKE_SOURCE_DIR}/demo/dialogAssistant2.wav
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})

        install(FILES ${CMAKE_SOURCE_DIR}/demo/dialogAssistant3.wav
                DESTINATION
                ${CMAKE_INSTALL_DEMO_PREFIX})
    endif()

    install(DIRECTORY
            ${CMAKE_SOURCE_DIR}/demo/
            DESTINATION
            ${CMAKE_INSTALL_DEMO_PREFIX}
            FILES_MATCHING
            PATTERN "test*.wav")

    install(FILES ${CMAKE_SOURCE_DIR}/demo/vipServerDemo.cpp
            DESTINATION
            ${CMAKE_INSTALL_DEMO_PREFIX})


    if(ENABLE_BUILD_PRIVATE_SDK)
        install(FILES ${CMAKE_SOURCE_DIR}/demo/Private_CMakeLists.txt
                DESTINATION ${CMAKE_INSTALL_PREFIX}/
                RENAME CMakeLists.txt
                )
    else()
        install(FILES ${CMAKE_SOURCE_DIR}/demo/Public_CMakeLists.txt
                DESTINATION ${CMAKE_INSTALL_PREFIX}/
                RENAME CMakeLists.txt
                )
    endif()
endfunction()

#NlsCommonSdk
function(installNlsCommonSdkFiles)
    #nlsCommonSdk
    install(FILES ${COMMON_LIB_PATH}/nlsCommonSdk.lib
            DESTINATION 
			${CMAKE_INSTALL_DEMO_LIB_PREFIX}
            )

    install(DIRECTORY
            ${CMAKE_SOURCE_DIR}/nlsCppSdk/thirdparty/nlsCommonSdk/
            DESTINATION
            ${CMAKE_INSTALL_DEMO_INCLUDE_PREFIX}/nlsCommonSdk)
endfunction()

#安装过程
function(installNlsWindowsSdkFiles)
    #头文件
    installHeaderFiles()

    #version等文件
    installCommentsFiles()
    #readme.md仅使用与linux, windows
    install(FILES ${CMAKE_SOURCE_DIR}/readme.md DESTINATION ${CMAKE_INSTALL_PREFIX})

    #nlsCppSdk
    install(FILES ${SDK_BASE_OUTPUT_PATH}/${NLS_SDK_OUTPUT_NAME}.dll DESTINATION ${CMAKE_INSTALL_LIB_PREFIX})
	install(FILES ${SDK_BASE_OUTPUT_PATH}/${NLS_SDK_OUTPUT_NAME}.lib DESTINATION ${CMAKE_INSTALL_LIB_PREFIX})

    #demo
    installDemoFiles()

    installNlsCommonSdkFiles()

    message(STATUS "CMAKE_INSTALL_INCLUDEDIR: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
    message(STATUS "CMAKE_INSTALL_LIBDIR: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    message(STATUS "CMAKE_INSTALL_BINDIR: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")

endfunction()
