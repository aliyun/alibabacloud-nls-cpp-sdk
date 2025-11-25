/*
 * Copyright 2025 Alibaba Group Holding Limited
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

#ifndef NLS_SDK_EVENT_INNER_H
#define NLS_SDK_EVENT_INNER_H

#include <list>
#include <string>
#include <vector>

#include "nlsEvent.h"
#include "nlsGlobal.h"

namespace AlibabaNls {

class NlsEventInner {
 public:
  NlsEventInner();
  NlsEventInner(const std::string& msg, NlsType nlsType,
                NlsServiceProtocol serviceProtocol = WsServiceProtocolNls);
  NlsEventInner(unsigned char* data, int dataBytes, int code,
                NlsEvent::EventType type, const std::string& taskId,
                NlsType nlsType,
                NlsServiceProtocol serviceProtocol = WsServiceProtocolNls);
  ~NlsEventInner();
  NlsEventInner& operator=(const NlsEventInner& event);

  int transferEvent(NlsEvent* target);

  /**
   * @brief 解析消息字符串
   * @param ignore 忽略消息中关键key的校验
   * @return 成功返回0，失败返回负值, 抛出异常
   */
  int parseJsonMsg(bool ignore = false);

  const char* getAllResponse();
  NlsEvent::EventType getMsgType();

 private:
  int parseMsgType(std::string name);
  int convertFunAsrStResultGenerated();
  int convertParaformerStResultGenerated();
  int convertCosyVoiceFssResultGenerated();

  int _statusCode;
  std::string _msg;
  NlsEvent::EventType _msgType;
  std::string _taskId;
  std::string _result;
  std::string _displayText;
  std::string _spokenText;
  int _sentenceTimeOutStatus;
  int _sentenceIndex;
  int _sentenceTime;
  int _sentenceBeginTime;
  double _sentenceConfidence;
  std::list<WordInfomation> _sentenceWordsList;
  bool _wakeWordAccepted;
  bool _wakeWordKnown;
  std::string _wakeWordUserId;
  int _wakeWordGender;

  std::vector<unsigned char> _binaryData;
  unsigned char* _binaryDataInChar;
  unsigned int _binaryDataSize;

  int _stashResultSentenceId;
  int _stashResultBeginTime;
  std::string _stashResultText;
  int _stashResultCurrentTime;

  int _usage;

  NlsType _nlsType;
  NlsServiceProtocol _serviceProtocol;
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_EVENT_INNER_H
