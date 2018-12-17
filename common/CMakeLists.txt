
include(ExternalProject)
include(GNUInstallDirs)

set(JSONCPP_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ExternalProject_Add(jsoncpp
            PREFIX jsoncpp
            DOWNLOAD_NAME jsoncpp-0.10.6.tar.gz
            DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
            URL https://github.com/open-source-parsers/jsoncpp/archive/0.10.6.tar.gz
            URL_HASH MD5=13d1991d79697df8cadbc25c93e37c83
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DCMAKE_CXX_FLAGS=${JSONCPP_CXX_FLAGS}
            -DJSONCPP_WITH_TESTS=OFF
            -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF
            -DJSONCPP_WITH_WARNING_AS_ERROR=OFF
            -DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF
            -DJSONCPP_WITH_CMAKE_PACKAGE=OFF
            -DBUILD_SHARED_LIBS=ON
            -DBUILD_STATIC_LIBS=ON)

ExternalProject_Add(uuid
            URL https://jaist.dl.sourceforge.net/project/libuuid/libuuid-1.0.3.tar.gz
            URL_HASH MD5=d44d866d06286c08ba0846aba1086d68
            DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
            CONFIGURE_COMMAND ./configure CFLAGS=-fPIC --prefix=<INSTALL_DIR>
            BUILD_IN_SOURCE 1
            BUILD_COMMAND ${MAKE})

ExternalProject_Add(openssl
            URL https://www.openssl.org/source/old/1.0.2/openssl-1.0.2j.tar.gz
            URL_HASH MD5=96322138f0b69e61b7212bc53d5e912b
            DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
            CONFIGURE_COMMAND ./config shared -fPIC --prefix=<INSTALL_DIR>
            BUILD_IN_SOURCE 1
            BUILD_COMMAND ${MAKE})


