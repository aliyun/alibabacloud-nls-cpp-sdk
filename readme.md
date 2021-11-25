# 阿里智能语音交互

欢迎使用阿里智能语音交互（C++ SDK）。

C++ SDK 提供一句话识别、实时语音识别、语音合成等服务。可应用于客服、法院智能问答等多个场景。  


## 前提条件

在使用 C++ SDK 前，确保您已经：

* 注册了阿里云账号并获取您的Access Key ID 和 Secret。

* 开通智能语音交互服务

* 创建项目

* 获取访问令牌（Access Token)

* Linux下请安装GCC 4.1.2 或以上版本

详细说明请参考:[智能语音交互接入](https://help.aliyun.com/document_detail/72138.html)  


## 如何使用 C++ SDK

接入前请仔细阅读C++ SDK3.0文档：https://help.aliyun.com/product/30413.html

特殊说明：当前版本C++SDK3.1.x，相较于3.0，接口sendAudio有改动，具体请看接口头文件中接口说明。  

### Linux平台编译及说明：
编译指令：  
> ./scripts/build_linux.sh                         默认增量编译，生成Debug版本  
> ./scripts/build_linux.sh all debug        全量编译，生成Debug版本  
> ./scripts/build_linux.sh incr debug     增量编译，生成Debug版本  
> ./scripts/build_linux.sh all release      全量编译，生成Release版本  
> ./scripts/build_linux.sh incr release    增量编译，生成Release版本   

生成物NlsSdk3.X_LINUX 目录说明:  
NlsSdk3.X_LINUX  
├── bin  
│   ├── daDemo         对话Demo binary文件  
│   ├── srDemo         一句话识别Demo binary文件  
│   ├── stDemo         实时识别Demo binary文件  
│   └── syDemo         音频转写Demo binary文件  
├── demo  
│   ├── build_linux_demo.sh          一键编译当前Demo  
│   ├── dialogAssistantDemo.cpp      对话Demo源码  
│   ├── speechRecognizerDemo.cpp     一句话识别Demo源码  
│   ├── speechSynthesizerDemo.cpp    音频转写Demo源码  
│   └── speechTranscriberDemo.cpp    实时识别Demo源码  
│   ├── resource            测试资源（测试音频文件）  
│   │   └── audio  
│   │       ├── test0.wav  
│   │       ├── test1.wav  
│   │       ├── test2.wav  
│   │       └── test3.wav  
├── include                 接口头文件  
│   ├── iNlsRequest.h  
│   ├── nlsClient.h  
│   ├── nlsEvent.h  
│   ├── nlsGlobal.h  
│   ├── nlsToken.h  
│   ├── dialogAssistantRequest.h  
│   ├── speechRecognizerRequest.h  
│   ├── speechSynthesizerRequest.h  
│   └── speechTranscriberRequest.h  
├── lib                     库（原libalibabacloud-idst-common.so已合并入libalibabacloud-idst-speech.so）  
│   ├── libalibabacloud-idst-speech.a  
│   └── libalibabacloud-idst-speech.so  
├── README.md  
└── version                 版本说明  

注意：
1. linux环境下，运行环境最低要求：Glibc 2.5及以上， Gcc4及以上。  
2. linux环境下，高并发运行，注意 系统打开文件数限制，可通过ulimit -a查看当前允许的打开文件数限制。比如预设最大并发数1000，建议将open files限制设置大于1000，ulimit -n 2000。否则会出现connect failed错误。  

### 嵌入式(eg. arm-linux等)平台编译及说明：
> 后续将提供常见arm编译范例，客户可根据实际需求修改工具链完成交叉编译。  

### Windows平台编译及说明：
> 开发中...  



