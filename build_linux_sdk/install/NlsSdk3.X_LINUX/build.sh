#!/bin/bash

build_folder=$PWD/build

echo "建立编译目录..."

if [ ! -d $build_folder ];then
    mkdir $build_folder
else
    echo "编译目录build已存在..."
fi

rm -rf $build_folder/*

echo "进入编译目录build..."

cd $build_folder

echo "开始编译..."

cmake ../

make clean

make

echo "编译结束..."

echo "可以进入demo目录，执行命令[./demo  <your appkey> <your AccessKey ID> <your AccessKey Secret>]，运行demo"


