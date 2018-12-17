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

#if defined(__APPLE__)

namespace AlibabaNls {
namespace transport {

using namespace util;

int IosSslConnect::sslWaitRead(handle_t socketFd) {
    fd_set readFd;
    FD_ZERO(&readFd);
    FD_SET(socketFd, &readFd);

    if (select(socketFd + 1, &readFd, NULL, NULL, NULL) <= 0) {
        LOG_ERROR("IOS SSL: sslWaitRead Failed.");
        return -1;
    } else {
        return 0;
    }
}

int IosSslConnect::sslWaitWrite(handle_t socketFd) {
    fd_set writeFd;
    FD_ZERO(&writeFd);
    FD_SET(socketFd, &writeFd);

    if (select(socketFd + 1, NULL, &writeFd, NULL, NULL) <= 0) {
        LOG_ERROR("IOS SSL: sslWaitWrite Failed.");
        return -1;
    } else {
        return 0;
    }
}

OSStatus IosSslConnect::sslReadCallBack(SSLConnectionRef ctxHandle, void *data, size_t *dataLen) {
	sslCustomParam_t *handle = (sslCustomParam_t *) ctxHandle;
	size_t tmpLen = *dataLen;
	char *tmpBuff = (char *)data;

	while(tmpLen) {
		ssize_t ret = read(handle->socketFd, tmpBuff, tmpLen);
		if (ret == 0) {
			return errSSLClosedGraceful;
		} else if (ret < 0) {
			LOG_DEBUG("IOS SSL: sslReadCallBack status: %zd.", ret);
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
				if (-1 == sslWaitRead(handle->socketFd)) {
					return errSSLClosedAbort;
				}
				continue;
			}

            LOG_ERROR("IOS SSL: sslReadCallBack failed: %zd.", ret);
			return errSSLClosedAbort;
		} else {
			tmpBuff += ret;
		    tmpLen -= ret;
		}
	}

	return noErr;
}

OSStatus IosSslConnect::sslWriteCallBack(SSLConnectionRef ctxHandle, const void *data, size_t *dataLen) {
    sslCustomParam_t *handle = (sslCustomParam_t *) ctxHandle;
    size_t tmpLen = *dataLen;
    const char *tmpBuff = (const char *)data;

    while(tmpLen) {
        ssize_t ret = write(handle->socketFd, tmpBuff, tmpLen);
        if (ret == 0) {
            return errSSLClosedGraceful;
        } else if (ret < 0) {
            LOG_ERROR("IOS SSL: sslWriteCallBack status: %zd.", ret);
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
                if (-1 == sslWaitWrite(handle->socketFd)) {
                    return errSSLClosedAbort;
                }
                continue;
            }

            LOG_ERROR("IOS SSL: sslWriteCallBack failed: %zd.", ret);

            return errSSLClosedAbort;
        } else {
            tmpBuff += ret;
            tmpLen -= ret;
        }
    }

    return noErr;
}

IosSslConnect::IosSslConnect() {
    _ctxHandle = NULL;
}

IosSslConnect::~IosSslConnect() {
    LOG_DEBUG("IOS SSL: Ssl close.");
    _ctxHandle = NULL;
}

SSLContextRef IosSslConnect::getCtxHandle() {
    return _ctxHandle;
}

char* IosSslConnect::getErrorMsg() {
    return _errorMsg;
}

void* IosSslConnect::iosSslHandshake(sslCustomParam_t* customParam, const char* hostname) {
	memset(_errorMsg, 0x0, ERROR_MESSAGE_LEN);

    _ctxHandle = SSLCreateContext(NULL, kSSLClientSide, kSSLStreamType);
	if (!_ctxHandle) {
        strcpy(_errorMsg, "IOS SSL: Create context Failed, there was insufficient heap space.");
		LOG_ERROR(_errorMsg);
		return NULL;
	}

	(void)SSLSetProtocolVersionMin(_ctxHandle, kTLSProtocol1);
	(void)SSLSetProtocolVersionMax(_ctxHandle, kTLSProtocol12);

	OSStatus retStatus = SSLSetIOFuncs(_ctxHandle, sslReadCallBack, sslWriteCallBack);
	if (retStatus != noErr) {
        snprintf(_errorMsg, 128, "IOS SSL: set IO functions failed. System error code: %d.", retStatus);
		LOG_ERROR(_errorMsg);
		goto Failed;
	}

	retStatus = SSLSetConnection(_ctxHandle, customParam);
	if (retStatus != noErr) {
		snprintf(_errorMsg, 128, "IOS SSL: set connection failed. System error code: %d.", retStatus);
		LOG_ERROR(_errorMsg);
		goto Failed;
	}

//	if (vpn_ws_conf.ssl_no_verify) {
//		retStatus = SSLSetSessionOption(ctxHandle, kSSLSessionOptionBreakOnServerAuth, true);
//		if (retStatus != noErr) {
//			vpn_ws_log("vpn_ws_ssl_handshake()/SSLSetSessionOption(): %d\n", retStatus);
//			goto Failed;
//		}
//	}

	retStatus = SSLSetSessionOption(_ctxHandle, kSSLSessionOptionBreakOnServerAuth, true);
	if (retStatus != noErr) {
		snprintf(_errorMsg, 128, "IOS SSL: set session option failed. System error code: %d.", retStatus);
		LOG_ERROR(_errorMsg);
		goto Failed;
	}

    LOG_DEBUG("Hostname:%s.", hostname);
	retStatus = SSLSetPeerDomainName(_ctxHandle, hostname, strlen(hostname));
	if (retStatus != noErr) {
		snprintf(_errorMsg, 128, "IOS SSL: set peer domain name failed. System error code: %d.", retStatus);
		LOG_ERROR(_errorMsg);
		goto Failed;
	}

	while(1) {
		retStatus = SSLHandshake(_ctxHandle);
		if (retStatus == noErr) {
            LOG_DEBUG("IOS SSL: Handshake success.");
		    break;
		} else {
			if ((retStatus == errSSLServerAuthCompleted) ||
			    (retStatus == errSSLWouldBlock)) {
			    LOG_DEBUG("IOS SSL: Handshake continue: %d.", retStatus);
			    continue;
			}
			snprintf(_errorMsg, 128, "IOS SSL: Handshake failed. System error code: %d.", retStatus);
			LOG_ERROR(_errorMsg);
            goto Failed;
		}
	}

	return _ctxHandle;

Failed:
	CFRelease(_ctxHandle);
    _ctxHandle = NULL;
    return NULL;
}

int IosSslConnect::iosSslWrite(SSLContextRef ctx, const void* buff, size_t buffLen) {
	size_t writeLen = -1;

	OSStatus retStatus = SSLWrite(ctx, buff, buffLen, &writeLen);
	if (writeLen != buffLen) {
		LOG_ERROR("IOS SSL: SSLWrite failed. System error code: %d.", retStatus);
		return -1;
	}

	if (retStatus == noErr) {
		return writeLen;
	}

	LOG_ERROR("IOS SSL: SSLWrite. System error code: %d.", retStatus);
	return -1;
}

ssize_t IosSslConnect::iosSslRead(SSLContextRef ctx, void* buff, size_t buffLen) {
	size_t readLen = -1;

	OSStatus retStatus = SSLRead(ctx, buff, buffLen, &readLen);

	if (retStatus == noErr) {
		return readLen;
	}

	if (retStatus == errSSLClosedGraceful) {
		return 0;
	}

	LOG_ERROR("IOS SSL: SSLRead. System error code: %d.", retStatus);
    return -1;
}

void IosSslConnect::iosSslClose() {
    if (_ctxHandle) {
        SSLClose(_ctxHandle);
        CFRelease(_ctxHandle);
        _ctxHandle = NULL;
	}
}

}
}

#endif

