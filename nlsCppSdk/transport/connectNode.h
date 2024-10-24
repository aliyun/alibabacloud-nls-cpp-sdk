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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#endif

#include <stdint.h>
#include <queue>
#include <string>

#include "SSLconnect.h"
#include "error.h"
#include "event.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/util.h"
#include "nlsClientImpl.h"
#include "nlsEncoder.h"
#include "nlsGlobal.h"
#include "webSocketFrameHandleBase.h"
#include "webSocketTcp.h"

#if defined(ENABLE_REQUEST_RECORDING) || defined(ENABLE_CONTINUED)
#include "json/json.h"
#endif

namespace AlibabaNls {

class INlsRequest;
class WorkThread;
class NlsEventNetWork;

#if defined(_MSC_VER)

#define NLS_ERR_IS_EAGAIN(e) ((e) == WSAEWOULDBLOCK || (e) == EAGAIN)
#define NLS_ERR_RW_RETRIABLE(e) ((e) == WSAEWOULDBLOCK || (e) == WSAEINTR)
#define NLS_ERR_CONNECT_RETRIABLE(e)                                    \
  ((e) == WSAEWOULDBLOCK || (e) == WSAEINTR || (e) == WSAEINPROGRESS || \
   (e) == WSAEINVAL)
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
#define NLS_ERR_CONNECT_RETRIABLE(e) ((e) == EINTR || (e) == EINPROGRESS)
/* True iff e is an error that means a accept can be retried. */
#define NLS_ERR_ACCEPT_RETRIABLE(e) \
  ((e) == EINTR || NLS_ERR_IS_EAGAIN(e) || (e) == ECONNABORTED)
/* True iff e is an error that means the connection was refused */
#define NLS_ERR_CONNECT_REFUSED(e) ((e) == ECONNREFUSED)

#endif /*#if defined(_MSC_VER)*/

#define CLOSE_JSON_STRING "{\"channelClosed\": \"nls request finished.\"}"
#define TASKFAILED_CONNECT_JSON_STRING "connect failed."
#define TASKFAILED_PARSE_JSON_STRING \
  "{\"TaskFailed\": \"JSON: Json parse failed.\"}"
#define TASKFAILED_NEW_NLSEVENT_FAILED \
  "{\"TaskFailed\": \"new NlsEvent failed, memory is not enough.\"}"
#define TASKFAILED_UTF8_JSON_STRING "{\"TaskFailed\": \"utf8ToGbk failed.\"}"
#define TASKFAILED_WS_JSON_STRING \
  "{\"TaskFailed\": \"WEBSOCKET: unkown head type.\"}"
#define TASKFAILED_ERROR_CLOSE_STRING \
  "{\"TaskFailed\": \"WEBSOCKET: invalid closeCode of wsFrame.\"}"

/* EventNetWork发送Node的具体指令 */
enum CmdType {
  CmdStart = 0,
  CmdStop,
  CmdStControl,
  CmdTextDialog,
  CmdExecuteDialog,
  CmdWarkWord,
  CmdCancel,
  CmdSendText,
  CmdSendPing,
  CmdSendFlush,
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
  NodeInvoking, /* 刚调用start的过程, 向notifyEventCallback发送c指令 */
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
  NodeSendText
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

    /* about API */
    api_start_run = false;
    api_stop_run = false;
    api_cancel_run = false;
    api_send_run = false;
    api_ctrl_run = false;
    last_api_timestamp_ms = 0;

    /* about CALLBACK */
    last_callback = NlsEvent::TaskFailed;
    last_cb_start_timestamp_ms = 0;
    last_cb_end_timestamp_ms = 0;
    last_cb_run = false;
  };
  ~NodeProcess(){};

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

  /* about API */
  bool api_start_run;
  bool api_stop_run;
  bool api_cancel_run;
  bool api_send_run;
  bool api_ctrl_run;
  uint64_t last_api_timestamp_ms;

  /* about CALLBACK */
  NlsEvent::EventType last_callback;
  uint64_t last_cb_start_timestamp_ms;
  uint64_t last_cb_end_timestamp_ms;
  bool last_cb_run;
};
#endif

#ifdef ENABLE_CONTINUED
/* Node运行中断自动重连的状态记录 */
struct NodeReconnection {
 public:
  enum { max_try_count = 4, reconnect_interval_ms = 100 };
  enum ReconnectionState {
    NoReconnection = 0,       // The reconnection has not been triggered
    WillReconnect,            // Trigger reconnection, will event_add(launch)
    TriggerReconnection,      // Triggers a new reconnection, new node launched
    NewReconnectionStarting,  // New node is running
  };
  explicit NodeReconnection() {
    state = NoReconnection;
    reconnected_count = 0;
    tw_index_offset = 0;
    interruption_timestamp_ms = 0;
    first_audio_timestamp_ms = 0;
  };
  ~NodeReconnection(){};

  ReconnectionState state;
  uint32_t reconnected_count;
  uint64_t tw_index_offset;
  uint64_t interruption_timestamp_ms;
  uint64_t first_audio_timestamp_ms;
};
#endif

class ConnectNode {
 public:
  ConnectNode(INlsRequest *request,
              HandleBaseOneParamWithReturnVoid<NlsEvent> *handler,
              bool isLongConnection = false);
  virtual ~ConnectNode();

  /* 1. about pointer and status of this node  */
  /* 1.1. something about instance&request&eventThread of this node */
  inline void setInstance(NlsClientImpl *instance) { _instance = instance; }
  inline NlsClientImpl *getInstance() { return _instance; }
  inline INlsRequest *getRequest() { return _request; }
  inline void setRequest(INlsRequest *request) { _request = request; }
  inline WorkThread *getEventThread() { return _eventThread; }
  inline void setEventThread(WorkThread *thread) { _eventThread = thread; }
  /* 1.2. get event point of launching node */
  struct event *getLaunchEvent(bool init = false);
  /* 1.3. something about status of this node */
  /*      design to record work status */
  ConnectStatus getConnectNodeStatus();
  std::string getConnectNodeStatusString();
  std::string getConnectNodeStatusString(ConnectStatus status);
  void setConnectNodeStatus(ConnectStatus status);
  /* 1.4. UUID of this node */
  inline std::string getNodeUUID() { return _nodeUUID; }
  /*      design to record exit status */
  ExitStatus getExitStatus();
  std::string getExitStatusString();
  /*      design to record wakeup status */
  bool getWakeStatus();
  /*      take effect all setting parameters */
  void updateParameters();
  /*      design to long connection */
  inline bool isLongConnection() { return _isLongConnection; }
  inline void setConnected(bool isConnected) { _isConnected = isConnected; }
  void initAllStatus(); /*init all status in longConnection mode*/
  /* 1.4. about error */
  inline int getErrorCode() { return _nodeErrCode; };
  inline const char *getErrorMsg() { return _nodeErrMsg.c_str(); };

  /* 2. command operation of request */
  /* 2.1. run command */
  void addCmdDataBuffer(CmdType type, const char *message = NULL);
  int cmdNotify(CmdType type, const char *message);
  /* 2.2. evBuffer for command */
  inline struct evbuffer *getBinaryEvBuffer() { return _binaryEvBuffer; };
  inline struct evbuffer *getCmdEvBuffer() { return _cmdEvBuffer; };
  inline struct evbuffer *getWwvEvBuffer() { return _wwvEvBuffer; };

  /* 3. send command and audio data */
  /* 3.1. send audio data */
  int addAudioDataBuffer(const uint8_t *frame, size_t length);
  int addSlicedAudioDataBuffer(const uint8_t *frame, size_t length);
  /* 3.2. parse&send request */
  int sendControlDirective();
  int gatewayRequest();
  int nlsSendFrame(struct evbuffer *eventBuffer, bool audio_frame = true);

  /* 4. recv response and parse */
  int gatewayResponse();
  int webSocketResponse();

  /* 5. something about network */
  inline evutil_socket_t getSocketFd() { return _socketFd; }
  inline urlAddress getUrlAddress() { return _url; }
  int dnsProcess(int aiFamily, char *directIp, bool sysGetAddr);
  int socketConnect();
  int connectProcess(const char *ip, int aiFamily);
  int sslProcess();
  void disconnectProcess();

  /* 6. exit operation */
  void closeConnectNode();
  /*    about status of node in destroy */
  bool updateDestroyStatus();
  /*    about event of callback */
  void delAllEvents();

  /* 7. something about other modules */
  /*    init encoder about opus&opu */
  void initNlsEncoder();

  /* 8. design to native_getaddrinfo */
#ifdef __LINUX__
  char *_nodename;
  char *_servname;

  pthread_t _dnsThread; /*异步dns方案启动线程*/
  bool _dnsThreadExit;
  bool _dnsThreadRunning;   /*异步dns方案线程已经启动*/
  struct timespec _outtime; /*异步dns方案超时设置*/

  struct gaicb *_gaicbRequest[1];
  struct event *_dnsEvent;
  int _dnsErrorCode;
  struct evutil_addrinfo *_addrinfo;

  static void *async_dns_resolve_thread_fn(void *arg);
#endif
  int _dnsRequestCallbackStatus; /* 1:开始DNS; 2:结束DNS */

  /* 9. design for thread safe */
#if defined(_MSC_VER)
  HANDLE _mtxNode;
  HANDLE _mtxCloseNode;
  HANDLE _mtxEventCallbackNode;
#else
  pthread_mutex_t _mtxNode;
  pthread_mutex_t _mtxCloseNode;
  pthread_mutex_t _mtxEventCallbackNode;
  pthread_cond_t _cvEventCallbackNode; /*释放过程中等待事件回调结束*/
#endif
  bool _inEventCallbackNode;         /*是否处于事件回调中*/
  bool _releasingFlag;               /*处于释放中*/
  bool _waitEventCallbackAbnormally; /*处于异常状态*/

  /* 10. design for sync call */
  inline void setSyncCallTimeout(unsigned int timeout_ms) {
    _syncCallTimeoutMs = timeout_ms;
  }
  inline unsigned int getSyncCallTimeout() { return _syncCallTimeoutMs; }
  void waitInvokeFinish();

  /* 11. about listener */
  void handlerTaskFailedEvent(std::string failedInfo,
                              int code = DefaultErrorCode);

#ifdef ENABLE_REQUEST_RECORDING
  /* 12. design for recording process */
  void updateNodeProcess(std::string api, int status, bool enter, size_t size);
  const char *dumpAllInfo();
#endif

#ifdef ENABLE_CONTINUED
  /* 13. design for reconnection automatically */
  struct event *getReconnectEvent();
  struct NodeReconnection _reconnection;
#endif

  /* 14. others */
  void sendFakeSynthesisStarted();

 private:
  enum ConnectNodeConstValue {
    RetryConnectCount = 4,
    ConnectTimerIntervalMs = 30,
    SampleRate16K = 16000,
    Buffer8kMaxLimit = 16000,
    Buffer16kMaxLimit = 32000,
    NodeFrameSize = 2048,
  };

  /* 1. about pointer and status of this node  */
  /* 1.1. something about instance&request&eventThread of this node */
  NlsClientImpl *_instance;
  /*      setting WorkThread of this node*/
  WorkThread *_eventThread;
  /*      setting request of this node*/
  INlsRequest *_request;
  /* 1.2. event point of launching node */
  struct event *_launchEvent;
  /* 1.3. something about status of this node */
  bool _isStop;
  bool _isFirstBinaryFrame;
  /*      design to record work status */
  ConnectStatus _workStatus;
  /*      design to record exit status */
  ExitStatus _exitStatus;
  /*      design to record wakeup status */
  bool _isWakeStop;
  /*      about status of node in destroy */
  bool _isDestroy;
  /*      design to long connection */
  bool _isConnected;
  bool _isLongConnection;
  /* 1.4. about error */
  int getErrorCodeFromMsg(const char *msg);
  std::string _nodeErrMsg;
  int _nodeErrCode;
  /* 1.5. about UUID */
  std::string _nodeUUID;

  /* 2. command operation of request */
  /* 2.1. run command */
  std::string getCmdTypeString(int type);
  /* 2.2. evBuffer for command */
  size_t _limitSize;
  struct evbuffer *_readEvBuffer;
  struct evbuffer *_binaryEvBuffer;
  struct evbuffer *_cmdEvBuffer;
  struct evbuffer *_wwvEvBuffer;
  /* 2.3. ev for command */
  struct event *_connectEvent;
  struct event *_readEvent;
  struct event *_writeEvent;

  /* 3. send command and audio data */
  /* 3.1. send audio data */
  int addRemainAudioData();
  /* 3.2. parse&send request */
  bool parseUrlInformation(char *ip);
  int socketWrite(const uint8_t *buffer, size_t len);
  int nlsSend(const uint8_t *frame, size_t length);

  /* 4. recv response and parse */
  int socketRead(uint8_t *buffer, size_t len);
  int nlsReceive(uint8_t *buffer, int max_size);
  NlsEvent *convertResult(WebSocketFrame *frame, int *result);
  int parseFrame(WebSocketFrame *wsFrame);

  /* 5. something about network */
  bool checkConnectCount();
  /*    about socket connection */
  urlAddress _url;
  evutil_socket_t _socketFd;
  SSLconnect *_sslHandle;
  /*    parameters about network */
  int _aiFamily;
  struct sockaddr_in _addrV4;
  struct sockaddr_in6 _addrV6;
  WebSocketTcp _webSocket;
  WebSocketHeaderType _wsType;
  struct evdns_getaddrinfo_request *_dnsRequest;
  size_t _retryConnectCount; /*try count of connection*/
  struct timeval _connectTv;
  bool _enableRecvTv;
  struct timeval _recvTv;
  struct timeval _sendTv;
#ifdef ENABLE_HIGH_EFFICIENCY
  struct timeval _connectTimerTv;
  struct event *_connectTimerEvent;
  bool _connectTimerFlag;
#endif

  /* 6. exit operation */
  const char *genCloseMsg(std::string *buf_str);
  void closeStatusConnectNode();

  /* 7. something about other modules */
  /*    about audio data encoder */
  NlsEncoder *_nlsEncoder;
  ENCODER_TYPE _encoderType;
  uint8_t *_audioFrame;
  int _audioFrameSize;
  int _maxFrameSize;
  bool _isFirstAudioFrame;

  /* 9. design for thread safe */
  void waitEventCallback();
  /* 10. design for sync call */
  void sendFinishCondSignal(NlsEvent::EventType eventType);
#if defined(_MSC_VER)
  HANDLE _mtxInvokeSyncCallNode;
#else
  pthread_mutex_t _mtxInvokeSyncCallNode;
  pthread_cond_t _cvInvokeSyncCallNode; /*调用过程中等待调用结束*/
#endif
  unsigned int _syncCallTimeoutMs;

  /* 11. about listener */
  void handlerEvent(const char *error, int errorCode,
                    NlsEvent::EventType eventType, bool ignore = false);
  void handlerMessage(const char *response, NlsEvent::EventType eventType);
  int handlerFrame(NlsEvent *frameEvent);
  HandleBaseOneParamWithReturnVoid<NlsEvent> *_handler; /*callback listener*/
  bool _enableOnMessage;

#ifdef ENABLE_REQUEST_RECORDING
  /* 12. design for recording process */
  std::string replenishNodeProcess(const char *error);
  Json::Value updateNodeProcess4Data();
  Json::Value updateNodeProcess4Last();
  Json::Value updateNodeProcess4Timestamp();
  Json::Value updateNodeProcess4Callback();
  Json::Value updateNodeProcess4Block();

  struct NodeProcess _nodeProcess;
#endif

#ifdef ENABLE_CONTINUED
  /* 13. design for reconnection automatically */
  Json::Value updateNodeReconnection();
  void updateTwIndexOffset(NlsEvent *frameEvent);
  bool nodeReconnecting();
  struct event *_reconnectEvent;
#endif
  bool ignoreCallbackWhenReconnecting(NlsEvent::EventType eventType, int code);

  /* 14. others */
  const char *genSynthesisStartedMsg();
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_CONNECT_NODE_H
