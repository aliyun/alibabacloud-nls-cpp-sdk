# 阿里智能语音交互

欢迎使用阿里智能语音交互（C++ SDK）。

C++ SDK 提供一句话识别、实时语音识别、语音合成等服务。可应用于客服、法院智能问答等多个场景。

完成本文档中的操作开始使用 C++ SDK。

注意：SDK 采用 ISO标准C++ 编写。

## 前提条件

在使用 C++ SDK 前，确保您已经：

* 注册了阿里云账号并获取您的Access Key ID 和 Secret。

* 开通智能语音交互服务

* 创建项目, 获取AppKey

* 获取访问令牌（Access Token)

* Windows下请安装 Visual Studio 2013/2015 、Linux下请安装GCC 4.1.2 或以上版本

详细说明请参考:[智能语音交互接入](https://help.aliyun.com/document_detail/72138.html)


## 从源代码构建 SDK


- 依赖库

SDK 自带依赖库 openssl(l-1.0.2j)，opus(1.2.1)，jsoncpp(0.10.6)，uuid(1.0.3)，pthread(2.9.1)。外部依赖库放置在 path/to/sdk/lib 下。

注意：path/to/sdk/lib/linux/uuid仅在linux下使用。path/to/sdk/lib/windwos/1x.0/pthread仅在windows下使用。



- 构造项目

### Windows

进入 sdk 目录使用 Visual Studio 2013 打开 alibabacloud-sdk.sln 生成解决方案。
编译通过后，请根据实际位置设置 调试-命令参数 ( <your appkey> <your AccessKey ID> <your AccessKey Secret>)。

注意：在vs2015中打开项目，请在 预处理器 - 预处理定义 中添加定义 HAVE_STRUCT_TIMESPEC。否则编译错误。


### Linux

安装 cmake 3.1 或以上版本，进入 SDK 创建生成必要的构建文件

```
cd <path/to/sdk>
./build_linux.sh
cd build_linux_sdk/demo
./demo <your appkey> <your AccessKey ID> <your AccessKey Secret>
```

## 如何使用 C++ SDK

接入前请仔细阅读C++ SDK2.0文档：https://help.aliyun.com/product/30413.html


#### SDK压缩包说明:

- CMakeLists.txt demo工程的CMakeList文件
- build 编译目录
- demo/PublicCloud 包含demo.cpp，各语音服务配置文件。各文件描述见下表：

| 文件名  | 描述  |
| ------------ | ------------ |
| sdkDemo.cpp | windows专用，默认为一句话识别功能demo，如需可自行替换成其它功能(编码格式：UTF-8 代签名) |
| speechRecognizerDemo.cpp | 一句话异步识别demo |
| speechRecognizerSyncDemo.cpp | 一句话同步识别demo |
| speechSynthesizerDemo.cpp | 语音合成demo |
| speechTranscriberDemo.cpp | 实时语音异步识别demo |
| speechTranscriberSyncDemo.cpp | 实时语音同步识别demo |
| testX.wav | 测试音频 |


- include 包含sdk头文件，以及部分第三方头文件。各文件描述见下表

| 文件名  | 描述  |
| ------------ | ------------ |
| openssl |  openssl  |
| pthread | pthread线（windows下使用） |
| libuuid |  uuid(linux下使用)  |
| libjsoncpp |  json |
| curl | libcurl(nls SDK并不依赖curl，仅用于demo中，用以获取token) |
| nlsCommonSdknls | sdk并不依赖nlsCommonSdk，仅用于demo中，用以获取token |
| nlsClient.h | SDK实例  |
| nlsEvent.h | 回调事件说明  |
| speechRecognizerRequest.h | 一句哈识别  |
| speechSynthesizerRequest.h | 语音合成  |
| speechTranscriberRequest.h | 实时音频流识别  |

- lib
  包含sdk，以及第三方依赖库。其中根据平台不同，可以选择linux版本32/64位libnlscppsdk.so(glibc2.5及以上, Gcc4, Gcc5), windows版本32/64位nlscppsdk.dll（VS2013、VS2015）
- readme.txt SDK说明
- release.log 版本说明
- version 版本号
- build.sh demo编译脚本

注意：
1. linux环境下，运行环境最低要求：Glibc 2.5及以上， Gcc4、Gcc5
2. windows下，目前支持VS2013，VS2015


#### 依赖库：
SDK 依赖 openssl(l-1.0.2j)，opus(1.2.1)，jsoncpp(0.y.z)，uuid(1.0.3)，pthread(2.9.1)。依赖库放置在 path/to/sdk/lib 下。
注意：path/to/sdk/lib/linux/uuid仅在linux下使用。path/to/sdk/lib/windwos/1x.0/pthread仅在windows下使用。
      SDK压缩包内提供的依赖库为64位，不提供32位。在32位下，需要用户自行编译。


#### Linux下demo编译过程:
运行编译脚本build.sh：
1. 请确认本地系统以安装Cmake，最低版本3.1、Glibc 2.5、Gcc 4.1.2及以上。
2. cd path/to/sdk/lib
3. tar -zxvpf linux.tar.gz
4. cd path/to/sdk
5. 执行./build.sh 编译demo
6. 编译完毕，进入path/to/sdk/demo目录。可以看见以生成demo可执行程序：
   srDemo(一句话异步识别)、srSyncDemo(一句话同步识别)、stDemo(实时音频异步识别)、stSyncDemo(实时音频同步识别)、syDemo(语音合成)。
7. 执行[./demo <your appkey> <your AccessKey ID> <your AccessKey Secret>]

如果不支持cmake，可尝试手动编译:
1. cd path/to/sdk/demo
2. g++ -o srDemo speechRecognizerDemo.cpp -I../include -L../lib/linux -lnlsCppSdk -lnlsCommonSdk -lcurl -lssl -lcrypto -lopus -lpthread -luuid -ljsoncpp -D_GLIBCXX_USE_CXX11_ABI=0
3. export LD_LIBRARY_PATH=path/to/sdk/lib/linux/
4. 执行[./demo <your appkey> <your AccessKey ID> <your AccessKey Secret>]

#### windows平台：
暂时没有提供编译脚本，需要用户自己搭建


