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

#include "socket.h"

#ifdef __GNUC__
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#include <cstring>
#include <sstream>

namespace AlibabaNls {
namespace transport {

#ifdef _WIN32
#pragma comment(lib, "ws2_32")
#endif

#ifdef __GNUC__
#define MAKEWORD(a, b) 0
#define WSAStartup(a, b) 0
#define WSAECONNRESET 0
#define WSAECONNABORTED 0
#define WSAENOTCONN 0
#define WSAEINPROGRESS 0
#define WSADATA int
#define ioctlsocket ioctl
#endif

using namespace util;

#ifdef XP
static const int inet_ptons(int af, const char *csrc, void *dst) {
    char * src;

    if (csrc == NULL || (src = strdup(csrc)) == NULL) {
        _set_errno(ENOMEM);
        return 0;
    }

    switch (af) {
        case AF_INET: {
            struct sockaddr_in  si4;
            INT r;
            INT s = sizeof(si4);

            si4.sin_family = AF_INET;
            r = WSAStringToAddress(src, AF_INET, NULL, (LPSOCKADDR)&si4, &s);
            free(src);
            src = NULL;

            if (r == 0) {
                memcpy(dst, &si4.sin_addr, sizeof(si4.sin_addr));
                return 1;
            }
        }
        break;

        case AF_INET6: {
            struct sockaddr_in6 si6;
            INT r;
            INT s = sizeof(si6);

            si6.sin6_family = AF_INET6;
            r = WSAStringToAddress(src, AF_INET6, NULL, (LPSOCKADDR)&si6, &s);
            free(src);
            src = NULL;

            if (r == 0) {
                memcpy(dst, &si6.sin6_addr, sizeof(si6.sin6_addr));
                return 1;
            }
        }
        break;

        default:
            _set_errno(97);
            return -1;
    }

    /* the call failed */
    {
        int le = WSAGetLastError();

        if (le == WSAEINVAL) {
            return 0;
        }

        _set_errno(le);
        return -1;
    }
}

#endif

#if defined(__ANDROID__) || defined(__linux__)
pthread_mutex_t InetAddress::_mtxDns = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  InetAddress::_cvDns = PTHREAD_COND_INITIALIZER;
std::string InetAddress::_resolvedDns = "";

static void *async_dns_resolve_thread_fn(void * arg) {

    std::string host_name = (const char*)arg;
    char dns_buff[8192] = {0};
    struct hostent hostinfo, *phost;
	int rc = 0;
    std::string tmpResolvedDns;

    if (0 == gethostbyname_r(host_name.c_str(), &hostinfo, dns_buff, 8192, &phost, &rc) && phost != NULL) {
        tmpResolvedDns = inet_ntoa(*((in_addr *) phost->h_addr_list[0]));
	} else {
		LOG_ERROR("gethostbyname_r error: %s", gai_strerror(rc));

        return NULL;
	}

    pthread_mutex_lock(&InetAddress::_mtxDns);
    InetAddress::_resolvedDns.clear();
    InetAddress::_resolvedDns = tmpResolvedDns;
    pthread_cond_signal(&InetAddress::_cvDns);
    pthread_mutex_unlock(&InetAddress::_mtxDns);

    return NULL;
}
#endif


bool InetAddress::GetInetAddressByHostname(const std::string hostname, std::string &ip, int &aiFamily, std::string &errorMsg) {

    if (hostname.empty()) {
        errorMsg = "URL: hostname is empty.";
        LOG_ERROR("URL: hostname is empty.");
	    return false;
    }

#if defined(_WIN32)
    struct addrinfo	hints, *res;
    int	error = 0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    error = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
    if (error){
        errorMsg = gai_strerrorA(error);
        LOG_ERROR("DNS: error: %s", errorMsg.c_str());
        return false;
    }

	char ipTmp[MAX_HOST_IP_LENGTH] = {0};
    if (res != NULL) {
        if (AF_INET == res->ai_family) {
            struct sockaddr_in* in;
            in = (struct sockaddr_in *)(res->ai_addr);
            inet_ntop(AF_INET, &in->sin_addr, ipTmp, sizeof(ipTmp));
            ip = ipTmp;
            aiFamily = AF_INET;
        }
        else if (AF_INET6 == res->ai_family) {
            struct sockaddr_in6* in6;
            in6 = (struct sockaddr_in6 *)(res->ai_addr);
            inet_ntop(AF_INET6, &in6->sin6_addr, ipTmp, sizeof(ipTmp));
            ip = ipTmp;
            aiFamily = AF_INET6;
        }
        else {
            LOG_ERROR("the host name is neither ipv4 or ipv6");
            freeaddrinfo(res);
            return false;
        }
    }

	freeaddrinfo(res);

#elif defined(__APPLE__)
    struct addrinfo	hints, *res;
    int	error = 0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    error = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
    if (error){
        errorMsg = gai_strerror(error);
        LOG_ERROR("DNS: error: %s", errorMsg.c_str());
        return false;
    }

    char ipTmp[MAX_HOST_IP_LENGTH] = {0};
    if (res != NULL) {
        if (AF_INET == res->ai_family) {
            struct sockaddr_in* in;
            in = (struct sockaddr_in *)(res->ai_addr);
            inet_ntop(AF_INET, &in->sin_addr, ipTmp, sizeof(ipTmp));
            ip = ipTmp;
            aiFamily = AF_INET;
        }
        else if (AF_INET6 == res->ai_family) {
            struct sockaddr_in6* in6;
            in6 = (struct sockaddr_in6 *)(res->ai_addr);
            inet_ntop(AF_INET6, &in6->sin6_addr, ipTmp, sizeof(ipTmp));
            ip = ipTmp;
            aiFamily = AF_INET6;
        }
        else {
            LOG_ERROR("the host name is neither ipv4 or ipv6");
            freeaddrinfo(res);
            return false;
        }
    }

    freeaddrinfo(res);

#elif defined(__ANDROID__) || defined(__linux__)

    bool tmpResolveResult = false;
    struct timeval now;
    struct timespec outtime;
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + 2;
    outtime.tv_nsec = now.tv_usec * 1000;

    pthread_t dnsThread;
    pthread_create(&dnsThread, NULL, &async_dns_resolve_thread_fn, (void*)hostname.c_str());
    pthread_detach(dnsThread);

    pthread_mutex_lock(&_mtxDns);
    LOG_DEBUG("resolved_dns Wait.");
    if (ETIMEDOUT == pthread_cond_timedwait(&_cvDns, &_mtxDns, &outtime)) {
        errorMsg = "DNS: resolved timeout.";
        LOG_ERROR("DNS: resolved timeout.");
    } else {
        aiFamily = AF_INET;
        ip = _resolvedDns;
        tmpResolveResult = true;
    }
    pthread_mutex_unlock(&_mtxDns);

    LOG_DEBUG("resolve dns done _resolveResult=%d", tmpResolveResult);

    return tmpResolveResult;

#endif

	return true;
}

//size_t InetAddress::HashCode() const {
//	size_t h = 0;
//	std::string hostPort = this->ToString();
//	std::string::const_iterator p, p_end;
//	for (p = hostPort.begin(), p_end = hostPort.end(); p != p_end; ++p) {
//		h = 31 * h + (*p);
//	}
//	return h;
//}

//std::string InetAddress::ToString() const {
//	char hostPort[MAX_HOST_PORT_LENGTH], host[MAX_HOST_IP_LENGTH]; // TODO:change number to const.
//	uint16_t port;
//
//	inet_ntop(AF_INET, (void *) &_addr.sin_addr, host, sizeof(host));
//    port = ntohs(_addr.sin_port);
//    sprintf(hostPort, "%s:%u", host, port);
//
//    return hostPort;
//}

//uint16_t InetAddress::getPort() {
//	return ntohs(_addr.sin_port);
//}

//void InetAddress::setPort(uint16_t port) {
//    _addr.sin_port = htons(port);
//}

//void InetAddress::setIpAdress(const std::string &ip) {
//#ifdef XP
//    if (inet_ptons(AF_INET, ip.c_str(), &m_addr.sin_addr) <= 0)
//#else
//    if (inet_pton(AF_INET, ip.c_str(), &_addr.sin_addr) <= 0)
//#endif // XP
//    {
//        throw ExceptionWithString("IP: ip address is not valid.", Socket::getLastErrorCode());
//    }
//}

//void InetAddress::setAddr(const struct sockaddr_in &addr) {
//    _addr = addr;
//}

const struct sockaddr_in &InetAddress::getIpv4Addr() const {
    return _addr;
}

const struct sockaddr_in6 &InetAddress::getIpv6Addr() const {
    return _addr6;
}

const int InetAddress::getAiFamily() const {
    return _aiFamily;
}

//InetAddress::InetAddress(const struct sockaddr_in &addr) : _addr(addr) {
//
//}

InetAddress::InetAddress(const std::string &ip, int aiFamily, uint16_t port) {
    _aiFamily = aiFamily;

    if (AF_INET == aiFamily) {
        memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(port);
#ifdef XP
        if (inet_ptons(AF_INET, ip.c_str(), &_addr.sin_addr) <= 0)
#else
        if (inet_pton(AF_INET, ip.c_str(), &_addr.sin_addr) <= 0)
#endif // XP
            throw ExceptionWithString("IP: ip address is not valid.", Socket::getLastErrorCode());
    }
    else if (AF_INET6 == aiFamily) {
        memset(&_addr6, 0, sizeof(_addr6));
        _addr6.sin6_family = AF_INET6;
        _addr6.sin6_port = htons(port);
#ifdef XP
        if (inet_ptons(AF_INET6, ip.c_str(), &_addr6.sin6_addr) <= 0)
#else
        if (inet_pton(AF_INET6, ip.c_str(), &_addr6.sin6_addr) <= 0)
#endif // XP
            throw ExceptionWithString("IP: ip address is not valid.", Socket::getLastErrorCode());
    }

    
}

//InetAddress::InetAddress(uint16_t port) {
//    memset(&_addr, 0, sizeof(_addr));
//    _addr.sin_family = AF_INET;
//    _addr.sin_port = htons(port);
//    _addr.sin_addr.s_addr = htonl(INADDR_ANY);
//}

//InetAddress::InetAddress() {
//    memset(&_addr, 0, sizeof(_addr));
//    _addr.sin_family = AF_INET;
//    _addr.sin_port = htons(0);
//    _addr.sin_addr.s_addr = htonl(INADDR_ANY);
//}

void SocketFuncs::GetSocketOption(SOCKET sockfd, int level, int optName, socklen_t *optVal) {
    socklen_t optLen = sizeof(int);
    ENSURE(SOCKET_ERROR != getsockopt(sockfd, level, optName, (char *) optVal, &optLen))(WSAGetLastError());
}

void SocketFuncs::GetSocketOption(SOCKET sockfd, int level, int optName, char *optVal, socklen_t *optLen) {
    ENSURE(SOCKET_ERROR != getsockopt(sockfd, level, optName, optVal, optLen))(WSAGetLastError());
}

void SocketFuncs::SetSocketOption(SOCKET sockfd, int level, int optName, const char *optVal, int optLen) {
    ENSURE(SOCKET_ERROR != setsockopt(sockfd, level, optName, optVal, optLen))(WSAGetLastError());
}

void SocketFuncs::SetSocketOption(SOCKET sockfd, int level, int optName, int optVal) {
    ENSURE(SOCKET_ERROR != setsockopt(sockfd, level, optName, (char *) &optVal, sizeof(optVal)))(WSAGetLastError());
}

//unsigned int SocketFuncs::GetBindedPort(SOCKET sockfd) {
//    struct sockaddr_in sin;
//    socklen_t addrlen = sizeof(sin);
//    if (getsockname(sockfd, (struct sockaddr *) &sin, &addrlen) == 0 &&
//        sin.sin_family == AF_INET &&
//        addrlen == sizeof(sin)) {
//
//        return ntohs(sin.sin_port);
//
//    }
//    return 0;
//}

bool SocketFuncs::Select(SOCKET maxsock, fd_set *pWrite, fd_set *pRead, int timeout) {
    struct timeval tTimeout;
    tTimeout.tv_sec = timeout / 1000;
    tTimeout.tv_usec = timeout % 1000 * 1000;

    int result = select((int) maxsock + 1, pRead, pWrite, NULL, &tTimeout);
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();

        ENSURE(WSAECONNRESET == error ||
               WSAECONNABORTED == error ||
               WSAENOTCONN == error ||
               WSAEINPROGRESS == error)(error);
        return false;
    }
    return result > 0;
}

bool SocketFuncs::SelectWrite(SOCKET sockfd, int timeout) {
    fd_set writeFds;
    FD_ZERO(&writeFds);
    FD_SET(sockfd, &writeFds);
    return Select(sockfd, &writeFds, NULL, timeout);
}

bool SocketFuncs::SelectRead(SOCKET sockfd, int timeout) {
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(sockfd, &readFds);

    return Select(sockfd, NULL, &readFds, timeout);
}

void SocketFuncs::Listen(SOCKET sockfd, int connections) {
    ENSURE(SOCKET_ERROR != listen(sockfd, connections))(WSAGetLastError());
}

SOCKET SocketFuncs::Accept(SOCKET sockfd) {
    while (true) {
        SOCKET rt = accept(sockfd, NULL, 0);
        if (SOCKET_ERROR == rt) {
            int error = WSAGetLastError();
            if (error != WSAECONNRESET)
                throw ExceptionWithString("SocketFuncs::Accept aborted.", error);
            continue;
        }
        return rt;
    }
}

void SocketFuncs::Shutdown(SOCKET sockfd) {

    closesocket(sockfd);

    int result = shutdown(sockfd, /* SD_BOTH*/0x02);

    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        ENSURE(WSAECONNRESET == error
               || WSAECONNABORTED == error
               || WSAENOTCONN == error
               || WSAEINPROGRESS == error)(error);
    }
}

int SocketFuncs::connectTo(SOCKET sockfd, const InetAddress &addr) {
    if (AF_INET == addr.getAiFamily()) {
        const struct sockaddr_in &bindAddr = addr.getIpv4Addr();
        return connect(sockfd, (const sockaddr *)&bindAddr, sizeof(bindAddr));
    } else if (AF_INET6 == addr.getAiFamily()) {
        const struct sockaddr_in6 &bindAddr = addr.getIpv6Addr();
        return connect(sockfd, (const sockaddr *)&bindAddr, sizeof(bindAddr));
    }

    return -1;
}

void SocketFuncs::Bind(SOCKET sockfd, const InetAddress &addr) {
    if (AF_INET == addr.getAiFamily()) {
        const struct sockaddr_in &bindAddr = addr.getIpv4Addr();
        ENSURE(SOCKET_ERROR != bind(sockfd, (const sockaddr *)&bindAddr, sizeof(bindAddr)))(WSAGetLastError());
    }
    else if (AF_INET6 == addr.getAiFamily()) {
        const struct sockaddr_in6 &bindAddr = addr.getIpv6Addr();
        ENSURE(SOCKET_ERROR != bind(sockfd, (const sockaddr *)&bindAddr, sizeof(bindAddr)))(WSAGetLastError());
    }
}

void SocketFuncs::Startup() {
    WSADATA wsaData;
	ENSURE(WSAStartup(MAKEWORD(2, 0), &wsaData) != SOCKET_ERROR)(WSAGetLastError());
}

SocketFuncs::SocketFuncs() {

}

Socket::Socket(SmartHandle<SOCKET> sockfd, int timeOut) : _sockfd(sockfd),
                                                          WAITING_TIME(timeOut) {
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;

    if (SOCKET_ERROR == setsockopt(_sockfd.get(), SOL_SOCKET, SO_LINGER, (char *)&so_linger, sizeof(struct linger))) {
        LOG_ERROR("Set SO_LINGER error: %d.", Socket::getLastErrorCode());
    }

#if defined(__APPLE__)
    int optval = 1;
    if (SOCKET_ERROR == setsockopt(_sockfd.get(), SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval))) {
        LOG_ERROR("Set SO_NOSIGPIPE error: %d.", Socket::getLastErrorCode());
    }
#endif

#ifdef _WIN32
    int rvTimeout = 0;
    if (WAITING_TIME > 0) {
        rvTimeout = WAITING_TIME * 1000;

        if (SOCKET_ERROR == setsockopt(_sockfd.get(), SOL_SOCKET, SO_RCVTIMEO, (char *)&rvTimeout, sizeof(rvTimeout))) {
            LOG_ERROR("Set SO_RCVTIMEO error: %d.", Socket::getLastErrorCode());
        }
    }

    int sdTimeout = WEBSOCKET_DEFAULT_SEND_TIMEOUT * 1000;
	if (SOCKET_ERROR == setsockopt(_sockfd.get(), SOL_SOCKET, SO_SNDTIMEO, (char *)&sdTimeout, sizeof(sdTimeout))) {
		LOG_ERROR("Set SO_SNDTIMEO error: %d.", Socket::getLastErrorCode());
	}

#else

    struct timeval rvTimeout;
    if (WAITING_TIME > 0) {
        rvTimeout.tv_sec = WAITING_TIME;
        rvTimeout.tv_usec = 0;

        if (SOCKET_ERROR == setsockopt(_sockfd.get(), SOL_SOCKET, SO_RCVTIMEO, (char *)&rvTimeout, sizeof(rvTimeout))) {
            LOG_ERROR("Set SO_RCVTIMEO error: %d.", Socket::getLastErrorCode());
        }
    }

    struct timeval sdTimeout = { WEBSOCKET_DEFAULT_SEND_TIMEOUT, 0 };
    if (SOCKET_ERROR == setsockopt(_sockfd.get(), SOL_SOCKET, SO_SNDTIMEO, (char *)&sdTimeout, sizeof(sdTimeout))) {
        LOG_ERROR("Set SO_SNDTIMEO error: %d.", Socket::getLastErrorCode());
    }

#endif

}

int Socket::SetSocketRecvTimeOut(int timeOut) {

    if (_sockfd.get() != INVALID_SOCKET) {
#ifdef _WIN32

        int rvTimeout = timeOut * 1000;
        if (SOCKET_ERROR == setsockopt(_sockfd.get(), SOL_SOCKET, SO_RCVTIMEO, (char *)&rvTimeout, sizeof(rvTimeout))) {
            LOG_ERROR("Set SO_RCVTIMEO error: %d", Socket::getLastErrorCode());
            return -1;
        }
#else
        struct timeval rvTimeout = {timeOut, 0};
        if (SOCKET_ERROR == setsockopt(_sockfd.get(), SOL_SOCKET, SO_RCVTIMEO, (char *) &rvTimeout, sizeof(rvTimeout))) {
            LOG_ERROR("Set SO_RCVTIMEO error: %d", Socket::getLastErrorCode());

            return -1;
        }
#endif
        return 0;
    }

    return -1;
}

//void Socket::GetPeerAddress(InetAddress *address) {
//    sockaddr add;
//    socklen_t len = sizeof(add);
//    ::getpeername(_sockfd.get(), &add, &len);
//    address->setAddr(*((sockaddr_in *) &add));
//}

void Socket::Shutdown() {
    try {
        SOCKET socketFd = _sockfd.get();
        if (socketFd == INVALID_SOCKET) {
            return ;
        } else {
            LOG_DEBUG("Begin close socketFd:%d.\n", (int)socketFd);
        }

        int result = closesocket(socketFd);

        if (result == SOCKET_ERROR) {

            int error = getLastErrorCode();
            LOG_ERROR("socketFd: %d, error: %d\n", (int)_sockfd.GetHandle(), error);

            ENSURE(WSAECONNRESET == error
                   || WSAECONNABORTED == error
                   || WSAENOTCONN == error
                   || WSAEINPROGRESS == error)(error);
        } else {
            _sockfd.ResetHandle();
            LOG_DEBUG("End close socketFd:%d.\n", (int)_sockfd.GetHandle());
        }
    } catch (...) {

    }
}

bool Socket::getEtryEagin(int errorCode) {
#ifdef __GNUC__
    return (errorCode == EAGAIN) || (errorCode == EWOULDBLOCK) || (errorCode == EINTR);
#else
    return (errorCode == WSAEWOULDBLOCK) || (errorCode == WSAEINTR);
#endif

}

int Socket::Recv(unsigned char *buffer, int bufLen) {
    SOCKET socketFd = _sockfd.get();
    if (socketFd != INVALID_SOCKET) {
        int recvCount = recv(socketFd, (char*)buffer, bufLen, 0);
//        if (recvCount <= 0) {
//            LOG_ERROR("Recv failed. fd: %d, recvCount: %d, errno: %d.\n", socketFd, recvCount, Socket::getLastErrorCode());
//        }

        return recvCount;
    } else {
        return -1;
    }
}

int Socket::CheckSocketReturn(int result) {
    if (SOCKET_ERROR == result) {
        int error = Socket::getLastErrorCode();
        std::ostringstream os;
        os << "socket encounter error: " << error << std::endl;
        throw ExceptionWithString(os.str(), error);
    }
    return result;
}


int Socket::Send(const unsigned char *buffer, int bufLen) {
    SOCKET socketFd = _sockfd.get();
	int sentCount = 0;
    int seekCount = 0;

    if (socketFd != INVALID_SOCKET) {

        do {
#if defined(__APPLE__)
		    sentCount = send(socketFd, (const char *)(buffer + seekCount), (bufLen - seekCount), 0);
#elif defined(_WIN32)
		    sentCount = send(socketFd, (const char *)(buffer + seekCount), (bufLen - seekCount), 0);
#else
            sentCount = send(socketFd, (const char *)(buffer + seekCount), (bufLen - seekCount), MSG_NOSIGNAL);
#endif

            if (SOCKET_ERROR == sentCount) {
		LOG_ERROR("socket send failed, error: %d\n", Socket::getLastErrorCode());
                return -1;
            } else {
                seekCount += sentCount;
            }
        }while(seekCount !=  bufLen);

        return (sentCount);
    } else {
        return -1;
    }

//    return CheckSocketReturn(sentCount);
}

void Socket::GetSocketOption(int level, int optName, socklen_t *optVal) {
    SocketFuncs::GetSocketOption(_sockfd.get(), level, optName, optVal);
}

void Socket::GetSocketOption(int level, int optName, char *optVal, socklen_t *optLen) {
    SocketFuncs::GetSocketOption(_sockfd.get(), level, optName, optVal, optLen);
}

int Socket::getLastErrorCode() {
#ifdef __GNUC__
    int error = errno;
#else
    int error = (WSAGetLastError());
#endif
    return error;
}

void Socket::SetSocketOption(int level, int optName, const char *optVal, socklen_t optLen) {
    SocketFuncs::SetSocketOption(_sockfd.get(), level, optName, optVal, optLen);
}

void Socket::SetSocketOption(int level, int optName, socklen_t optVal) {
    SocketFuncs::SetSocketOption(_sockfd.get(), level, optName, optVal);
}

Socket::~Socket() {
    Shutdown();
}
}
}
