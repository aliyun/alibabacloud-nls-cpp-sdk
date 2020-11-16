# 阿里智能语音交互

欢迎使用阿里智能语音交互（C++ SDK）。

C++ SDK 提供一句话识别、实时语音识别、语音合成等服务。可应用于客服、法院智能问答等多个场景。

注意：SDK 采用 ISO标准C++ 编写。

## 前提条件

在使用 C++ SDK 前，确保您已经：

* 注册了阿里云账号并获取您的Access Key ID 和 Secret。

* 开通智能语音交互服务

* 创建项目

* 获取访问令牌（Access Token)

* Windows下需支持VC12/VC14 、Linux下请安装GCC 4.1.2 或以上版本

详细说明请参考:[智能语音交互接入](https://help.aliyun.com/document_detail/72138.html)


## SDK压缩包说明:

- CMakeLists.txt demo工程的CMakeList文件
- demo 包含demo.cpp，各语音服务配置文件。各文件描述见下表：

| 文件名  | 描述  |
| ------------ | ------------ |
| speechRecognizerDemo.cpp | 一句话异步识别demo |
| speechRecognizerSyncDemo.cpp | 一句话同步识别demo |
| speechSynthesizerDemo.cpp | 语音合成demo |
| speechTranscriberDemo.cpp | 实时语音异步识别demo |
| speechTranscriberSyncDemo.cpp | 实时语音同步识别demo |
| test.wav | 测试音频 |

- include sdk头文件

| 文件名  | 描述  |
| ------------ | ------------ |
| nlsClient.h | SDK实例  |
| nlsEvent.h | 回调事件说明  |
| speechRecognizerRequest.h | 一句哈识别  |
| speechSynthesizerRequest.h | 语音合成  |
| speechTranscriberRequest.h | 实时音频流识别  |

- lib

    libalibabacloud-idst-speech.so 语音SDK(64位、glibc2.5及以上, Gcc4, Gcc5)

    alibabacloud-idst-speech.dll windows版本32/64位（VC12、VC14）

- demo/include Token SDK头文件

| 文件名  | 描述  |
| ------------ | ------------ |
| nlsCommonSdk | sdk并不依赖nlsCommonSdk，仅用于demo中，用以获取token |

- demo/lib

    Token SDK 用于获取Token


    libalibabacloud-idst-common.so (64位、glibc2.5及以上, Gcc4, Gcc5)


    alibabacloud-idst-common.dll windows版本32/64位（VC12、VC14）

- readme.md SDK说明
- release.log 版本说明
- version 版本号


## Linux下demo编译过程:
cmake编译：
运行编译脚本build.sh：
1. 请确认本地系统以安装Cmake，最低版本3.1、Glibc 2.5、Gcc 4.1.2及以上。
2. cd path/to/sdk/
3. mkdir build
4. cd build
5. cmake ..
6. make
6. cd path/to/sdk/demo 可以看见以生成demo可执行程序：
   srDemo(一句话异步识别)、srSyncDemo(一句话同步识别)、stDemo(实时音频异步识别)、stSyncDemo(实时音频同步识别)、syDemo(语音合成)。
7. ./demo (Appkey)  (AccessKeyId) (AccessKey Secret)

注意：
如果使用libalibabacloud-idst-speech.a编译报缺少gzclose缺失，请在CMakeLists.txt，或编译命令中加 -lrt -lz


## Windows下demo编译过程
1: 请确认本地系统安装VS2013或以上
2: 进入path/to/sdk/
3: 双击 idst-speech.sln
4: 编译运行
