/*
 * Copyright 2015 Alibaba Group Holding Limited
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

#include <unistd.h>
#include <sys/select.h>
#include "iosSsl.h"
#include "log.h"

namespace AlibabaNls {
//namespace transport {

using std::string;
using namespace utility;

#define ioErr -36
int SslConnect::sslWaitRead(handle_t socketFd) {
    fd_set readFd;
    struct timeval timeout = {0, 1000};
    FD_ZERO(&readFd);
    FD_SET(socketFd, &readFd);

    select(socketFd + 1, &readFd, NULL, NULL, &timeout);

    return 0;
}

int SslConnect::sslWaitWrite(handle_t socketFd) {
    fd_set writeFd;
    struct timeval timeout = {0, 1000};
    FD_ZERO(&writeFd);
    FD_SET(socketFd, &writeFd);

    select(socketFd + 1, &writeFd, NULL, NULL, &timeout);

    return 0;
}

OSStatus SslConnect::sslReadCallBack(SSLConnectionRef ctxHandle, void *data, size_t *dataLen) {
	sslCustomParam_t *handle = (sslCustomParam_t *) ctxHandle;
	size_t tmpLen = *dataLen;
	char *tmpBuff = (char *)data;

	if(*dataLen == 0) {
		return noErr;
	}

	OSStatus retValue = noErr;
	int theErr;
	ssize_t ret;
	while(tmpLen) {
		ret = read(handle->socketFd, tmpBuff, tmpLen);
		if (ret == 0) {
			return errSSLClosedGraceful;
		} else if (ret < 0) {
//			LOG_DEBUG("IOS SSL: sslReadCallBack status: %zd.", ret);
			theErr = errno;
				switch (theErr) {
					case ENOENT:
						/* connection closed */
						retValue = errSSLClosedGraceful;
						break;
					case ECONNRESET:
						retValue = errSSLClosedAbort;
						break;
					case EAGAIN:
						retValue = errSSLWouldBlock;
                        sslWaitRead(handle->socketFd);
						break;
					default:
						LOG_ERROR("try to read %zu bytes, " "got error %d", tmpLen, errno);
						retValue = ioErr;
						break;
				}
			break;
		} else {
			tmpBuff += ret;
		    tmpLen -= ret;
		}
	}

//	LOG_DEBUG("sslReadCallBack: %d %d %d %d.", ret, *dataLen, *dataLen - tmpLen, retValue);
	*dataLen -= tmpLen;

	return retValue;
}

OSStatus SslConnect::sslWriteCallBack(SSLConnectionRef ctxHandle, const void *data, size_t *dataLen) {
    sslCustomParam_t *handle = (sslCustomParam_t *) ctxHandle;
    size_t tmpLen = *dataLen;
    const char *tmpBuff = (const char *)data;
	OSStatus retValue = noErr;
	int theErr;
	ssize_t ret;

	if(*dataLen == 0) {
		return noErr;
	}

	while(tmpLen) {
        ret = write(handle->socketFd, tmpBuff, tmpLen);
        if (ret == 0) {
            return errSSLClosedGraceful;
        } else if (ret < 0) {
//            LOG_DEBUG("IOS SSL: sslWriteCallBack status: %zd.", ret);
			theErr = errno;
			switch (theErr) {
				case EAGAIN:
					retValue = errSSLWouldBlock;
                    sslWaitWrite(handle->socketFd);
					break;

				case EPIPE:
				case ECONNRESET:
					retValue = errSSLClosedAbort;
					break;

				default:
					LOG_ERROR("error while writing: %d", errno);
					retValue = ioErr;
					break;
			}

            break;//return retValue;
        } else {
            tmpBuff += ret;
            tmpLen -= ret;
        }
    }

//	LOG_DEBUG("sslWriteCallBack: %d  %d %d.", ret, *dataLen, *dataLen - tmpLen, retValue);
	*dataLen -= tmpLen;

    return retValue;
}

SslConnect::SslConnect() {
    _ctxHandle = NULL;
	pthread_mutex_init(&_mtxSsl, NULL);
}

SslConnect::~SslConnect() {
	pthread_mutex_destroy(&_mtxSsl);
//	LOG_DEBUG("IOS SSL: Ssl connect close.");

	if (_ctxHandle) {
//		SSLClose(_ctxHandle);
		CFRelease(_ctxHandle);
		_ctxHandle = NULL;
	}

}

int SslConnect::sslHandshake(int socketFd,  const char* hostname) {
    OSStatus retStatus = 0;
    char tmpInfo[128] = {0};
	
//    LOG_DEBUG("connect socketFd: %d",  _iosSslParam.socketFd);

    if (!_ctxHandle) {
        _iosSslParam.socketFd = socketFd;
        _ctxHandle = SSLCreateContext(NULL, kSSLClientSide, kSSLStreamType);
        if (!_ctxHandle) {
            strcpy(tmpInfo, "IOS SSL: Create context Failed, there was insufficient heap space.");
            LOG_ERROR(tmpInfo);
            return -1;
        }

        (void)SSLSetProtocolVersionMin(_ctxHandle, kTLSProtocol1);
        (void)SSLSetProtocolVersionMax(_ctxHandle, kTLSProtocol12);

        retStatus = SSLSetIOFuncs(_ctxHandle, sslReadCallBack, sslWriteCallBack);
        if (retStatus != noErr) {
            goto Failed;
        }
        
        retStatus = SSLSetConnection(_ctxHandle, &_iosSslParam);
        if (retStatus != noErr) {
            goto Failed;
        }
        
        retStatus = SSLSetSessionOption(_ctxHandle, kSSLSessionOptionBreakOnServerAuth, true);
        if (retStatus != noErr) {
            goto Failed;
        }
        
        LOG_DEBUG("Hostname:%s.", hostname);
        retStatus = SSLSetPeerDomainName(_ctxHandle, hostname, strlen(hostname));
        if (retStatus != noErr) {
            goto Failed;
        }
    }
    
	retStatus = SSLHandshake(_ctxHandle);
	if (retStatus == noErr) {
		LOG_DEBUG("IOS SSL: Handshake success.");
		return 0;
	} else {
		if ((retStatus == errSSLServerAuthCompleted) || (retStatus == errSSLWouldBlock)) {
//			LOG_DEBUG("IOS SSL: Handshake continue: %d.", retStatus);
			return retStatus;
//			continue;
		} else {
			goto Failed;
		}
	}

//	while(1) {
//		retStatus = SSLHandshake(_ctxHandle);
//		if (retStatus == noErr) {
//            LOG_DEBUG("IOS SSL: Handshake success.");
//		    break;
//		} else {
//			if ((retStatus == errSSLServerAuthCompleted) || (retStatus == errSSLWouldBlock)) {
//                LOG_DEBUG("IOS SSL: Handshake continue: %d.", retStatus);
//			    continue;
//			}
//            goto Failed;
//		}
//	}

//	return 0;

Failed:
    snprintf(tmpInfo, 128, "IOS sslHandshake failed. System error code: %d.", (int)retStatus);
	_errorMsg = tmpInfo;
    LOG_ERROR(tmpInfo);

	CFRelease(_ctxHandle);
    _ctxHandle = NULL;
    return -1;
}

ssize_t SslConnect::sslWrite(const uint8_t * buffer, size_t buffLen) {
	size_t writeLen = -1;
	OSStatus ret = noErr;
	size_t actualSize = 0;
	size_t tLen = buffLen;
	void * tBuff = (void *)buffer;

	if (buffer == NULL) {
		return -1;
	}

	pthread_mutex_lock(&_mtxSsl);
	do {
		ret = SSLWrite(_ctxHandle, tBuff, tLen, &actualSize);
		if (ret == noErr) {
			/* actualSize remains zero because no new data send */
			break;
		} else if (ret == errSSLWouldBlock) {
			tBuff = NULL;
			tLen = 0;
			continue;
		} else {
			LOG_DEBUG("SSLWrite failed: %d", ret);
			char tmpInfo[128] = {0};
			snprintf(tmpInfo, 128, "SSLWrite failed. System error code: %d.", (int)ret);
			_errorMsg = tmpInfo;
//			LOG_ERROR(tmpInfo);

			pthread_mutex_unlock(&_mtxSsl);
			return -1;
		}
	} while(1);
	pthread_mutex_unlock(&_mtxSsl);

	return actualSize;
}

ssize_t SslConnect::sslRead(uint8_t * buffer, size_t buffLen) {
	size_t readLen = -1;

	if (buffer == NULL) {
		return -1;
	}

	pthread_mutex_lock(&_mtxSsl);

	size_t actualSize;
	OSStatus ret = SSLRead(_ctxHandle, (void *)buffer, buffLen, &actualSize);
	if (ret == noErr) {
		pthread_mutex_unlock(&_mtxSsl);
		return actualSize;
	}

	if (ret == errSSLWouldBlock) {
		if (actualSize) {
			pthread_mutex_unlock(&_mtxSsl);
			return actualSize;
		}
		pthread_mutex_unlock(&_mtxSsl);
		return 0;
	}

	LOG_DEBUG("SSLRead failed: %d", ret);
	char tmpInfo[128] = {0};
	snprintf(tmpInfo, 128, "SSLRead failed. System error code: %d.", (int)ret);
	_errorMsg = tmpInfo;

	/* peer performed shutdown */
	if (ret == errSSLClosedNoNotify || ret == errSSLClosedGraceful) {
		LOG_DEBUG("Got close notification with code %i", (int)ret);
		pthread_mutex_unlock(&_mtxSsl);
		return -1;
	}

	pthread_mutex_unlock(&_mtxSsl);

	return -1;
}

void SslConnect::sslClose() {
    if (_ctxHandle) {
        SSLClose(_ctxHandle);
//        CFRelease(_ctxHandle);
//        _ctxHandle = NULL;
	}
}

const char* SslConnect::getFailedMsg() {
	return _errorMsg.c_str();
}

//}

}

