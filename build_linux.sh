#!/bin/bash -e

build_folder=$PWD/build_linux_sdk
if [ ! -d $build_folder ];then
    mkdir -p $build_folder
fi

rm -rf $build_folder/*
echo "建立IOS编译目录:"$build_folder

common_folder=$build_folder/common
echo "建立common目录:" $common_folder
mkdir -p $common_folder

install_folder=$build_folder/install
echo "建立install目录:" $install_folder
mkdir -p $install_folder

cd $build_folder

#echo "开始编译..."
#cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../toolchain/linux.toolchain.cmake  ..

cmake -DCMAKE_BUILD_TYPE=Release ..

make clean

make

make install

cd $install_folder

echo "install目录:" $PWD

##mv NlsSdk2.X_LINUX/lib64 NlsSdk2.X_LINUX/lib

##mv NlsSdk2.X_LINUX/lib/libnlsCppSdk-a.a NlsSdk2.X_LINUX/lib/libnlsCppSdk.a

sdk_install_folder=NlsSdk3.X_LINUX

rm -rf $sdk_install_folder/demo/PrivateCloud
rm -rf $sdk_install_folder/demo/PublicCloud
rm -rf $sdk_install_folder/demo/lib/gtest

echo "Build文件:" $sdk_install_folder/build.sh
chmod 777 $sdk_install_folder/build.sh

cd $sdk_install_folder/tmp/

ar x libalibabacloud-idst-speech.a
ar x libssl.a
ar x libuuid.a
ar x libcrypto.a
ar x libopus.a
ar x libevent_core.a
ar x libevent_extra.a
ar x libevent_pthreads.a
ar x liblog4cpp.a
ar x libjsoncpp.a

if [ -f libvsclient.a ]; then
    ar x libvsclient.a
fi

ar cru ../lib/libalibabacloud-idst-speech.a *.o
ranlib ../lib/libalibabacloud-idst-speech.a
#strip ../lib/libalibabacloud-idst-speech.a

cd $install_folder
rm -rf $sdk_install_folder/tmp
tar -zcvpf $sdk_install_folder.tar.gz $sdk_install_folder

rm -rf $sdk_install_folder

echo "编译结束..."

#echo "可以进入demo目录，执行命令[./demo <your appkey> <your AccessKey ID> <your AccessKey Secret>]，运行demo"
