#include <jni.h>
#include <string>
#include <cstdlib>
#include <vector>
#include "native-lib.h"
#include "nlsClient.h"
#include "nlsEvent.h"
#include "sr/speechRecognizerRequest.h"
#include "st/speechTranscriberRequest.h"
#include "sy/speechSynthesizerRequest.h"
#include "log.h"
#include "NlsRequestWarpper.h"

using namespace AlibabaNls;
using namespace utility;

NlsClient* gnlsClient = NULL;

extern "C"
{
JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_NlsClient_releaseClient(JNIEnv *env, jobject instance);

}

//void FreeNlsRequestWarpper(NlsRequestWarpper* tst) {
//    if (!tst) {
//        return;
//    }
//
//    if (tst->_release) {
//        LOG_DEBUG("Free NlsRequestWarpper done.");
//        delete tst;
//        tst = NULL;
//    }
//
//    return ;
//}

void OnTaskFailed(NlsEvent* str, void* para) {
    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

//    LOG_DEBUG("JJJJJJNI OnTaskFailed");

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if (tst == NULL || tst->_callback == NULL) {
        LOG_INFO("OnTaskFailed is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if(env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onTaskFailed","(Ljava/lang/String;I)V");

//    jstring msg = env->NewStringUTF(str->getAllResponse());

    jstring msg;
    const char* tmp_failed = "{\"TaskFailed\": \"Default Error.\"}";
    if (CheckUtfString(str->getAllResponse()) == 0) {
        msg = env->NewStringUTF(str->getAllResponse());
    } else {
        msg = env->NewStringUTF(tmp_failed);
        LOG_INFO("TTTT is not UTF8.");
    }

    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef(msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
//    LOG_DEBUG("JJJJJJNI End OnTaskFailed");
}

void OnChannelClosed(NlsEvent* str, void* para) {

//    LOG_DEBUG("JJJJJJNI OnChannelClosed");

    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if (tst == NULL || tst->_callback == NULL) {
        LOG_INFO("OnChannelClosed is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if(env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

//    LOG_DEBUG("JJJJJJNI Find OnChannelClosed");

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz, "onChannelClosed", "(Ljava/lang/String;I)V");
    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback, methodID_func, msg, code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef(msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    if (tst != NULL) {
        delete tst;
        tst = NULL;

        LOG_DEBUG("JJJJJJNI Delete wrapper.");
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

//    LOG_DEBUG("JJJJJJNI End OnChannelClosed");
}

void OnRecognizerStarted(NlsEvent* str, void* para) {

//    LOG_DEBUG("JJJJJJNI OnRecognizerStarted");

    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if (tst == NULL || tst->_callback == NULL) {
        LOG_INFO("OnRecognizerStarted is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if (env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

//    LOG_DEBUG("JJJJJJNI Find OnRecognizerStarted");

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    if (native_clazz == 0) {
//        LOG_DEBUG("JJJJJJNI Unable to find class");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jmethodID methodID_func= env->GetMethodID(native_clazz, "onRecognizedStarted", "(Ljava/lang/String;I)V");
    if (methodID_func == NULL) {
//        LOG_DEBUG("JJJJJJNI Unable to find method:onRecognizedStarted");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef(msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

//    LOG_DEBUG("JJJJJJNI End OnRecognizerStarted");
}

void OnRecognizerCompleted(NlsEvent* str, void* para) {

//    LOG_DEBUG("JJJJJJNI OnRecognizerCompleted");

    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if (tst == NULL || tst->_callback == NULL) {
        LOG_INFO("OnRecognizerCompleted is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if (env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

//    LOG_DEBUG("JJJJJJNI Find OnRecognizerCompleted");

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    if (native_clazz == 0) {
//        LOG_DEBUG("JJJJJJNI Unable to find Completed class");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jmethodID methodID_func= env->GetMethodID(native_clazz,"onRecognizedCompleted","(Ljava/lang/String;I)V");
    if (methodID_func == NULL) {
//        LOG_DEBUG("JJJJJJNI Unable to find method:onRecognizedCompleted");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef( msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

//    LOG_DEBUG("JJJJJJNI End OnRecognizerCompleted");
}

void OnRecognizedResultChanged(NlsEvent* str, void* para) {

//    LOG_DEBUG("JJJJJJNI OnRecognizedResultChanged");

    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if (tst == NULL || tst->_callback == NULL) {
        LOG_INFO("OnRecognizedResultChanged is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0)
            {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if (env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

//    LOG_DEBUG("JJJJJJNI Find OnRecognizedResultChanged");

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onRecognizedResultChanged","(Ljava/lang/String;I)V");
    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef(msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

//    LOG_DEBUG("JJJJJJNI End OnRecognizedResultChanged");
}

void OnDialogResultGenerated(NlsEvent* str, void* para) {

//    LOG_DEBUG("JJJJJJNI OnDialogResultGenerated");

    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if (tst == NULL || tst->_callback == NULL) {
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        LOG_INFO("OnDialogResultGenerated is free.");
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if (env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

//    LOG_DEBUG("JJJJJJNI Find OnDialogResultGenerated");

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onDialogResultGenerated","(Ljava/lang/String;I)V");
    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef(msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

//    LOG_DEBUG("JJJJJJNI End OnDialogResultGenerated");
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_NlsClient_initClient(JNIEnv *env, jobject instance, jboolean isInitialSsl){
    if (gnlsClient == NULL) {
        gnlsClient = NlsClient::getInstance(isInitialSsl);
        gnlsClient->setLogConfig(NULL, LogDebug);
        gnlsClient->getInstance()->startWorkThread(1);
    }

    return (jlong) gnlsClient;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_NlsClient_logConfig(JNIEnv *env, jobject instance, jstring logfilename_) {
    const char * logfilename = NULL;

    if (logfilename_ != NULL) {
        logfilename = env->GetStringUTFChars(logfilename_, 0);
    }

    int ret = -1;
    if (gnlsClient != NULL) {
        ret = gnlsClient->setLogConfig(logfilename, LogDebug);
    }

    if (logfilename != NULL) {
        env->ReleaseStringUTFChars(logfilename_, logfilename);
    }

    return ret;
}

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_NlsClient_releaseClient(JNIEnv *env, jobject instance){
//    if(gnlsClient != NULL)
//        gnlsClient->releaseInstance();
//    gnlsClient = NULL;
}

int CheckUtfString(const char* bytes) {
    const char* origBytes = bytes;
    if (bytes == NULL) {
        return -1;
    }

    while (*bytes != '\0') {
        unsigned char utf8 = *(bytes++);
        // Switch on the high four bits.
        switch (utf8 >> 4) {
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07: {
                // Bit pattern 0xxx. No need for any extra bytes.
                break;
            }
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0f: {
                /*printf("****JNI WARNING: illegal start byte 0x%x\n", utf8);*/
                return -1;
            }
            case 0x0e: {
                // Bit pattern 1110, so there are two additional bytes.
                utf8 = *(bytes++);
                if ((utf8 & 0xc0) != 0x80) {
                    /*printf("****JNI WARNING: illegal continuation byte 0x%x\n", utf8);*/
                    return -1;
                }
                // Fall through to take care of the final byte.
            }
            case 0x0c:
            case 0x0d: {
                // Bit pattern 110x, so there is one additional byte.
                utf8 = *(bytes++);
                if ((utf8 & 0xc0) != 0x80) {
                    /*printf("****JNI WARNING: illegal continuation byte 0x%x\n", utf8);*/
                    return -1;
                }
                break;
            }
        }
    }
    return 0;
}
