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

#ifndef _NLSCPPSDK_EVENT_STRUCT_H_
#define _NLSCPPSDK_EVENT_STRUCT_H_

struct NLS_EVENT_STRUCT
{
	int binaryDataSize;
	unsigned char binaryData[16384];

	int statusCode;
	unsigned char msg[8192];
	int msgType;
	char taskId[128];
	unsigned char result[8192];
	unsigned char displayText[8192];
	unsigned char spokenText[8192];
	int sentenceTimeOutStatus;
	int sentenceIndex;
	int sentenceTime;
	int sentenceBeginTime;
	double sentenceConfidence;
	bool wakeWordAccepted;
	bool wakeWordKnown;
	unsigned char wakeWordUserId[128];
	int wakeWordGender;

	int stashResultSentenceId;
	int stashResultBeginTime;
	unsigned char stashResultText[8192];
	int stashResultCurrentTime;

	bool isValid;

	void* user;

	HANDLE eventMtx;
};

static NLS_EVENT_STRUCT* srEvent = NULL;
static NLS_EVENT_STRUCT* stEvent = NULL;
static NLS_EVENT_STRUCT* syEvent = NULL;

static void CleanNlsEvent(NLS_EVENT_STRUCT* e)
{
	WaitForSingleObject(e->eventMtx, INFINITE);

	e->statusCode = 0;
	e->msgType = 0;
    memset(e->taskId, 0, 128);
	memset(e->result, 0, 8192);
	memset(e->displayText, 0, 8192);
	memset(e->spokenText, 0, 8192);
	e->sentenceTimeOutStatus = 0;
	e->sentenceIndex = 0;
	e->sentenceTime = 0;
	e->sentenceBeginTime = 0;
	e->sentenceConfidence = 0;
	e->wakeWordAccepted = false;
    e->binaryDataSize = 0;
	memset(e->msg, 0, 8192);
	e->stashResultSentenceId = 0;
	e->stashResultBeginTime = 0;
	memset(e->stashResultText, 0, 8192);
	e->stashResultCurrentTime = 0;
	e->isValid = false;

	ReleaseMutex(e->eventMtx);
	return;
}

static void ConvertNlsEvent(AlibabaNls::NlsEvent* in, NLS_EVENT_STRUCT* out)
{
	WaitForSingleObject(out->eventMtx, INFINITE);

	int str_len = 0;
	out->statusCode = in->getStatusCode();
	out->msgType = (int)in->getMsgType();

	const char* getTaskId = in->getTaskId();
	if (getTaskId) {
		strncpy(out->taskId, getTaskId, 128);
	}
	const char* getResult = in->getResult();
	if (getResult) {
		str_len = strnlen(getResult, 8192);
		memcpy(out->result, getResult, str_len);
	}
	const char* getDisplayText = in->getDisplayText();
	if (getDisplayText) {
		str_len = strnlen(getDisplayText, 8192);
		memcpy(out->displayText, getDisplayText, str_len);
	}
	const char* getSpokenText = in->getSpokenText();
	if (getSpokenText) {
		str_len = strnlen(getSpokenText, 8192);
		memcpy(out->spokenText, getSpokenText, str_len);
	}

	out->sentenceTimeOutStatus = in->getSentenceTimeOutStatus();
	out->sentenceIndex = in->getSentenceIndex();
	out->sentenceTime = in->getSentenceTime();
	out->sentenceBeginTime = in->getSentenceBeginTime();
	out->sentenceConfidence = in->getSentenceConfidence();
	out->wakeWordAccepted = in->getWakeWordAccepted();

	if (in->getMsgType() == AlibabaNls::NlsEvent::EventType::Binary)
	{
		//memset(out->binaryData, 0, 16384);
		std::vector<unsigned char> data = in->getBinaryData();
		int old_data = out->binaryDataSize;
		int data_size = data.size();
		if (data_size > 0)
		{
			memcpy(&out->binaryData[old_data], (char*)&data[0], data_size);
			out->binaryDataSize = old_data + data_size;
		}
	}
	else
	{
		//out->binaryDataSize = 0;
	}

	const char* getAllResponse = in->getAllResponse();
	if (getAllResponse) {
		str_len = strnlen(getAllResponse, 8192);
		memcpy(out->msg, getAllResponse, str_len);
	}

	out->stashResultSentenceId = in->getStashResultSentenceId();
	out->stashResultBeginTime = in->getStashResultBeginTime();

	const char* getStashResultText = in->getStashResultText();
	if (getStashResultText) {
		str_len = strnlen(getStashResultText, 8192);
		memcpy(out->stashResultText, getStashResultText, str_len);
	}

	out->stashResultCurrentTime = in->getStashResultCurrentTime();
	out->isValid = true;

	ReleaseMutex(out->eventMtx);
	return;
}

struct UserCallback
{
	NlsCallbackDelegate delegate_callback;
	void* user_handler;
};

#endif // _NLSCPPSDK_EVENT_STRUCT_H_
