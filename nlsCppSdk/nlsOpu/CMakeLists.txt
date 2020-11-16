
#编译依赖库设置
set(opus_install_dir ${BUILD_PLATFORM_WORK_PATH}/common/opus-prefix)
message(STATUS "opus install path: ${opus_install_dir}")

ADD_DEFINITIONS(-D_NLS_OPU_SHARED_)

#源文件-nlsOpuJni
set(NLS_OPU_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpuCoder.c
        ${CMAKE_CURRENT_SOURCE_DIR}/nlsOpuCoder.h
        )

set(NLS_OPU_JNI_FILE_LIST
        ${NLS_OPU_FILE_LIST}
        ${CMAKE_CURRENT_SOURCE_DIR}/com_alibaba_nls_client_util_OpuCodec.h
        ${CMAKE_CURRENT_SOURCE_DIR}/com_alibaba_nls_client_util_OpuCodec.c
        )

add_library(nlsJniOpu SHARED ${NLS_OPU_JNI_FILE_LIST})
target_include_directories(nlsJniOpu
        PRIVATE
        ${opus_install_dir}/include
        /Library/Java/JavaVirtualMachines/jdk1.8.0_151.jdk/Contents/Home/include/
        /Library/Java/JavaVirtualMachines/jdk1.8.0_151.jdk/Contents/Home/include/darwin
        )
target_link_libraries(nlsJniOpu ${opus_install_dir}/lib/libopus.a)

add_library(nlsJniOpu-a STATIC ${NLS_OPU_JNI_FILE_LIST})
target_include_directories(nlsJniOpu-a
        PRIVATE
        ${opus_install_dir}/include
        )
target_link_libraries(nlsJniOpu-a ${opus_install_dir}/lib/libopus.a)

