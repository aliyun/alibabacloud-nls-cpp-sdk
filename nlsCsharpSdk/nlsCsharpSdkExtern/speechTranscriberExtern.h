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

#ifndef _NLSCPPSDK_TRANSCRIBER_EXTERN_H_
#define _NLSCPPSDK_TRANSCRIBER_EXTERN_H_


static NlsCallbackDelegate transcriptionTaskFailedCallback = NULL;
static NlsCallbackDelegate transcriptionStartedCallback = NULL;
static NlsCallbackDelegate transcriptionResultChangedCallback = NULL;
static NlsCallbackDelegate transcriptionCompletedCallback = NULL;
static NlsCallbackDelegate transcriptionClosedCallback = NULL;
static NlsCallbackDelegate sentenceBeginCallback = NULL;
static NlsCallbackDelegate sentenceEndCallback = NULL;
static NlsCallbackDelegate sentenceSemanticsCallback = NULL;

NLSAPI(int) STstart(AlibabaNls::SpeechTranscriberRequest* request)
{
	return request->start();
}

NLSAPI(int) STstop(AlibabaNls::SpeechTranscriberRequest* request)
{
	return request->stop();
}


NLSAPI(int) STsendAudio(AlibabaNls::SpeechTranscriberRequest* request, uint8_t* data, uint64_t dataSize, int type)
{
	return request->sendAudio(data, dataSize, (ENCODER_TYPE)type);
}

// 对外回调
static void onTranscriptionStarted(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, stEvent);
	if (transcriptionStartedCallback)
	{
		transcriptionStartedCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onTranscriptionResultChanged(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, stEvent);
	if (transcriptionResultChangedCallback)
	{
		transcriptionResultChangedCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onTranscriptionCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, stEvent);
	if (transcriptionCompletedCallback)
	{
		transcriptionCompletedCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onTranscriptionClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, stEvent);
	if (transcriptionClosedCallback)
	{
		transcriptionClosedCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onTranscriptionTaskFailed(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, stEvent);
	if (transcriptionTaskFailedCallback)
	{
		transcriptionTaskFailedCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onSentenceBegin(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, stEvent);
	if (sentenceBeginCallback)
	{
		sentenceBeginCallback(cbEvent->getStatusCode());
	}
	return;
}

static void onSentenceEnd(AlibabaNls::NlsEvent* cbEvent, void* cbParam)
{
	ConvertNlsEvent(cbEvent, stEvent);
	if (sentenceEndCallback)
	{
		sentenceEndCallback(cbEvent->getStatusCode());
	}
	return;
}


NLSAPI(int) STGetNlsEvent(NLS_EVENT_STRUCT &event)
{
	event.statusCode = stEvent->statusCode;
	memcpy(event.msg, stEvent->msg, 8192);
	event.msgType = stEvent->msgType;
	memcpy(event.taskId, stEvent->taskId, 128);
	memcpy(event.result, stEvent->result, 8192);
	memcpy(event.displayText, stEvent->displayText, 8192);
	memcpy(event.spokenText, stEvent->spokenText, 8192);
	event.sentenceTimeOutStatus = stEvent->sentenceTimeOutStatus;
	event.sentenceIndex = stEvent->sentenceIndex;
	event.sentenceTime = stEvent->sentenceTime;
	event.sentenceBeginTime = stEvent->sentenceBeginTime;
	event.sentenceConfidence = stEvent->sentenceConfidence;
	event.wakeWordAccepted = stEvent->wakeWordAccepted;
	event.wakeWordKnown = stEvent->wakeWordKnown;
	memcpy(event.wakeWordUserId, stEvent->wakeWordUserId, 128);
	event.wakeWordGender = stEvent->wakeWordGender;
	memcpy(event.binaryData, stEvent->binaryData, 16384);
	event.binaryDataSize = stEvent->binaryDataSize;
	event.stashResultSentenceId = stEvent->stashResultSentenceId;
	event.stashResultBeginTime = stEvent->stashResultBeginTime;
	memcpy(event.stashResultText, stEvent->stashResultText, 8192);
	event.stashResultCurrentTime = stEvent->stashResultBeginTime;
	event.isValid = false;

	return 0;
}

// 设置回调
NLSAPI(int) STOnTranscriptionStarted(
	AlibabaNls::SpeechTranscriberRequest* request, NlsCallbackDelegate c)
{
	request->setOnTranscriptionStarted(onTranscriptionStarted, NULL);
	transcriptionStartedCallback = c;
	return 0;
}

NLSAPI(int) STOnTranscriptionResultChanged(
	AlibabaNls::SpeechTranscriberRequest* request, NlsCallbackDelegate c)
{
	request->setOnTranscriptionResultChanged(onTranscriptionResultChanged, NULL);
	transcriptionResultChangedCallback = c;
	return 0;
}

NLSAPI(int) STOnTranscriptionCompleted(
	AlibabaNls::SpeechTranscriberRequest* request, NlsCallbackDelegate c)
{
	request->setOnTranscriptionCompleted(onTranscriptionCompleted, NULL);
	transcriptionCompletedCallback = c;
	return 0;
}

NLSAPI(int) STOnTaskFailed(
	AlibabaNls::SpeechTranscriberRequest* request, NlsCallbackDelegate c)
{
	request->setOnTaskFailed(onTranscriptionTaskFailed, NULL);
	transcriptionTaskFailedCallback = c;
	return 0;
}

NLSAPI(int) STOnChannelClosed(
	AlibabaNls::SpeechTranscriberRequest* request, NlsCallbackDelegate c)
{
	request->setOnChannelClosed(onTranscriptionClosed, NULL);
	transcriptionClosedCallback = c;
	return 0;
}

NLSAPI(int) STOnSentenceBegin(
	AlibabaNls::SpeechTranscriberRequest* request, NlsCallbackDelegate c)
{
	request->setOnSentenceBegin(onSentenceBegin, NULL);
	sentenceBeginCallback = c;
	return 0;
}

NLSAPI(int) STOnSentenceEnd(
	AlibabaNls::SpeechTranscriberRequest* request, NlsCallbackDelegate c)
{
	request->setOnSentenceEnd(onSentenceEnd, NULL);
	sentenceEndCallback = c;
	return 0;
}

// 设置参数
NLSAPI(int) STsetUrl(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setUrl(value);
}

NLSAPI(int) STsetAppKey(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setAppKey(value);
}

NLSAPI(int) STsetToken(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setToken(value);
}

NLSAPI(int) STsetFormat(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setFormat(value);
}

NLSAPI(int) STsetSampleRate(AlibabaNls::SpeechTranscriberRequest* request, int value)
{
	return request->setSampleRate(value);
}

NLSAPI(int) STsetIntermediateResult(AlibabaNls::SpeechTranscriberRequest* request, bool value)
{
	return request->setIntermediateResult(value);
}

NLSAPI(int) STsetPunctuationPrediction(AlibabaNls::SpeechTranscriberRequest* request, bool value)
{
	return request->setPunctuationPrediction(value);
}

NLSAPI(int) STsetInverseTextNormalization(AlibabaNls::SpeechTranscriberRequest* request, bool value)
{
	return request->setInverseTextNormalization(value);
}

NLSAPI(int) STsetSemanticSentenceDetection(AlibabaNls::SpeechTranscriberRequest* request, bool value)
{
	return request->setSemanticSentenceDetection(value);
}

NLSAPI(int) STsetMaxSentenceSilence(AlibabaNls::SpeechTranscriberRequest* request, int value)
{
	return request->setMaxSentenceSilence(value);
}

NLSAPI(int) STsetCustomizationId(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setCustomizationId(value);
}

NLSAPI(int) STsetVocabularyId(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setVocabularyId(value);
}

NLSAPI(int) STsetTimeout(AlibabaNls::SpeechTranscriberRequest* request, int value)
{
	return request->setTimeout(value);
}

NLSAPI(int) STsetEnableNlp(AlibabaNls::SpeechTranscriberRequest* request, bool value)
{
	return request->setEnableNlp(value);
}

NLSAPI(int) STsetNlpModel(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setNlpModel(value);
}

NLSAPI(int) STsetSessionId(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setSessionId(value);
}

NLSAPI(int) STsetOutputFormat(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setOutputFormat(value);
}

NLSAPI(int) STsetPayloadParam(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setPayloadParam(value);
}

NLSAPI(int) STsetContextParam(AlibabaNls::SpeechTranscriberRequest* request, const char* value)
{
	return request->setContextParam(value);
}

NLSAPI(int) STappendHttpHeaderParam(AlibabaNls::SpeechTranscriberRequest* request, const char* key, const char* value)
{
	return request->AppendHttpHeaderParam(key, value);
}


#endif // _NLSCPPSDK_TRANSCRIBER_EXTERN_H_
