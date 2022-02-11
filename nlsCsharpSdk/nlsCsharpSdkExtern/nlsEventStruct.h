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
	char msg[8192];
	int msgType;
	char taskId[128];
	char result[8192];
	char displayText[8192];
	char spokenText[8192];
	int sentenceTimeOutStatus;
	int sentenceIndex;
	int sentenceTime;
	int sentenceBeginTime;
	double sentenceConfidence;
	bool wakeWordAccepted;
	bool wakeWordKnown;
	char wakeWordUserId[128];
	int wakeWordGender;

	int stashResultSentenceId;
	int stashResultBeginTime;
	char stashResultText[8192];
	int stashResultCurrentTime;

	bool isValid;
	HANDLE eventMtx;
};

static NLS_EVENT_STRUCT* srEvent = NULL;
static NLS_EVENT_STRUCT* stEvent = NULL;
static NLS_EVENT_STRUCT* syEvent = NULL;

static void CleanNlsEvent(NLS_EVENT_STRUCT* e)
{
	WaitForSingleObject(e->eventMtx, INFINITE);

	memset(e, 0, sizeof(NLS_EVENT_STRUCT));

	ReleaseMutex(e->eventMtx);
	return;
}

static void ConvertNlsEvent(AlibabaNls::NlsEvent* in, NLS_EVENT_STRUCT* out)
{
	WaitForSingleObject(out->eventMtx, INFINITE);

	out->statusCode = in->getStatusCode();
	out->msgType = (int)in->getMsgType();
	if (in->getTaskId()) strncpy(out->taskId, in->getTaskId(), 128);
	if (in->getResult()) strncpy(out->result, in->getResult(), 8192);
	if (in->getDisplayText()) strncpy(out->displayText, in->getDisplayText(), 8192);
	if (in->getSpokenText()) strncpy(out->spokenText, in->getSpokenText(), 8192);
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

	if (in->getAllResponse()) strncpy(out->msg, in->getAllResponse(), 8192);

	out->stashResultSentenceId = in->getStashResultSentenceId();
	out->stashResultBeginTime = in->getStashResultBeginTime();
	if (in->getStashResultText()) strncpy(out->stashResultText, in->getStashResultText(), 8192);
	out->stashResultCurrentTime = in->getStashResultCurrentTime();

	out->isValid = true;

	ReleaseMutex(out->eventMtx);
	return;
}



#endif // _NLSCPPSDK_EVENT_STRUCT_H_
