
######uuid使用libevent生成######
set(UUID_C_FLAGS "-fPIC -fvisibility=hidden")
set(UUID_EXTERNAL_COMPILER_FLAGS
    URL ${UUID_URL}
    URL_HASH MD5=${UUID_URL_HASH}
    CONFIGURE_COMMAND ./configure CFLAGS=${UUID_C_FLAGS} enable_shared=no enable_static=yes --prefix=<INSTALL_DIR>
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${MAKE}
    )
option(UUID_ENABLE "Enable Uuid." ON)

set(OGG_C_FLAGS "-fPIC -fvisibility=hidden")
set(OGG_EXTERNAL_COMPILER_FLAGS
    URL ${OGG_URL}
    URL_HASH MD5=${OGG_URL_HASH}
    CONFIGURE_COMMAND ./configure CFLAGS=${OGG_C_FLAGS} enable_shared=no enable_static=yes --prefix=<INSTALL_DIR>
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${MAKE}
    )
option(OGG_ENABLE "Enable Ogg." ON)

set(OPUS_C_FLAGS "-fPIC -fvisibility=hidden")
set(OPUS_CXX_FLAGS "-fPIC -fvisibility=hidden -ffast-math")
set(OPUS_EXTERNAL_COMPILER_FLAGS
    URL ${OPUS_URL}
    URL_HASH MD5=${OPUS_URL_HASH}
    CONFIGURE_COMMAND ./configure CFLAGS=${OPUS_C_FLAGS} CXXFLAGS=${OPUS_CXX_FLAGS} enable_shared=no enable_static=yes --prefix=<INSTALL_DIR>
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${MAKE}
    )
option(OPUS_ENABLE "Enable Opus." ON)

set(OPENSSL_EXTERNAL_COMPILER_FLAGS
    URL ${OPENSSL_URL}
    URL_HASH MD5=${OPENSSL_URL_HASH}
    CONFIGURE_COMMAND ./config -fPIC threads no-shared -fvisibility=hidden --prefix=<INSTALL_DIR>
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${MAKE}
    )
option(OPENSSL_ENABLE "Enable Openssl." ON)

set(LOG4CPP_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -D_GLIBCXX_USE_CXX11_ABI=${CXX11_ABI} -fvisibility=hidden")
if (CXX11_ABI)
  message(STATUS "LOG4CPP support std=c++11")
  set(LOG4CPP_CXX_FLAGS "${LOG4CPP_CXX_FLAGS} -std=c++11")
endif ()
set(LOG4CPP_EXTERNAL_COMPILER_FLAGS
    URL ${LOG4CPP_URL}
    URL_HASH MD5=${LOG4CPP_URL_HASH}
    CONFIGURE_COMMAND  ./configure CXXFLAGS=${LOG4CPP_CXX_FLAGS} --disable-symbols-visibility-options enable_debug=no enable_shared=no enable_static=yes --prefix=<INSTALL_DIR>
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${MAKE}
    )
option(LOG4CPP_ENABLE "Enable Log4Cpp." ON)

set(LIBEVENT_C_FLAGS "-fPIC -fvisibility=hidden")
set(LIBEVENT_EXTERNAL_COMPILER_FLAGS
    URL ${LIBEVENT_URL}
    URL_HASH MD5=${LIBEVENT_URL_HASH}
    CONFIGURE_COMMAND ./configure CFLAGS=${LIBEVENT_C_FLAGS} enable_debug_mode=no enable_static=yes enable_shared=no --disable-openssl --prefix=<INSTALL_DIR>
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${MAKE}
    )
option(LIBEVENT_ENABLE "Enable Libevent." ON)

set(JSONCPP_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden -D_GLIBCXX_USE_CXX11_ABI=${CXX11_ABI}")
if (CXX11_ABI)
  message(STATUS "JSONCPP support std=c++11")
  set(JSONCPP_CXX_FLAGS "${JSONCPP_CXX_FLAGS} -std=c++11")

  set(JSONCPP_EXTERNAL_COMPILER_FLAGS
      URL ${JSONCPP1_URL}
      URL_HASH MD5=${JSONCPP1_URL_HASH}
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_CXX_FLAGS=${JSONCPP_CXX_FLAGS}
        -DJSONCPP_WITH_TESTS=OFF
        -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF
        -DJSONCPP_WITH_WARNING_AS_ERROR=OFF
        -DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF
        -DJSONCPP_WITH_CMAKE_PACKAGE=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_STATIC_LIBS=ON
      )
else ()
  set(JSONCPP_EXTERNAL_COMPILER_FLAGS
      URL ${JSONCPP0_URL}
      URL_HASH MD5=${JSONCPP0_URL_HASH}
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_CXX_FLAGS=${JSONCPP_CXX_FLAGS}
        -DJSONCPP_WITH_TESTS=OFF
        -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF
        -DJSONCPP_WITH_WARNING_AS_ERROR=OFF
        -DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF
        -DJSONCPP_WITH_CMAKE_PACKAGE=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_STATIC_LIBS=ON
      )
endif ()
option(JSONCPP_ENABLE "Enable Jsoncpp." ON)

set(CURL_C_FLAGS "-fPIC -fvisibility=hidden -D_GLIBCXX_USE_CXX11_ABI=${CXX11_ABI}")
set(CURL_WITHOUT_PACKAGE  --without-nghttp2 --without-nghttp3 --without-libidn2 --without-zstd --without-brotli --without-ldap --without-ldaps --without-rtsp --without-rtmp)
set(CURL_EXTERNAL_COMPILER_FLAGS
    URL ${CURL_URL}
    URL_HASH MD5=${CURL_URL_HASH}
    CONFIGURE_COMMAND ./configure CFLAGS=${CURL_C_FLAGS} enable_debug=no enable_shared=yes enable_static=yes --prefix=<INSTALL_DIR> ${CURL_WITHOUT_PACKAGE} --with-openssl=<INSTALL_DIR>/../openssl-prefix/
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${MAKE}
    )
option(CURL_ENABLE "Enable Curl." ON)

