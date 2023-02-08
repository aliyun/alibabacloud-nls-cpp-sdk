/*
 * Copyright 2021 Alibaba Group Holding Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NLS_SDK_GLOBAL_H
#define NLS_SDK_GLOBAL_H

#if defined(_MSC_VER)

  #define NLS_SDK_DECL_EXPORT __declspec(dllexport)
  #define NLS_SDK_DECL_IMPORT __declspec(dllimport)

  #if defined(_NLS_SDK_SHARED_)
    #define NLS_SDK_CLIENT_EXPORT NLS_SDK_DECL_EXPORT
  #else
    #define NLS_SDK_CLIENT_EXPORT NLS_SDK_DECL_IMPORT
  #endif

  #define NLS_EXTERN_C extern "C"
  #define NLS_EXPORTS NLS_SDK_DECL_EXPORT
  #define NLS_CDECL __cdecl
  #define NLSAPI(rettype) NLS_EXTERN_C NLS_EXPORTS rettype NLS_CDECL

  typedef int (NLS_CDECL * NlsCallbackDelegate)(void*);

#else

  #if defined(_NLS_SDK_SHARED_)
    #define NLS_SDK_CLIENT_EXPORT __attribute__((visibility("default")))
  #else
    #define NLS_SDK_CLIENT_EXPORT
  #endif

#endif

enum ENCODER_TYPE {
  ENCODER_NONE = 0,
  ENCODER_OPUS,
  ENCODER_OPU,
};

enum NlsRetCode {
  Success = 0,

  /* common */
  DefaultError = 10,            /* 默认错误 */
  JsonParseFailed,              /* 错误的Json格式 */
  JsonObjectError,              /* 错误的Json对象 */
  MallocFailed,                 /* Malloc失败 */
  ReallocFailed,                /* Realloc失败 */
  InvalidInputParam,            /* 传入无效的参数 */


  /* log */
  InvalidLogLevel = 50,         /* 无效日志级别 */
  InvalidLogFileSize,           /* 无效日志文件大小 */
  InvalidLogFileNum,            /* 无效日志文件数量 */


  /* encoder */
  EncoderExistent = 100,        /* NLS的编码器已存在 */
  EncoderInexistent,            /* NLS的编码器不存在 */
  OpusEncoderCreateFailed,      /* Opus编码器创建失败 */
  OggOpusEncoderCreateFailed,   /* OggOpus编码器创建失败 */


  /* nls client */
  EventClientEmpty = 150,       /* 主工作线程空指针, 已释放 */
  SelectThreadFailed,           /* 工作线程选择失败, 未初始化 */
  StartCommandFailed = 160,     /* 发送start命令失败 */
  InvokeStartFailed,            /* 请求状态机不对, 导致start失败 */
  InvokeSendAudioFailed,        /* 请求状态机不对, 导致sendAudio失败 */
  InvalidOpusFrameSize,         /* opus帧长无效, 默认为640字节 */
  InvokeStopFailed,             /* 请求状态机不对, 导致stop失败 */
  InvokeCancelFailed,           /* 请求状态机不对, 导致stop失败 */
  InvokeStControlFailed,        /* 请求状态机不对, 导致stControl失败 */


  /* nls event */
  NlsEventEmpty = 200,          /* NLS事件为空 */
  NewNlsEventFailed,            /* 创建NlsEvent失败 */
  NlsEventMsgEmpty,             /* NLS事件中消息为空 */
  InvalidNlsEventMsgType,       /* 无效的NLS事件中消息类型 */
  InvalidNlsEventMsgStatusCode, /* 无效的NLS事件中消息状态码 */
  InvalidNlsEventMsgHeader,     /* 无效的NLS事件中消息头 */


  /* work thread */
  CancelledExitStatus = 250,    /* 已调用cancel */
  InvalidWorkStatus,            /* 无效的工作状态 */
  InvalidNodeQueue,             /* workThread中NodeQueue无效 */


  /* request */
  InvalidRequestParams = 300,   /* 请求的入参无效 */
  RequestEmpty,                 /* 请求是空指针 */
  InvalidRequest,               /* 无效的请求 */
  SetParamsEmpty,               /* 设置传入的参数为空 */


  /* websocket */
  GetHttpHeaderFailed = 350,    /* 获得http头失败 */
  HttpGotBadStatus,             /* http错误的状态 */
  WsResponsePackageFailed,      /* 解析websocket返回包失败 */
  WsResponsePackageEmpty,       /* 解析websocket返回包为空 */
  WsRequestPackageEmpty,        /* websocket请求包为空 */
  UnknownWsFrameHeadType,       /* 未知websocket帧头类型 */
  InvalidWsFrameHeaderSize,     /* 无效的websocket帧头大小 */
  InvalidWsFrameHeaderBody,     /* 无效的websocket帧头本体 */
  InvalidWsFrameBody,           /* 无效的websocket帧本体 */
  WsFrameBodyEmpty,             /* 帧数据为空, 常见为收到了脏数据 */

  /* connect node */
  NodeEmpty = 400,              /* node为空指针 */
  InvaildNodeStatus,            /* node所处状态无效 */
  GetAddrinfoFailed,            /* 通过dns解析地址识别 */
  ConnectFailed,                /* 联网失败 */
  InvalidDnsSource,             /* 当前设备无DNS */
  ParseUrlFailed,               /* 无效url */

  SslHandshakeFailed,           /* SSL握手失败 */
  SslCtxEmpty,                  /* SSL_CTX未空 */
  SslNewFailed,                 /* SSL_new失败 */
  SslSetFailed,                 /* SSL设置参数失败 */
  SslConnectFailed,             /* SSL_connect失败 */
  SslWriteFailed,               /* SSL发送数据失败 */
  SslReadSysError,              /* SSL接收数据收到SYSCALL错误 */
  SslReadFailed,                /* SSL接收数据失败 */

  SocketFailed,                 /* 创建socket失败 */
  SetSocketoptFailed,           /* 设置socket参数失败 */
  SocketConnectFailed,          /* 进行socket链接失败 */
  SocketWriteFailed,            /* socket发送数据失败 */
  SocketReadFailed,             /* socket接收数据失败 */

  NlsReceiveFailed = 430,       /* NLS接收帧数据失败 */
  NlsReceiveEmpty,              /* NLS接收帧数据为空 */
  ReadFailed,                   /* 接收数据失败 */
  NlsSendFailed,                /* NLS发送数据失败 */
  NewOutputBufferFailed,        /* 创建buffer失败 */
  NlsEncodingFailed,            /* 音频编码失败 */
  EventEmpty,                   /* event为空 */
  EvbufferTooMuch,              /* evbuffer中数据太多 */
  EvutilSocketFalied,           /* evutil设置参数失败 */
  InvalidExitStatus,            /* 无效的退出状态 */


  /* token */
  InvalidAkId = 450,            /* 阿里云账号ak id无效 */
  InvalidAkSecret,              /* 阿里云账号ak secret无效 */
  InvalidAppKey,                /* 项目appKey无效 */
  InvalidDomain,                /* domain无效 */
  InvalidAction,                /* action无效 */
  InvalidServerVersion,         /* ServerVersion无效 */
  InvalidServerResource,        /* ServerResource无效 */
  InvalidRegionId,              /* RegionId无效 */


  /* file transfer */
  InvalidFileLink = 500,        /* 无效的录音文件链接 */
  ErrorStatusCode,              /* 错误的状态码 */
  IconvOpenFailed,              /* 申请转换描述失败 */
  IconvFailed,                  /* 编码转换失败 */
  ClientRequestFaild,           /* 账号客户端请求失败 */

  /* 900 - 998 reserved for C# */

  NlsMaxErrorCode = 999,
};

enum NlsErrorCode {
  /*
   * msg: SSL: couldn't create a context!
   * solution: 建议重新初始化。
   */
  NewSslCtxFailed = 10000001,

  /*
   * msg: return of SSL_read: error:00000000:lib(0):func(0):reason(0)
   * solution: 建议重新尝试。
   *
   * msg: return of SSL_read: error:140E0197:SSL routines:SSL_shutdown:shutdown while in init
   * solution: 建议重新尝试。
   */
  DefaultErrorCode = 10000002,

  SysErrorCode = 10000003,

  /*
   * msg: URL: The url is empty.
   * solution: 传入的URL为空, 请重新填写正确URL。
   */
  EmptyUrl = 10000004,

  /*
   * msg: Could not parse WebSocket url:
   * solution: 传入的URL格式错误, 请重新填写正确URL。
   */
  InvalidWsUrl = 10000005,

  /*
   * msg: JSON: Json parse failed.
   * solution: Json格式异常, 请通过日志查看具体的错误点。
   */
  JsonStringParseFailed = 10000007,

  /*
   * msg: WEBSOCKET: unkown head type.
   * solution: 联网失败,请检查本机dns解析和URL是否有效。
   */
  UnknownWsHeadType = 10000008,

  /*
   * msg: HTTP: connect failed.
   * solution: 与云端连接失败,请检查网络后,重试。
   */
  HttpConnectFailed = 10000009,

  /*
   * msg:
   * solution: 请检查内存是否充足。
   */
  MemNotEnough = 10000010,

  /*
   * msg: connect failed.
   * solution: 联网失败,请检查本机dns解析和URL是否有效。
   */
  SysConnectFailed = 10000015,


  /*
   * msg: Got bad status host=xxxxx line=HTTP/1.1 403 Forbidden
   * solution: 链接被拒,请检查账号特别是token是否过期。
   */
  HttpGotBadStatusWith403 = 10000100,

  /*
   * msg: Send timeout. socket error:
   * solution: libevent发送event超时,请检查回调中是否有耗时任务,或并发过大导致无法及时处理事件。
   */
  EvSendTimeout = 10000101,

  /*
   * msg: Recv timeout. socket error:
   * solution: libevent接收event超时,请检查回调中是否有耗时任务,或并发过大导致无法及时处理事件。
   */
  EvRecvTimeout = 10000102,

  /*
   * msg: Unknown event:
   * solution: 未知的libevent事件,建议重新尝试。
   */
  EvUnknownEvent = 10000103,

  /*
   * msg: Operation now in progress
   * solution: 链接正在进行中,建议重新尝试。
   */
  OpNowInProgress = 10000104,

  /*
   * msg: Broken pipe
   * solution: pipe处理不过来,建议重新尝试。
   */
  BrokenPipe = 10000105,

  /*
   * msg: Gateway:ACCESS_DENIED:The token 'xxx' has expired!
   * solution: 请更新token。
   */
  TokenHasExpired = 10000110,

  /*
   * msg: Meta:ACCESS_DENIED:The token 'xxx' is invalid!
   * solution: 请检查token的有效性。
   */
  TokenIsInvalid = 10000111,

  /*
   * msg: Gateway:ACCESS_DENIED:No privilege to this voice! (voice: zhinan, privilege: 0)
   * solution: 此发音人无权使用。
   */
  NoPrivilegeToVoice = 10000112,

  /*
   * msg: Gateway:ACCESS_DENIED:Missing authorization header!
   * solution: 请检查账号是否有权限,或并发是否在限度内。
   */
  MissAuthHeader = 10000113,

  /*
   * msg: utf8ToGbk failed
   * solution: utf8转码失败,常为系统问题,建议重新尝试。
   */
  Utf8ConvertError = 10000120,

  SuccessStatusCode = 20000000,


  // get error code from server status code
  /*
   * get status code(40000000) from SERVICE
   * msg: [tts:atom-offline]Client disconnected!
   * solution: 请再次尝试语音交互请求。
   *
   * msg: Gateway:CLIENT_ERROR:in post
   * solution: 请再次尝试语音交互请求。
   *
   * msg: text is empty.
   * solution: 传入的语音合成text为空。
   */
  ClientError = 40000000,

  /*
   * get status code(40000001) from SERVICE
   * msg: Meta:ACCESS_DENIED:The token 'xxx' is invalid!
   * solution: 请检查token的有效性。
   *
   * msg: Gateway:ACCESS_DENIED:No privilege to this voice! (voice: zhinan, privilege: 0)
   * solution: 此发音人无权使用。
   *
   * msg: Gateway:ACCESS_DENIED:Missing authorization header!
   * solution: 请检查账号是否有权限,或并发是否在限度内。
   */
  AccessDenied = 40000001,

  /*
   * get status code(40000002) from SERVICE
   * msg: Gateway:MESSAGE_INVALID:Invalid message id 'null'!
   * solution: 检查发送的消息是否符合要求。
   * 
   * msg: Gateway:MESSAGE_INVALID:Missing message header!"
   * solution:检查发送的消息是否符合要求。
   */
  MessageInvalid = 40000002,

  /*
   * get status code(40000003) from SERVICE
   * msg: Gateway:PARAMETER_INVALID:appkey not set
   * solution: 参数无效,appkey未设置。
   *
   * msg: Gateway:PARAMETER_INVALID:Invalid voice name 'xxx'!
   * solution: 参数无效,发音人无效。
   *
   * msg: Gateway:PARAMETER_INVALID:Invalid appkey
   * solution: 参数无效,appkey无效。
   */
  ParameterInvalid = 40000003,

  /*
   * get status code(40000004) from SERVICE
   * msg: Gateway:IDLE_TIMEOUT:Websocket session is idle for too long time!
   * solution: 长时间未发送指令。
   */
  SessionIdleTooLongTime = 40000004,

  /*
   * get status code(40000005) from SERVICE
   * msg: Gateway:TOO_MANY_REQUESTS:Too many requests!
   * solution: 请求数量过多,检查是否超过了并发连接数或者每秒钟请求数。
   */
  TooManyRequests = 40000005,

  /*
   * get status code(40000009) from SERVICE
   * msg: Too many opus packets per page: max=100, actual=101
   * solution: 请求数量过多,检查是否超过了并发连接数或者每秒钟请求数。
   */
  TooManyOpusPackets = 40000009,

  /*
   * get status code(40000010) from SERVICE
   * msg: Gateway:FREE_TRIAL_EXPIRED:The free trial has expired!
   * solution: 免费试用已过期,请开通商用。
   */
  FreeTrialExpired = 40000010,

  /*
   * get status code(40010001) from SERVICE
   * msg: Gateway:NAMESPACE_NOT_FOUND:RESTful url path illegal!
   * solution: 请检查传入的url。
   */
  NamespaceNotFound = 40010001,

  /*
   * get status code(40010003) from SERVICE
   * msg: Gateway:DIRECTIVE_INVALID:No text specified!
   * solution: 无效的语音合成text。
   *
   * msg: Gateway:DIRECTIVE_INVALID:Invalid format 'PCM'!
   * solution: 无效的PCM格式,请检查此时的数据格式是否为PCM。
   *
   * msg: Gateway:DIRECTIVE_INVALID:Invalid payload for event 'SpeechLongSynthesizer.StartSynthesis'!
   * solution: 无效的payload。
   */
  DirectiveInvalid = 40010003,

  /*
   * get status code(40010005) from SERVICE
   * msg: Gateway:TASK_STATE_ERROR:State is not WORKING, empty body data or appkey illegal, can not handle last content!
   * solution: 请再次尝试语音交互请求。
   */
  TaskStateError = 40010005,

  /*
   * get status code(40020105) from SERVICE
   * msg: Meta:APPKEY_NOT_EXIST:Appkey not exist!
   * solution: appkey不存在, 是否与传入的token归属同一账号。
   */
  AppkeyNotExist = 40020105,

  /*
   * get status code(40020106) from SERVICE
   * msg: Meta:APPKEY_UID_MISMATCH:Appkey and user mismatch!
   * solution: 请检查appkey和账号是否一致。
   */
  AppkeyUidMismatch = 40020106,

  /*
   * get status code(40270002) from SERVICE
   * msg: Silent speech
   * solution: 传入的是无人声语音。
   */
  SilentSpeech = 40270002,

  /*
   * get status code(41010100) from SERVICE
   * msg: UNSUPPORTED_FORMAT
   * solution: 请检查入参。
   */
  UnsupportedFormat = 41010100,

  /*
   * get status code(41010104) from SERVICE
   * msg: TOO_LONG_SPEECH
   * solution: 实时语音识别太长(超过2分半), 请适时断句再启动识别。
   */
  TooLongSpeech = 41010104,

  /*
   * get status code(41020001) from SERVICE
   * msg: TTS:TtsClientError:SetVoice RetCode[2] invalid voice name
   * solution: 无效的发音人。
   *
   * msg: TTS:TtsClientError:Illegal ssml text
   * solution: 无效的ssml格式text。
   */
  TtsClientError = 41020001,

  /*
   * get status code(41050002) from SERVICE
   * msg: FILE_DOWNLOAD_FAILED
   * solution: 
   */
  FileDownloadFailed = 41050002,

  /*
   * get status code(50000000) from SERVICE
   * msg: Gateway:SERVER_ERROR:Server error!
   * msg: Gateway:SERVER_ERROR:No endpoint found for service 'jupiter-flow'
   * msg: Realtime:SERVER_ERROR:Instance pool exhausted!
   * solution: 请再次尝试语音交互请求。
   */
  ServerError = 50000000,

  /*
   * get status code(50000001) from SERVICE
   * msg: Gateway:GRPC_ERROR:Grpc error!
   * solution: 请再次尝试语音交互请求。
   */
  GrpcError = 50000001,

  /*
   * get status code(50020003) from SERVICE
   * msg: Meta:ROUTE_GROUP_NOT_FOUND:Route group 'xxx' not found!
   * solution: route group不存在,请再次尝试语音交互请求。
   */
  RouteGroupNotFound = 50020003,

  /*
   * get status code(51020001) from SERVICE
   * msg: TTS:TtsServerError:[flow]Instance pool exhausted!
   * solution: 请再次尝试语音交互请求。
   *
   * msg: TTS:TtsServerError:SetVoice RetCode[2] invalid voice name
   * solution: 请检查发音人是否有效。
   *
   * msg: TTS:TtsServerError:[tts]Failed to invoke 'jai_stream_producer_start_customized'
   * solution: 请检查语音合成参数是否有效。
   */
  TtsServerError = 51020001,

};

#endif // NLS_SDK_GLOBAL_H
