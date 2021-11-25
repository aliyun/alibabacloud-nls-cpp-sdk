
set(SDK_FOLDER NlsSdk3.X_LINUX)

#安装目录
set(CMAKE_INSTALL_TMP_PREFIX ${CMAKE_SOURCE_DIR}/build/install/${SDK_FOLDER}/tmp)

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

ExternalProject_Get_Property(uuid INSTALL_DIR)
set(uuid_install_dir ${INSTALL_DIR})
message(STATUS "uuid install path: ${uuid_install_dir}")

ExternalProject_Get_Property(curl INSTALL_DIR)
set(curl_install_dir ${INSTALL_DIR})
message(STATUS "curl install path: ${curl_install_dir}")


#安装基础依赖库文件, 即搬运静态库到操作目录
function(installThirdpartySdkFiles)
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
  install(FILES ${jsoncpp_install_dir}/lib/${CMAKE_LIBRARY_ARCHITECTURE}/libjsoncpp.a
#  install(FILES ${jsoncpp_install_dir}/lib/libjsoncpp.a
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

  #curl
  install(FILES ${curl_install_dir}/lib/libcurl.a
          DESTINATION
          ${CMAKE_INSTALL_TMP_PREFIX})
endfunction()

