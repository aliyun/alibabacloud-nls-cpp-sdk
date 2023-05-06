#ifndef NLSANDROIDSDK_NLSREQUESTWARPPER_H
#define NLSANDROIDSDK_NLSREQUESTWARPPER_H

#include <jni.h>
#include <map>
#include <pthread.h>
#include "nlsClient.h"

class NlsRequestWarpper {
public:
    static std::map <void*, bool> _requestMap;
    static pthread_mutex_t _global_mtx;// = PTHREAD_MUTEX_INITIALIZER;

//    NlsRequestWarpper(jobject obj, pthread_mutex_t* mtx);
    NlsRequestWarpper(jobject obj);
    ~NlsRequestWarpper();
    JavaVM* _jvm;
    jobject _callback;

//    pthread_mutex_t* _release_lock;
};

#endif //NLSANDROIDSDK_NLSREQUESTWARPPER_H
