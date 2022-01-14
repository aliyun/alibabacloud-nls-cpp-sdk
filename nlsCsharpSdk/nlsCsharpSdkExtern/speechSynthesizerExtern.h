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

#ifndef _NLSCPPSDK_SYNTHESIZER_EXTERN_H_
#define _NLSCPPSDK_SYNTHESIZER_EXTERN_H_


static NlsCallbackDelegate synthesisTaskFailedCallback = NULL;
static NlsCallbackDelegate synthesisDataReceivedCallback = NULL;
static NlsCallbackDelegate synthesisCompletedCallback = NULL;
static NlsCallbackDelegate synthesisClosedCallback = NULL;
static NlsCallbackDelegate metaInfoCallback = NULL;


NLSAPI(int) SYstart(AlibabaNls::SpeechSynthesizerRequest* request)
{
	return request->start();
}

NLSAPI(int) SYstop(AlibabaNls::SpeechSynthesizerRequest* request)
{
	return request->stop();
}

NLSAPI(int) SYcancel(AlibabaNls::SpeechSynthesizerRequest* request)
{
	return request->cancel();
}


// 对外回调
static void onSynthesisTaskFailed(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, syEvent);
	if (synthesisTaskFailedCallback)
	{
		synthesisTaskFailedCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onSynthesisClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, syEvent);
	if (synthesisClosedCallback)
	{
		synthesisClosedCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onSynthesisCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, syEvent);
	if (synthesisCompletedCallback)
	{
		synthesisCompletedCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onSynthesisMetaInfo(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, syEvent);
	if (metaInfoCallback)
	{
		metaInfoCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onSynthesisDataReceived(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, syEvent);
	if (synthesisDataReceivedCallback)
	{
		synthesisDataReceivedCallback(cbEvent->getStatusCode());
	}
	return;
}


NLSAPI(int) SYGetNlsEvent(NLS_EVENT_STRUCT &event)
{
	event.statusCode = syEvent->statusCode;
	memcpy(event.msg, syEvent->msg, 8192);
	event.msgType = syEvent->msgType;
	memcpy(event.taskId, syEvent->taskId, 128);
	memcpy(event.result, syEvent->result, 8192);
	memcpy(event.displayText, syEvent->displayText, 8192);
	memcpy(event.spokenText, syEvent->spokenText, 8192);
	event.sentenceTimeOutStatus = syEvent->sentenceTimeOutStatus;
	event.sentenceIndex = syEvent->sentenceIndex;
	event.sentenceTime = syEvent->sentenceTime;
	event.sentenceBeginTime = syEvent->sentenceBeginTime;
	event.sentenceConfidence = syEvent->sentenceConfidence;
	event.wakeWordAccepted = syEvent->wakeWordAccepted;
	event.wakeWordKnown = syEvent->wakeWordKnown;
	memcpy(event.wakeWordUserId, syEvent->wakeWordUserId, 128);
	event.wakeWordGender = syEvent->wakeWordGender;
	memcpy(event.binaryData, syEvent->binaryData, 16384);
	event.binaryDataSize = syEvent->binaryDataSize;
	event.stashResultSentenceId = syEvent->stashResultSentenceId;
	event.stashResultBeginTime = syEvent->stashResultBeginTime;
	memcpy(event.stashResultText, syEvent->stashResultText, 8192);
	event.stashResultCurrentTime = syEvent->stashResultBeginTime;
	event.isValid = false;

	return event.binaryDataSize;
}

// 设置回调
NLSAPI(int) SYOnSynthesisCompleted(
	AlibabaNls::SpeechSynthesizerRequest* request, NlsCallbackDelegate c)
{
	request->setOnSynthesisCompleted(onSynthesisCompleted, NULL);
	synthesisCompletedCallback = c;
	return 0;
}

NLSAPI(int) SYOnTaskFailed(
	AlibabaNls::SpeechSynthesizerRequest* request, NlsCallbackDelegate c)
{
	request->setOnTaskFailed(onSynthesisTaskFailed, NULL);
	synthesisTaskFailedCallback = c;
	return 0;
}

NLSAPI(int) SYOnChannelClosed(
	AlibabaNls::SpeechSynthesizerRequest* request, NlsCallbackDelegate c)
{
	request->setOnChannelClosed(onSynthesisClosed, NULL);
	synthesisClosedCallback = c;
	return 0;
}

NLSAPI(int) SYOnBinaryDataReceived(
	AlibabaNls::SpeechSynthesizerRequest* request, NlsCallbackDelegate c)
{
	request->setOnBinaryDataReceived(onSynthesisDataReceived, NULL);
	synthesisDataReceivedCallback = c;
	return 0;
}

NLSAPI(int) SYOnMetaInfo(
	AlibabaNls::SpeechSynthesizerRequest* request, NlsCallbackDelegate c)
{
	request->setOnMetaInfo(onSynthesisMetaInfo, NULL);
	metaInfoCallback = c;
	return 0;
}


// 设置参数
NLSAPI(int) SYsetUrl(AlibabaNls::SpeechSynthesizerRequest* request, const char* value)
{
	return request->setUrl(value);
}

NLSAPI(int) SYsetAppKey(AlibabaNls::SpeechSynthesizerRequest* request, const char* value)
{
	return request->setAppKey(value);
}

NLSAPI(int) SYsetToken(AlibabaNls::SpeechSynthesizerRequest* request, const char* value)
{
	return request->setToken(value);
}

NLSAPI(int) SYsetFormat(AlibabaNls::SpeechSynthesizerRequest* request, const char* value)
{
	return request->setFormat(value);
}

NLSAPI(int) SYsetSampleRate(AlibabaNls::SpeechSynthesizerRequest* request, int value)
{
	return request->setSampleRate(value);
}

NLSAPI(int) SYsetText(AlibabaNls::SpeechSynthesizerRequest* request, uint8_t* text, uint32_t textSize)
{
	char* textChar = new char[textSize + 1];
	memset(textChar, 0, textSize + 1);
	memcpy(textChar, text, textSize);
	return request->setText((const char*)textChar);
}

NLSAPI(int) SYsetVoice(AlibabaNls::SpeechSynthesizerRequest* request, const char* value)
{
	return request->setVoice(value);
}

NLSAPI(int) SYsetVolume(AlibabaNls::SpeechSynthesizerRequest* request, int value)
{
	return request->setVolume(value);
}

NLSAPI(int) SYsetSpeechRate(AlibabaNls::SpeechSynthesizerRequest* request, int value)
{
	return request->setSpeechRate(value);
}

NLSAPI(int) SYsetPitchRate(AlibabaNls::SpeechSynthesizerRequest* request, int value)
{
	return request->setPitchRate(value);
}

NLSAPI(int) SYsetMethod(AlibabaNls::SpeechSynthesizerRequest* request, int value)
{
	return request->setMethod(value);
}

NLSAPI(int) SYsetEnableSubtitle(AlibabaNls::SpeechSynthesizerRequest* request, bool value)
{
	return request->setEnableSubtitle(value);
}

NLSAPI(int) SYsetPayloadParam(AlibabaNls::SpeechSynthesizerRequest* request, const char* value)
{
	return request->setPayloadParam(value);
}

NLSAPI(int) SYsetTimeout(AlibabaNls::SpeechSynthesizerRequest* request, int value)
{
	return request->setTimeout(value);
}

NLSAPI(int) SYsetOutputFormat(AlibabaNls::SpeechSynthesizerRequest* request, const char* value)
{
	return request->setOutputFormat(value);
}

NLSAPI(int) SYsetContextParam(AlibabaNls::SpeechSynthesizerRequest* request, const char* value)
{
	return request->setContextParam(value);
}

NLSAPI(int) SYappendHttpHeaderParam(AlibabaNls::SpeechSynthesizerRequest* request, const char* key, const char* value)
{
	return request->AppendHttpHeaderParam(key, value);
}


#endif // _NLSCPPSDK_SYNTHESIZER_EXTERN_H_
