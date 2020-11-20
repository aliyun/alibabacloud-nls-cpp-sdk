//
// Created by Administrator on 2018/6/24.
//

#ifndef NLSANDROIDSDK2_NATIVE_LIB_H
#define NLSANDROIDSDK2_NATIVE_LIB_H
#include "nlsClient.h"
#include "nlsEvent.h"

extern AlibabaNls::NlsClient* gnlsClient;

void OnTaskFailed(AlibabaNls::NlsEvent* str, void* para);
void OnChannelClosed(AlibabaNls::NlsEvent* str, void* para);

void OnRecognizerStarted(AlibabaNls::NlsEvent* str, void* para);
void OnRecognizerCompleted(AlibabaNls::NlsEvent* str, void* para);
void OnRecognizedResultChanged(AlibabaNls::NlsEvent* str, void* para);

void OnDialogResultGenerated(AlibabaNls::NlsEvent* str, void* para);

int CheckUtfString(const char* bytes);

#endif //NLSANDROIDSDK2_NATIVE_LIB_H
