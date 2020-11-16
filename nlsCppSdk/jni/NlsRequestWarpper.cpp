#include "NlsRequestWarpper.h"
#include "log.h"
using namespace AlibabaNls::utility;

//NlsRequestWarpper::NlsRequestWarpper() {
//
//    this->_callback = NULL;
//    this->_jvm = NULL;
//}

pthread_mutex_t NlsRequestWarpper::_global_mtx = PTHREAD_MUTEX_INITIALIZER;
std::map <void*, bool> NlsRequestWarpper::_requestMap;

NlsRequestWarpper::~NlsRequestWarpper() {
    JNIEnv* env = NULL;
    bool attached = false;

    if(this->_jvm != NULL) {
        switch (_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
            case JNI_OK:
                break;
            case JNI_EDETACHED:
                if (_jvm->AttachCurrentThread(&env, NULL)!=0) {
                    LOG_DEBUG("attach fail");
                    return;
                }
                attached = true;
                break;
            case JNI_EVERSION:
                LOG_DEBUG("Invalid java version");
                return;
        }
    }

    if(this->_callback != NULL) {
        if(env != NULL) {
            env->DeleteGlobalRef(this->_callback);
            LOG_DEBUG("delete callback global ref");
        }
        _callback = NULL;
        LOG_DEBUG("delete callback");
    }

    if (attached) {
        _jvm->DetachCurrentThread();
    }
}

//NlsRequestWarpper::NlsRequestWarpper(jobject obj, pthread_mutex_t* mtx) {
NlsRequestWarpper::NlsRequestWarpper(jobject obj) {
    _callback = obj;

//    _release_lock = mtx;

    LOG_DEBUG("JJJJJJNI NlsRequestWarpper Init callback");
}
