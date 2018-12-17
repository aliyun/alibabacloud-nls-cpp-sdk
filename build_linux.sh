#!/bin/bash

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
cmake -DCMAKE_BUILD_TYPE=Release ..

make clean

make

make install

cd $install_folder

tar -zcvpf CppSdk2.X_LINUX.tar.gz CppSdk2.X

rm -rf CppSdk2.X

echo "编译结束..."

#echo "可以进入demo目录，执行命令[./demo <your appkey> <your AccessKey ID> <your AccessKey Secret>]，运行demo"
