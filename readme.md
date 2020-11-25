# 阿里智能语音交互

欢迎使用阿里智能语音交互（C++ SDK）。

C++ SDK 提供一句话识别、实时语音识别、语音合成等服务。可应用于客服、法院智能问答等多个场景。


## 前提条件

在使用 C++ SDK 前，确保您已经：

* 注册了阿里云账号并获取您的Access Key ID 和 Secret。

* 开通智能语音交互服务

* 创建项目

* 获取访问令牌（Access Token)

* Windows下需支持VC12/VC14 、Linux下请安装GCC 4.1.2 或以上版本

详细说明请参考:[智能语音交互接入](https://help.aliyun.com/document_detail/72138.html)


## 从源代码构建 SDK


- 依赖库

SDK 依赖库 openssl(l-1.1.0j)，opus(1.2.1)，jsoncpp(0.10.6)，uuid(1.0.3)，libevent(2.1.8)。
外部依赖库源码放置在 path/to/sdk/common 下。

- 构造项目

Windows: build_windows.bat


Linux: build_linux.sh (安装 cmake 3.1 或以上版本)


## 如何使用 C++ SDK

接入前请仔细阅读C++ SDK2.0文档：https://help.aliyun.com/product/30413.html


目录说明:

- CMakeLists.txt CMakeList文件
- demo 包含demo.cpp，各语音测试音频文件
- common 第三方依赖库源码
- lib  已经编译完成的第三方依赖库
- nlsCppSdk SDK源码
- readme.md SDK说明
- release.log 版本说明
- version 版本号
- build_***.sh 编译脚本

注意：
1. linux环境下，运行环境最低要求：Glibc 2.5及以上， Gcc4、Gcc5
2. windows下，目前支持VC12，VC14
3. 目前windows编译实例为VC14环境
4. 目录说明：alibabacloud-nls-cpp-sdk/lib/windows已经编译好的库目录。源码编译的时候需要将对应目录中nlsCommonSdk.dll、nlsCommonSdk.lib、测试语音拷贝到编译目录。
5. alibabacloud-nls-cpp-sdk/demo/Win32_Demo 存放windows测试demo代码。单独导入即可
6. 需要配置运行参数 ./demo.exe <your appkey> <your AccessKey ID> <your AccessKey Secret>







