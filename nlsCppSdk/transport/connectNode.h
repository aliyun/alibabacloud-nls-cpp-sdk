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
//#include "nlsEvent.h"
#include "nlsEncoder.h"
#include "error.h"
#include "webSocketTcp.h"
#include "webSocketFrameHandleBase.h"
#include "SSLconnect.h"

#include "event2/util.h"
#include "event2/dns.h"
#include "event2/buffer.h"
#include "event.h"

namespace AlibabaNls {

class INlsRequest;
class WorkThread;
class NlsEventNetWork;

#define LIMIT_CONNECT_TIMEOUT 2
#define RETRY_CONNECT_COUNT 4
#define SAMPLE_RATE_16K 16000
#define SAMPLE_RATE_8K 8000
#define BUFFER_16K_MAX_LIMIT 320000
#define BUFFER_8K_MAX_LIMIT 160000

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

#define CLOSE_JSON_STRING "{\"channeclClosed\": \"nls request finished.\"}"
#define TASKFAILED_CONNECT_JSON_STRING "connect failed."
#define TASKFAILED_PARSE_JSON_STRING "{\"TaskFailed\": \"JSON: Json parse failed.\"}"
#define TASKFAILED_UTF8_JSON_STRING "{\"TaskFailed\": \"utf8ToGbk failed.\"}"
#define TASKFAILED_WS_JSON_STRING "{\"TaskFailed\": \"WEBSOCKET: unkown head type.\"}"

//type
enum CmdType {
  CmdStart = 0,
  CmdStop,
  CmdStControl,
  CmdTextDialog,
  CmdExecuteDialog,
  CmdWarkWord,
  CmdCancel
};

enum ExitStatus {
  ExitInvalid = 0,
  ExitStopping,
  ExitStopped,
  ExitCancel,
};

enum ConnectStatus {
  NodeInitial = 0,
  NodeConnecting,
  NodeConnected,
  NodeHandshaking,
  NodeHandshaked,
  NodeStarting,
  NodeStarted,
  NodeWakeWording,
  NodeInvalid
};

//class ConnectNode : public utility::BaseError {
class ConnectNode {

 public:
  ConnectNode() {};
  ConnectNode(INlsRequest* request, 
              HandleBaseOneParamWithReturnVoid<NlsEvent>* handler);
  virtual ~ConnectNode();

  bool parseUrlInformation();

  void addCmdDataBuffer(CmdType type, const char* message = NULL);
  int addAudioDataBuffer(const uint8_t * frame, size_t length);
  int cmdNotify(CmdType type, const char* message);

  int nlsSend(const uint8_t * frame, size_t length);
  int nlsSendFrame(struct evbuffer * eventBuffer);
  int nlsReceive(uint8_t *buffer, int max_size);

  int gatewayResponse();
  int gatewayRequest();

  int webSocketResponse();

  int dnsProcess();
  int connectProcess(const char *ip, int aiFamily);
  int sslProcess();
  void closeConnectNode();
  void disconnectProcess();

  bool checkConnectCount();

  void handlerEvent(const char* error, int errorCode,
                    NlsEvent::EventType eventType);
  void handlerTaskFailedEvent(std::string failedInfo);

  WorkThread* _eventThread;
  evutil_socket_t _socketFd;
  urlAddress _url;
  INlsRequest *_request;
  HandleBaseOneParamWithReturnVoid<NlsEvent>* _handler;

  SSLconnect *_sslHandle;
  ConnectStatus _workStatus;
  ConnectStatus getConnectNodeStatus();
  std::string getConnectNodeStatusString();
  void setConnectNodeStatus(ConnectStatus status);

  ExitStatus getExitStatus();
  std::string getExitStatusString();
  void setExitStatus(ExitStatus status);

  void resetBufferLimit();

  bool _isWakeStop;
  bool getWakeStatus();
  void setWakeStatus(bool status);

  bool _isDestroy;
  bool updateDestroyStatus();

  int socketConnect();
    
  void initNlsEncoder();

  inline const char* getErrorMsg() {
  return _nodeErrMsg.c_str();
  };
  inline struct evbuffer *getBinaryEvBuffer() {return _binaryEvBuffer;};
  inline struct evbuffer *getCmdEvBuffer() {return _cmdEvBuffer;};
  inline struct evbuffer *getWwvEvBuffer() {return _wwvEvBuffer;};

  int sendControlDirective();

 private:
  int _connectErrCode;
  int _aiFamily;
  struct sockaddr_in _addrV4;
  struct sockaddr_in6 _addrV6;

  size_t _limitSize;
  struct timeval _connectTv;

  std::string	_nodeErrMsg;

  struct evbuffer *_readEvBuffer;
  struct evbuffer *_binaryEvBuffer;
  struct evbuffer *_cmdEvBuffer;
  struct evbuffer *_wwvEvBuffer;

  struct event _connectEvent;
  struct event _readEvent;
  struct event _writeEvent;

  WebSocketTcp _webSocket;
  WebSocketHeaderType _wsType;

  ExitStatus _exitStatus;
  size_t _retryConnectCount;

  NlsEncoder * _nlsEncoder;
  ENCODER_TYPE _encoder_type;

#if defined(_MSC_VER)
  HANDLE _mtxNode;
  HANDLE _mtxCloseNode;
#else
  pthread_mutex_t  _mtxNode;
  pthread_mutex_t  _mtxCloseNode;
#endif

#if defined(__ANDROID__) || defined(__linux__)
  int codeConvert(char *from_charset,
                  char *to_charset,
                  char *inbuf,
                  size_t inlen,
                  char *outbuf,
                  size_t outlen);
#endif

  std::string utf8ToGbk(const std::string &strUTF8);
  NlsEvent* convertResult(WebSocketFrame * frame);

  int parseFrame(WebSocketFrame *wsFrame);

  int socketWrite(const uint8_t * buffer, size_t len);
  int socketRead(uint8_t * buffer, size_t len);

  bool _isStop;
};

}

#endif /*NLS_SDK_CONNECT_NODE_H*/
