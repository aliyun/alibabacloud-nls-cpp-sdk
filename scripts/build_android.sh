#!/bin/bash -e

echo "Command:"
echo "./scripts/build_android.sh <all or incr> <debug or release> <android arch>"
echo "eg: ./scripts/build_android.sh all debug arm64-v8a"


ALL_FLAG=$1
DEBUG_FLAG=$2
PLATFORM_FLAG=$3
if [ $# == 0 ]; then
  ALL_FLAG="incr"
  DEBUG_FLAG="debug"
  PLATFORM_FLAG="arm64-v8a"
fi
if [ $# == 1 ]; then
  ALL_FLAG=$1
  DEBUG_FLAG="debug"
  PLATFORM_FLAG="arm64-v8a"
fi

git_root_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
g_build_folder=$git_root_path/build
build_folder=$g_build_folder/build_$PLATFORM_FLAG
audio_resource_folder=$git_root_path/resource/audio
if [ ! -d $build_folder ];then
  mkdir -p $build_folder
fi

if [ x${ALL_FLAG} == x"all" ];then
  rm -rf $build_folder/*
fi

#设置环境变量
source $git_root_path/config/android_bash_profile


if [ x${PLATFORM_FLAG} == x"arm64-v8a" ];then
  echo "Build arm64-v8a."
  TARGET_ARCH=arm64
  ANDROID_COMPILE_NAME=aarch64-linux-android
  CC=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME$ANDROID_API-clang++
  AR=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ar
  RANLIB=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ranlib
  STRIP=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-strip
  LD=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ld
elif [ x${PLATFORM_FLAG} == x"armeabi-v7a" ];then
  echo "Build armeabi-v7a."
  TARGET_ARCH=arm
  ANDROID_COMPILE_NAME=arm-linux-androideabi
  CC=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi$ANDROID_API-clang++
  AR=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ar
  RANLIB=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ranlib
  STRIP=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-strip
  LD=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ld
elif [ x${PLATFORM_FLAG} == x"x86" ];then
  echo "Build x86."
  TARGET_ARCH=x86
  ANDROID_COMPILE_NAME=i686-linux-android
  CC=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME$ANDROID_API-clang++
  AR=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ar
  RANLIB=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ranlib
  STRIP=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-strip
  LD=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ld
elif [ x${PLATFORM_FLAG} == x"x86_64" ];then
  echo "Build x86_64."
  TARGET_ARCH=x86_64
  ANDROID_COMPILE_NAME=x86_64-linux-android
  CC=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME$ANDROID_API-clang++
  AR=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ar
  RANLIB=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ranlib
  STRIP=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-strip
  LD=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/$ANDROID_COMPILE_NAME-ld
else
  echo "Unsupported " $PLATFORM_FLAG
  echo "Supported platforms: arm64-v8a, armeabi, armeabi-v7a, x86, x86_64"

  exit 1
fi


### 1
echo "建立编译目录:"$build_folder

thirdparty_folder=$build_folder/thirdparty
echo "建立thirdparty目录:" $thirdparty_folder
mkdir -p $thirdparty_folder

install_folder=$g_build_folder/install
echo "建立install目录:" $install_folder
mkdir -p $install_folder

lib_folder=$build_folder/lib
echo "建立libs目录:" $lib_folder
mkdir -p $lib_folder

demo_folder=$build_folder/demo
echo "建立demo目录:" $demo_folder
mkdir -p $demo_folder

cd $build_folder

#开始编译
if [ x${DEBUG_FLAG} == x"release" ];then
  echo "BUILD_TYPE: Release..."
  echo "Target CPU:" $PLATFORM_FLAG

  # eg. ANDROID_NDK = ANDROID_NDK_ROOT = /home/fsc/toolchains/android-ndk-r21e
  #     ANDROID_ABI = PLATFORM_FLAG = arm64-v8a
  #     ANDROID_PLATFORM = android-21
  #     CMAKE_ANDROID_ARCH_ABI = ANDROID_ABI = 21
  cmake -DANDROID_PLATFORM_FOLDER=${build_folder} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Android \
        -DANDROID_COMPILE_NAME=${ANDROID_COMPILE_NAME} \
        -DANDROID_NDK=${ANDROID_NDK_ROOT} \
        -DANDROID_ABI=${PLATFORM_FLAG} \
        -DANDROID_PLATFORM=android-${ANDROID_API} \
        -DANDROID_API=${ANDROID_API} \
        -DANDROID_TARGET_ARCH=${TARGET_ARCH} \
        -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
        -DANDROID_DEPRECATED_HEADERS=ON \
        -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
        ../..
else
  echo "BUILD_TYPE: Debug..."
  cmake -DANDROID_PLATFORM_FOLDER=${build_folder} \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_SYSTEM_NAME=Android \
        -DANDROID_COMPILE_NAME=${ANDROID_COMPILE_NAME} \
        -DANDROID_NDK=${ANDROID_NDK_ROOT} \
        -DANDROID_ABI=${PLATFORM_FLAG} \
        -DANDROID_PLATFORM=android-${ANDROID_API} \
        -DANDROID_API=${ANDROID_API} \
        -DANDROID_TARGET_ARCH=${TARGET_ARCH} \
        -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
        -DANDROID_DEPRECATED_HEADERS=ON \
        -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
        ../..
fi
if [ x${ALL_FLAG} == x"all" ];then
  make clean
fi

echo "ANDROID_ABI:" $PLATFORM_FLAG
echo "Begin compile."

make
make install

cd $install_folder

echo "进入install目录:" $PWD

sdk_install_folder=$install_folder/NlsSdk3.X_$PLATFORM_FLAG
mkdir -p $sdk_install_folder


### libcrypto.a和libevent_core.a中有.o文件重名, 释放在同目录会覆盖, 导致编译找不到定义. 比如buffer.o
### 2
mkdir -p $sdk_install_folder/tmp/
mkdir -p $sdk_install_folder/lib
cd $sdk_install_folder/tmp/
echo "生成库libalibabacloud-idst-speech.X ..."
$AR x $build_folder/nlsCppSdk/libalibabacloud-idst-speech.a
$AR x libjsoncpp.a
$AR x libuuid.a
$AR x libogg.a
$AR x libopus.a
$AR x libevent.a
mv $sdk_install_folder/tmp/buffer.o $sdk_install_folder/tmp/libevent_buffer.o

$AR x libssl.a
$AR x libcrypto.a
if [ -f "libcurl.a" ]; then
  $AR x libcurl.a
elif [ -f "libcurl-d.a" ]; then
  $AR x libcurl-d.a
fi

$AR cr $sdk_install_folder/lib/libalibabacloud-idst-speech_$PLATFORM_FLAG.a *.o
$RANLIB $sdk_install_folder/lib/libalibabacloud-idst-speech_$PLATFORM_FLAG.a

$CC -shared -Wl,-Bsymbolic -Wl,-Bsymbolic-functions -fPIC -fvisibility=hidden -Wl,--exclude-libs,ALL -Wl,-z,relro,-z,now -Wl,-z,noexecstack -fstack-protector -o $sdk_install_folder/lib/libalibabacloud-idst-speech_$PLATFORM_FLAG.so *.o

if [ x${DEBUG_FLAG} == x"release" ];then
  $STRIP $sdk_install_folder/lib/libalibabacloud-idst-speech_$PLATFORM_FLAG.so
fi


### 4
echo "搬动头文件和库文件..."
mkdir -p $sdk_install_folder/include
cp $git_root_path/nlsCppSdk/framework/feature/sr/speechRecognizerRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/feature/st/speechTranscriberRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/feature/sy/speechSynthesizerRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/feature/da/dialogAssistantRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/item/iNlsRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/common/nlsClient.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/common/nlsGlobal.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/common/nlsEvent.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/token/include/nlsToken.h $sdk_install_folder/include/


### 5
echo "生成DEMO..."
cd $demo_folder
cmake -DANDROID_PLATFORM_FOLDER=${demo_folder} \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_SYSTEM_NAME=Android \
      -DANDROID_ABI=${PLATFORM_FLAG} \
      -DANDROID_PLATFORM=android-${ANDROID_API} \
      -DANDROID_TARGET_ARCH=arm64 \
      -DCMAKE_ANDROID_ARCH_ABI=${ANDROID_ABI} \
      -DANDROID_DEPRECATED_HEADERS=ON \
      -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
      ../../../demo/Android
make

echo "编译结束..."

### 6
cd $install_folder
mkdir -p $sdk_install_folder/demo
mkdir -p $sdk_install_folder/bin
cp $git_root_path/demo/Android/*.cpp $sdk_install_folder/demo
cp $git_root_path/version $sdk_install_folder/
cp $git_root_path/readme.md $sdk_install_folder/
cp $build_folder/demo/*Demo $sdk_install_folder/bin
cp -r $git_root_path/resource $sdk_install_folder/demo/
cur_date=$(date +%Y%m%d%H%M)

rm -rf $sdk_install_folder/tmp
tar_file=NlsSdk3.X_${PLATFORM_FLAG}_$cur_date
echo "压缩 " $sdk_install_folder "到" $tar_file".tar.gz"
tar -zcPf $tar_file".tar.gz" NlsSdk3.X_$PLATFORM_FLAG

echo "打包结束..."

cp $audio_resource_folder/* $build_folder/demo/
