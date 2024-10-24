#!/bin/bash -e

echo "Command:"
echo "./scripts/build_linux.sh <all or incr> <debug or release> <abi> <private>"
echo "eg: ./scripts/build_linux.sh all debug 0 0"

ALL_FLAG=$1
DEBUG_FLAG=$2
ABI_FLAG=$3
PRIVATE_FLAG=$4
if [ $# == 0 ]; then
  ALL_FLAG="incr"
  DEBUG_FLAG="debug"
  ABI_FLAG=0
  PRIVATE_FLAG=0
fi
if [ $# == 1 ]; then
  ALL_FLAG=$1
  DEBUG_FLAG="debug"
  ABI_FLAG=0
  PRIVATE_FLAG=0
fi
if [ $# == 2 ]; then
  ALL_FLAG=$1
  DEBUG_FLAG=$2
  ABI_FLAG=0
  PRIVATE_FLAG=0
fi
if [ $# == 3 ]; then
  ALL_FLAG=$1
  DEBUG_FLAG=$2
  ABI_FLAG=$3
  PRIVATE_FLAG=0
fi

if [ "$#" -lt 3 ]; then
  gcc_version=$(gcc -dumpversion)
  gcc_major_version=$(echo $gcc_version | cut -d. -f1)
  if [ "$gcc_major_version" -lt 5 ]; then
    ABI_FLAG=0
  else
    ABI_FLAG=1
  fi
  echo "当前GCC版本:" $gcc_version " 将ABI_FLAG设置为:" $ABI_FLAG
fi

git_root_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
build_folder=$git_root_path/build
audio_resource_folder=$git_root_path/resource/audio
if [ ! -d $build_folder ];then
  mkdir -p $build_folder
fi

if [ x${ALL_FLAG} == x"all" ];then
  rm -rf $build_folder/*
fi

if [ $PRIVATE_FLAG == 1 ];then
  # get libvsclient.a
  libvsclient_dir=$git_root_path/nlsCppSdk/vipServerClient/libs
  libvsclient_path=$libvsclient_dir/x86_64/libvsclient.a
  mkdir -p $libvsclient_dir
  mkdir -p $libvsclient_dir/x86_64
  wget -O $libvsclient_path https://isv-data.oss-cn-hangzhou.aliyuncs.com/ics/NLS/resources/libs/libvsclient_Linux-x86_64.a
fi

### 1
echo "建立编译目录:"$build_folder

thirdparty_folder=$build_folder/thirdparty
echo "建立thirdparty目录:" $thirdparty_folder
mkdir -p $thirdparty_folder

install_folder=$build_folder/install
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
echo "BUILD_CXX11_ABI:" ${ABI_FLAG}
echo "BUILD_PRIVATE_FLAG:" ${PRIVATE_FLAG}
if [ x${DEBUG_FLAG} == x"release" ];then
  echo "BUILD_TYPE: Release..."
  cmake -DCMAKE_BUILD_TYPE=Release -DCXX11_ABI=${ABI_FLAG} -DPRIVATE_CLOUD=${PRIVATE_FLAG} ..
else
  echo "BUILD_TYPE: Debug..."
  cmake -DCMAKE_BUILD_TYPE=Debug -DCXX11_ABI=${ABI_FLAG} -DPRIVATE_CLOUD=${PRIVATE_FLAG} ..
fi
if [ x${ALL_FLAG} == x"all" ];then
  make clean
fi
make
make install

cd $install_folder

echo "进入install目录:" $PWD

sdk_install_folder=$install_folder/NlsSdk3.X_LINUX


### 2
echo "生成库libalibabacloud-idst-speech.X ..."
cd $sdk_install_folder/tmp/
rm -f *.o

if [ $PRIVATE_FLAG == 1 ];then
  cp $libvsclient_path libvsclient.a
  ar x libvsclient.a
  for files in $(ls *.o)
    do mv $files "libvsclient_"$files
  done
fi

ar x $build_folder/nlsCppSdk/libalibabacloud-idst-speech.a
ar x liblog4cpp.a
ar x libjsoncpp.a
ar x libuuid.a
ar x libogg.a
ar x libopus.a
ar x libevent_core.a
ar x libevent_extra.a
ar x libevent_pthreads.a
### libcrypto.a和libevent_core.a中有.o文件重名, 释放在同目录会覆盖, 导致编译找不到定义. 比如buffer.o
mv $sdk_install_folder/tmp/buffer.o $sdk_install_folder/tmp/libevent_buffer.o

ar x libssl.a
ar x libcrypto.a
ar x libcurl.a

ar cr ../../../lib/libalibabacloud-idst-speech.a *.o
ranlib ../../../lib/libalibabacloud-idst-speech.a

my_arch=$( uname -m )


### 3
CFLAG_PARAMS="-Wl,-Bsymbolic -Wl,-Bsymbolic-functions -fPIC -fvisibility=hidden -Wl,--exclude-libs,ALL -Wl,-z,relro,-z,now -Wl,-z,noexecstack -fstack-protector"
LDFALG_PARAMS="-lpthread -lz -ldl -lanl"
if [ $ABI_FLAG == 1 ];then
  echo "share library with c++11"
  CFLAG_PARAMS="-std=c++11 $CFLAG_PARAMS -D_GLIBCXX_USE_CXX11_ABI=1"
  LDFALG_PARAMS="$LDFALG_PARAMS -lstdc++"
else
  CFLAG_PARAMS="$CFLAG_PARAMS -D_GLIBCXX_USE_CXX11_ABI=0"
fi
if [ $PRIVATE_FLAG == 1 ];then
  LDFALG_PARAMS="$LDFALG_PARAMS -lrt"
fi
echo "=> gcc -shared $CFLAG_PARAMS -o ../../../lib/libalibabacloud-idst-speech.so *.o $LDFALG_PARAMS"
gcc -shared $CFLAG_PARAMS -o ../../../lib/libalibabacloud-idst-speech.so *.o $LDFALG_PARAMS

if [ x${DEBUG_FLAG} == x"release" ];then
  strip ../../../lib/libalibabacloud-idst-speech.so
fi


### 4
echo "搬动头文件和库文件..."
mkdir -p $sdk_install_folder/include
cp $git_root_path/nlsCppSdk/framework/feature/sr/speechRecognizerRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/feature/st/speechTranscriberRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/feature/sy/speechSynthesizerRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/feature/da/dialogAssistantRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/feature/fss/flowingSynthesizerRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/item/iNlsRequest.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/common/nlsClient.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/common/nlsGlobal.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/framework/common/nlsEvent.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/token/include/nlsToken.h $sdk_install_folder/include/
cp $git_root_path/nlsCppSdk/token/include/FileTrans.h $sdk_install_folder/include/

mkdir -p $sdk_install_folder/lib
cp $build_folder/lib/* $sdk_install_folder/lib/


### 5
echo "生成DEMO..."
cd $demo_folder
cmake -DCXX11_ABI=${ABI_FLAG} ../../demo/Linux
make

echo "编译结束..."


### 6
cd $install_folder
mkdir -p $sdk_install_folder/demo
mkdir -p $sdk_install_folder/bin
cp $git_root_path/demo/Linux/profile_scan.cpp $sdk_install_folder/demo
cp $git_root_path/demo/Linux/generateTokenDemo.cpp $sdk_install_folder/demo
cp $git_root_path/demo/Linux/fileTransferDemo.cpp $sdk_install_folder/demo
cp $git_root_path/demo/Linux/dialogAssistantDemo.cpp $sdk_install_folder/demo
cp $git_root_path/demo/Linux/speechRecognizerDemo.cpp $sdk_install_folder/demo
cp $git_root_path/demo/Linux/speechSynthesizerDemo.cpp $sdk_install_folder/demo
cp $git_root_path/demo/Linux/speechTranscriberDemo.cpp $sdk_install_folder/demo
cp $git_root_path/demo/Linux/flowingSynthesizerDemo.cpp $sdk_install_folder/demo
cp $git_root_path/demo/Linux/*.h $sdk_install_folder/demo
cp $git_root_path/demo/Linux/build_linux_demo.sh $sdk_install_folder/demo
cp $git_root_path/version $sdk_install_folder/
cp $git_root_path/readme.md $sdk_install_folder/
cp $git_root_path/release.log $sdk_install_folder/
cp $git_root_path/build/demo/*Demo $sdk_install_folder/bin
cp -r $git_root_path/resource $sdk_install_folder/demo/
cp -r $git_root_path/resource/audio/test*.wav $sdk_install_folder/bin/
cur_date=$(date +%Y%m%d%H%M)

rm -rf $sdk_install_folder/tmp
tar_file=NlsSdk3.X_LINUX_$cur_date
echo "压缩 " $sdk_install_folder "到" $tar_file".tar.gz"
tar -zcPf $tar_file".tar.gz" NlsSdk3.X_LINUX

echo "打包结束..."

cp $audio_resource_folder/* $build_folder/demo/

### 7
mkdir -p $build_folder/demo/tests
cp -f $git_root_path/demo/Linux/run_functional_test.sh $build_folder/demo/tests/
chmod a+x $build_folder/demo/tests/run_functional_test.sh
cp -f $git_root_path/demo/Linux/run_pressure_test.sh $build_folder/demo/tests/
chmod a+x $build_folder/demo/tests/run_pressure_test.sh