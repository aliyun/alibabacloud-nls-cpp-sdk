
include(ExternalProject)
include(GNUInstallDirs)

#后期处理开源，此文件需要由CMakeLists.txt生成
include(nls.common.url.cmake)

#include(opensource.common.url.cmake)

#加载编译选项
if(ENABLE_BUILD_IOS)
    include(ios.common.cmake)
elseif(ENABLE_BUILD_LINUX)
    include(linux.common.cmake)
elseif(ENABLE_BUILD_ANDROID)
    include(android.common.cmake)
elseif(ENABLE_BUILD_WINDOWS)
    include(windows.common.cmake)
elseif(ENABLE_BUILD_MACOS)
    include(macOS.common.cmake)
endif()

#编译第三方依赖库
######uuid使用libevent生成######
if(UUID_ENABLE)
    ExternalProject_Add(uuid ${UUID_EXTERNAL_COMPILER_FLAGS})
endif()

if(OPUS_ENABLE)
    ExternalProject_Add(opus ${OPUS_EXTERNAL_COMPILER_FLAGS})
endif()

#if(PERL_ENABLE)
    # 检查perl版本是否为5。10以上版本. perl -v 可以查看版本号. openssl 安装依赖perl 5.10
#./Configure -des -Dprefix=/usr/local/perl
#make -j2
#make test
#make install
#    ExternalProject_Add(perl-5.28 ${OPENSSL_EXTERNAL_COMPILER_FLAGS})
#endif()

if(OPENSSL_ENABLE)
    ExternalProject_Add(openssl ${OPENSSL_EXTERNAL_COMPILER_FLAGS})
endif()

if(LOG4CPP_ENABLE)
    ExternalProject_Add(log4cpp ${LOG4CPP_EXTERNAL_COMPILER_FLAGS})
endif()

if(LIBEVENT_ENABLE)
    ExternalProject_Add(libevent ${LIBEVENT_EXTERNAL_COMPILER_FLAGS})
endif()

if(JSONCPP_ENABLE)
    ExternalProject_Add(jsoncpp ${JSONCPP_EXTERNAL_COMPILER_FLAGS})
endif()

#if(GTEST_ENABLE)
#    ExternalProject_Add(gtest ${GTEST_EXTERNAL_COMPILER_FLAGS})
#endif()
