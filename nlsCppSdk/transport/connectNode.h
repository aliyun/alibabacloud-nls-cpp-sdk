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

#ifndef NLS_SDK_CONNECT_NODE_H
#define NLS_SDK_CONNECT_NODE_H

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <queue>
#include <string>
#include <stdint.h>
#include "nlsEncoder.h"
#include "error.h"
#include "webSocketTcp.h"
#include "webSocketFrameHandleBase.h"
#include "SSLconnect.h"
#include "nlsClient.h"
#include "nlsGlobal.h"

#include "event2/util.h"
#include "event2/dns.h"
#include "event2/buffer.h"
#include "event.h"

namespace AlibabaNls {

class INlsRequest;
class WorkThread;
class NlsEventNetWork;

#define CONNECT_TIMER_INVERVAL_MS 20
#define RETRY_CONNECT_COUNT       4
#define SAMPLE_RATE_16K           16000
#define SAMPLE_RATE_8K            8000
#define BUFFER_16K_MAX_LIMIT      320000
#define BUFFER_8K_MAX_LIMIT       160000

#if defined(_MSC_VER)

#define NLS_ERR_IS_EAGAIN(e) ((e) == WSAEWOULDBLOCK || (e) == EAGAIN)
#define NLS_ERR_RW_RETRIABLE(e) ((e) == WSAEWOULDBLOCK || (e) == WSAEINTR)
#define NLS_ERR_CONNECT_RETRIABLE(e) ((e) == WSAEWOULDBLOCK || (e) == WSAEINTR || (e) == WSAEINPROGRESS || (e) == WSAEINVAL)
#define NLS_ERR_ACCEPT_RETRIABLE(e) EVUTIL_ERR_RW_RETRIABLE(e)
#define NLS_ERR_CONNECT_REFUSED(e) ((e) == WSAECONNREFUSED)

#else

#define INVALID_SOCKET -1
#if EAGAIN == EWOULDBLOCK
  #define NLS_ERR_IS_EAGAIN(e) ((e) == EAGAIN)
#else
  #define NLS_ERR_IS_EAGAIN(e) ((e) == EAGAIN || (e) == EWOULDBLOCK)
 #endif /*EAGAIN == EWOULDBLOCK*/
/* True iff e is an error that means a read/write operation can be retried. */
#define NLS_ERR_RW_RETRIABLE(e) ((e) == EINTR || NLS_ERR_IS_EAGAIN(e))
/* True iff e is an error that means an connect can be retried. */
#define NLS_ERR_CONNECT_RETRIABLE(e)  ((e) == EINTR || (e) == EINPROGRESS)
/* True iff e is an error that means a accept can be retried. */
#define NLS_ERR_ACCEPT_RETRIABLE(e) ((e) == EINTR || NLS_ERR_IS_EAGAIN(e) || (e) == ECONNABORTED)
/* True iff e is an error that means the connection was refused */
#define NLS_ERR_CONNECT_REFUSED(e) ((e) == ECONNREFUSED)

#endif /*#if defined(_MSC_VER)*/

#define CONNECT_FAILED_CODE 10000001
#define TASK_FAILED_CODE 10000002
#define SEND_FAILED_CODE 10000003
#define RECV_FAILED_CODE 10000004
#define CLOSE_CODE 20000000

#define CLOSE_JSON_STRING              "{\"channelClosed\": \"nls request finished.\"}"
#define TASKFAILED_CONNECT_JSON_STRING "connect failed."
#define TASKFAILED_PARSE_JSON_STRING   "{\"TaskFailed\": \"JSON: Json parse failed.\"}"
#define TASKFAILED_NEW_NLSEVENT_FAILED "{\"TaskFailed\": \"new NlsEvent failed, memory is not enough.\"}"
#define TASKFAILED_UTF8_JSON_STRING    "{\"TaskFailed\": \"utf8ToGbk failed.\"}"
#define TASKFAILED_WS_JSON_STRING      "{\"TaskFailed\": \"WEBSOCKET: unkown head type.\"}"
#define TASKFAILED_ERROR_CLOSE_STRING  "{\"TaskFailed\": \"WEBSOCKET: invalid closeCode of wsFrame.\"}"


/* EventNetWork发送Node的具体指令 */
enum CmdType {
  CmdStart = 0,
  CmdStop,
  CmdStControl,
  CmdTextDialog,
  CmdExecuteDialog,
  CmdWarkWord,
  CmdCancel
};

/* Node处于的退出状态 */
enum ExitStatus {
  ExitInvalid = 0, /* 构造时, 未处于退出状态 */
  ExitStopping,    /* 调用stop时设置ExitStopping */
  ExitCancel,      /* 调用cancel时设置ExitCancel */
};

/* Node处于的最新运行状态 */
enum ConnectStatus {
  NodeInvalid = 0, /* node处于不可用或者释放状态 */
  NodeCreated,     /* 构造node */
  NodeInvoking,    /* 刚调用start的过程, 向notifyEventCallback发送c指令 */
  NodeInvoked,     /* 调用start的过程, 在notifyEventCallback完成 */
  NodeConnecting,  /* 正在dns解析 */
  NodeConnected,   /* socket链接成功 */
  NodeHandshaking, /* ssl握手中 */
  NodeHandshaked,  /* 握手成功 */
  NodeStarting,    /* 握手后收到response, 开始工作 */
  NodeStarted,     /* 收到started response时 */
  NodeWakeWording = 10,
  NodeFailed,
  NodeCompleted,
  NodeClosed,
  NodeReleased,

  NodeStop,
  NodeCancel,
  NodeSendAudio,
  NodeSendControl,
  NodePlayAudio,
};

#ifdef ENABLE_REQUEST_RECORDING
/* Node运行过程记录 */
struct NodeProcess {
public:
  explicit NodeProcess() {
    create_timestamp_ms = 0;
    start_timestamp_ms = 0;
    started_timestamp_ms = 0;
    stop_timestamp_ms = 0;
    cancel_timestamp_ms = 0;
    first_binary_timestamp_ms = 0;
    last_send_timestamp_ms = 0;
    last_ctrl_timestamp_ms = 0;
    failed_timestamp_ms = 0;
    completed_timestamp_ms = 0;
    closed_timestamp_ms = 0;

    last_op_timestamp_ms = 0;
    last_status = NodeInvalid;

    recording_bytes = 0;
    send_count = 0;
    play_bytes = 0;
    play_count = 0;
  };
  ~NodeProcess() {};

  uint64_t create_timestamp_ms;
  uint64_t start_timestamp_ms;
  uint64_t started_timestamp_ms;
  uint64_t stop_timestamp_ms;
  uint64_t cancel_timestamp_ms;
  uint64_t first_binary_timestamp_ms;
  uint64_t last_send_timestamp_ms;
  uint64_t last_ctrl_timestamp_ms;
  uint64_t failed_timestamp_ms;
  uint64_t completed_timestamp_ms;
  uint64_t closed_timestamp_ms;

  uint64_t last_op_timestamp_ms;
  ConnectStatus last_status;

  uint64_t recording_bytes;
  uint64_t send_count;
  uint64_t play_bytes;
  uint64_t play_count;
};
#endif

class ConnectNode {

 public:
  ConnectNode() {};
  ConnectNode(INlsRequest* request, 
              HandleBaseOneParamWithReturnVoid<NlsEvent>* handler,
              bool isLongConnection = false);
  virtual ~ConnectNode();

  bool parseUrlInformation(char *ip);

  void addCmdDataBuffer(CmdType type, const char* message = NULL);
  int addAudioDataBuffer(const uint8_t * frame, size_t length);
  int addSlicedAudioDataBuffer(const uint8_t * frame, size_t length);

  int cmdNotify(CmdType type, const char* message);

  int nlsSend(const uint8_t * frame, size_t length);
  int nlsSendFrame(struct evbuffer * eventBuffer);
  int nlsReceive(uint8_t *buffer, int max_size);

  int gatewayResponse();
  int gatewayRequest();

  int webSocketResponse();

  int dnsProcess(int aiFamily, char *directIp, bool sysGetAddr);
  int connectProcess(const char *ip, int aiFamily);
  int sslProcess();
  void closeStatusConnectNode();
  void closeConnectNode();
  void disconnectProcess();
  bool checkConnectCount();

  void handlerTaskFailedEvent(
      std::string failedInfo, int code = DefaultErrorCode);
  void handlerEvent(const char* error, int errorCode,
                    NlsEvent::EventType eventType, bool ignore = false);
  void handlerMessage(const char* response,
                      NlsEvent::EventType eventType);
  int handlerFrame(NlsEvent *frameEvent);

  /* init encoder about opus&opu */
  void initNlsEncoder();
  /* take effect all setting parameters */
  void updateParameters();

  /* setting NlsClient of this node */
  void setInstance(NlsClient* instance);
  NlsClient* getInstance();
  INlsRequest *_request;                                /*setting request of this node*/
  HandleBaseOneParamWithReturnVoid<NlsEvent>* _handler; /*callback listener*/
  WorkThread* _eventThread;                             /*setting WorkThread of this node*/
  unsigned int _syncCallTimeoutMs;
  std::string getNodeUUID();

  /* design to record work status */
  ConnectStatus _workStatus;
  ConnectStatus getConnectNodeStatus();
  std::string getConnectNodeStatusString();
  std::string getConnectNodeStatusString(ConnectStatus status);
  void setConnectNodeStatus(ConnectStatus status);

  /* design to record exit status */
  ExitStatus _exitStatus;
  ExitStatus getExitStatus();
  std::string getExitStatusString();
  void setExitStatus(ExitStatus status);

  /* design to record wakeup status */
  bool _isWakeStop;
  bool getWakeStatus();
  void setWakeStatus(bool status);

  /* about socket connection */
  urlAddress _url;
  evutil_socket_t _socketFd;
  SSLconnect *_sslHandle;
  int socketConnect();

  /* about status of node in destroy */
  bool _isDestroy;
  bool updateDestroyStatus();
  /* about event of callback */
  void delAllEvents();
  void waitEventCallback();

  inline int getErrorCode() {
    return _nodeErrCode;
  };
  inline const char* getErrorMsg() {
    return _nodeErrMsg.c_str();
  };
  inline void setErrorMsg(const char* msg) {
    _nodeErrMsg.assign(msg);
  };

  inline struct evbuffer *getBinaryEvBuffer() {return _binaryEvBuffer;};
  inline struct evbuffer *getCmdEvBuffer() {return _cmdEvBuffer;};
  inline struct evbuffer *getWwvEvBuffer() {return _wwvEvBuffer;};

  /* get event point of launching node */
  struct event *getLaunchEvent(bool init = false);

  /* design to long connection */
  bool _isLongConnection;
  bool _isConnected;
  void initAllStatus();  /*init all status in longConnection mode*/

  int sendControlDirective();
  void setSyncCallTimeout(unsigned int timeout_ms);
  void waitInvokeFinish();

  /* design to native_getaddrinfo */
#ifndef _MSC_VER
  char * _nodename;
  char * _servname;

  pthread_t _dnsThread;     /*异步dns方案启动线程*/
  bool _dnsThreadExit;
  bool _dnsThreadRunning;
  struct timespec _outtime; /*异步dns方案超时设置*/

  struct gaicb * _gaicbRequest[1];
  struct event * _dnsEvent;
  int _dnsErrorCode;
  struct evutil_addrinfo * _addrinfo;
#endif
  int _dnsRequestCallbackStatus;

  /* design for thread safe */
#if defined(_MSC_VER)
  HANDLE _mtxNode;
  HANDLE _mtxCloseNode;
  HANDLE _mtxEventCallbackNode;
#else
  pthread_mutex_t _mtxNode;
  pthread_mutex_t _mtxCloseNode;
  pthread_mutex_t _mtxEventCallbackNode;
  pthread_cond_t  _cvEventCallbackNode;   /*释放过程中等待事件回调结束*/
#endif
  bool _inEventCallbackNode;              /*是否处于事件回调中*/

  /* design for sync call */
#if defined(_MSC_VER)
  HANDLE _mtxInvokeSyncCallNode;
#else
  pthread_mutex_t _mtxInvokeSyncCallNode;
  pthread_cond_t  _cvInvokeSyncCallNode;   /*调用过程中等待调用结束*/
#endif

#ifdef ENABLE_REQUEST_RECORDING
  /* design for recording process */
  struct NodeProcess _nodeProcess;
#endif

 private:
#if defined(__ANDROID__) || defined(__linux__)
  int codeConvert(char *from_charset,
                  char *to_charset,
                  char *inbuf,
                  size_t inlen,
                  char *outbuf,
                  size_t outlen);
#endif

  std::string utf8ToGbk(const std::string &strUTF8);
  NlsEvent* convertResult(WebSocketFrame *frame, int *result);
  const char* cmdConvertForLog(char *buf_in, std::string *buf_out);

  int addRemainAudioData();

  int parseFrame(WebSocketFrame *wsFrame);

  int socketWrite(const uint8_t *buffer, size_t len);
  int socketRead(uint8_t *buffer, size_t len);

  int getErrorCodeFromMsg(const char *msg);
  std::string getCmdTypeString(int type);

  void sendFinishCondSignal(NlsEvent::EventType eventType);

  std::string getRandomUuid();
#ifdef ENABLE_REQUEST_RECORDING
  std::string replenishNodeProcess(const char *error);
#endif

  /* parameters about network */
  int _aiFamily;
  struct sockaddr_in _addrV4;
  struct sockaddr_in6 _addrV6;
  WebSocketTcp _webSocket;
  WebSocketHeaderType _wsType;
  struct evdns_getaddrinfo_request *_dnsRequest;
  size_t _retryConnectCount; /*try count of connection*/

  size_t _limitSize;
  struct timeval _connectTv;
  bool _enableRecvTv;
  bool _enableOnMessage;
  struct timeval _recvTv;
  struct timeval _sendTv;

  std::string	_nodeErrMsg;
  int	_nodeErrCode;

  struct evbuffer *_readEvBuffer;
  struct evbuffer *_binaryEvBuffer;
  struct evbuffer *_cmdEvBuffer;
  struct evbuffer *_wwvEvBuffer;

  struct event *_launchEvent;
  struct event *_connectEvent;
  struct event *_readEvent;
  struct event *_writeEvent;

#ifdef ENABLE_HIGH_EFFICIENCY
  struct timeval _connectTimerTv;
  struct event *_connectTimerEvent;
  bool _connectTimerFlag;
#endif

  /* about audio data encoder */
  NlsEncoder * _nlsEncoder;
  ENCODER_TYPE _encoderType;
  uint8_t * _audioFrame;
  int _audioFrameSize;
  int _maxFrameSize;
  bool _isFrirstAudioFrame;

  bool _isStop;
  bool _isFirstBinaryFrame;

  NlsClient* _instance;
  std::string _nodeUUID;
};

}

#endif // NLS_SDK_CONNECT_NODE_H
