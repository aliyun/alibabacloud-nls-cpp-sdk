# 阿里智能语音交互

欢迎使用阿里智能语音交互（C++ SDK）。

C++ SDK 提供一句话识别、实时语音识别、语音合成等服务。可应用于客服、法院智能问答等多个场景。

完成本文档中的操作开始使用 C++ SDK。

注意：SDK 采用 ISO标准C++ 编写。

## 前提条件

在使用 C++ SDK 前，确保您已经：

* 注册了阿里云账号并获取您的Access Key ID 和 Secret。

* 开通智能语音交互服务

* 创建项目

* 获取访问令牌（Access Token)

* Windows下请安装 Visual Studio 2013/2015 、Linux下请安装GCC 4.1.2 或以上版本

详细说明请参考:[智能语音交互接入](https://help.aliyun.com/document_detail/72138.html)


## 从源代码构建 SDK


- 依赖库

SDK 自带依赖库 openssl(l-1.0.2j)，opus(1.2.1)，jsoncpp(0.y.z)，uuid(1.0.3)，pthread(2.9.1)。其中jsoncpp以源码形式集成在SDK中。外部依赖库放置在 path/to/sdk/lib 下。

注意：path/to/sdk/lib/linux/uuid仅在linux下使用。path/to/sdk/lib/windwos/1x.0/pthread仅在windows下使用。



- 构造项目

### Windows

进入 sdk 目录使用 Visual Studio 2013 打开 alibabacloud-sdk.sln 生成解决方案。
编译通过后，请根据实际位置设置 调试-命令参数 (path/to/config.txt your-access-token)。

注意：在vs2015中打开项目，请在 预处理器 - 预处理定义 中添加定义 HAVE_STRUCT_TIMESPEC。否则编译错误。


### Linux

安装 cmake 2.6 或以上版本，进入 SDK 创建生成必要的构建文件

```
cd <path/to/sdk>
mkdir build
cd build
cmake ../
make clean
make
cd demo
./demo path/to/config.txt your-access-token
```

## 如何使用 C++ SDK

请参照 path/to/sdk/demo 目录。demo 文件夹包含：

| 文件名  | 描述  |
| ------------ | ------------ |
| sdkDemo.cpp | VS专用，默认为一句话识别功能demo，如需可自行替换 (编码格式：UTF-8 代签名) |
| config-speechRecognizer.txt |  一句话识别配置文件 |
| config-speechSynthesizer.txt | 语音合成配置文件 |
| config-speechTranscriber.txt | 实时音频流识别配置文件  |
|  speechRecognizerDemo.cpp | 一句话识别demo  |
|  speechSynthesizerDemo.cpp | 语音合成demo  |
|  speechTranscriberDemo.cpp | 实时音频流识别demo  |

