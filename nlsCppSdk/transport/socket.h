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

#ifndef NLS_SDK_TRANSPORT_SOCKET_H
#define NLS_SDK_TRANSPORT_SOCKET_H

#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>
#include "pthread.h"
#else
#include <netinet/in.h>
#include <sys/select.h>
#include <pthread.h>
#endif

#include <string>
#include "targetOs.h"
#include "smartHandle.h"

namespace AlibabaNls {
namespace transport {

#define WEBSOCKET_DEFAULT_SEND_TIMEOUT 3
#define WEBSOCKET_DEFAULT_RECV_TIMEOUT 12

class InetAddress {
public:
    //InetAddress();
    //explicit InetAddress(uint16_t port);
    InetAddress(const std::string &ip, int aiFamily, uint16_t port);
    //InetAddress(const struct sockaddr_in &addr);

    const struct sockaddr_in &getIpv4Addr() const;
    const struct sockaddr_in6 &getIpv6Addr() const;
    const int getAiFamily() const;
    //void setAddr(const struct sockaddr_in &addr);
    //void setIpAdress(const std::string &ip);
    //void setPort(uint16_t port);
    //uint16_t getPort();
    //std::string ToString() const;
    //size_t HashCode() const;

    static bool GetInetAddressByHostname(const std::string hostname, std::string &ip, int &aiFamily, std::string &errorMsg);

private:
    struct sockaddr_in _addr;
    struct sockaddr_in6 _addr6;
    int _aiFamily;
    static const int MAX_HOST_IP_LENGTH = INET6_ADDRSTRLEN;
    //static const int MAX_HOST_PORT_LENGTH = 22;
    //static const int IPV4_ADDRESS_LENGTH = 16;

public:

#if defined(__ANDROID__) || defined(__linux__)
    static pthread_mutex_t  _mtxDns;
    static pthread_cond_t  _cvDns;
	static std::string _resolvedDns;
#endif

};

class SocketFuncs {
private:
    SocketFuncs();

public:
    static void Startup();
    static void Bind(SOCKET sockfd, const InetAddress &addr);
    static int connectTo(SOCKET sockfd, const InetAddress &addr);
    static void Shutdown(SOCKET sockfd);
    static SOCKET Accept(SOCKET sockfd);
    static void Listen(SOCKET sockfd, int connections);
    static bool SelectRead(SOCKET sockfd, int timeout);
    static bool SelectWrite(SOCKET sockfd, int timeout);
    static bool Select(SOCKET maxsock, fd_set *pWrite, fd_set *pRead, int timeout);
    //static unsigned int GetBindedPort(SOCKET sockfd);
    static void SetSocketOption(SOCKET sockfd, int level, int optName, int optVal);
    static void SetSocketOption(SOCKET sockfd, int level, int optName, const char *optVal, int optLen);
    static void GetSocketOption(SOCKET sockfd, int level, int optName, char *optVal, socklen_t *optLen);
    static void GetSocketOption(SOCKET sockfd, int level, int optName, socklen_t *optVal);

private:
};

class Socket {
protected:
    Socket(util::SmartHandle<SOCKET> sockfd, int);
    virtual ~Socket();

public:
    static int getLastErrorCode();
    static bool getEtryEagin(int errorCode);
    virtual void SetSocketOption(int level, int optName, socklen_t optVal);
    virtual void SetSocketOption(int level, int optName, const char *optVal, socklen_t optLen);
    virtual void GetSocketOption(int level, int optName, char *optVal, socklen_t *optLen);
    virtual void GetSocketOption(int level, int optName, socklen_t *optVal);
    virtual int Send(const unsigned char *buffer, int bufLen);
    virtual int Recv(unsigned char *buffer, int bufLen);
    virtual void Shutdown();
    //virtual void GetPeerAddress(InetAddress *address);

	int SetSocketRecvTimeOut(int timeOut);

protected:
    int CheckSocketReturn(int result);
	util::SmartHandle<SOCKET> _sockfd;

private:
    const int WAITING_TIME;
};

}
}
#endif
