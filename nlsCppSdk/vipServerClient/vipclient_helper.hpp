#ifndef MIDDLEWARE_VIPCLIENT_CPP_HELPER_H_
#define MIDDLEWARE_VIPCLIENT_CPP_HELPER_H_
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "error_code.h"
#include "iphost.h"
#include "option.h"
#include "vipclient.h"
namespace middleware {
namespace vipclient {
namespace helper {

static std::string ToString(const IPHost& host) {
  std::ostringstream osstm;
  osstm << "ip:" << host.ip() << " "
        << "port:" << host.port() << " "
        << "weight:" << host.weight() << " "
        << "valid:" << host.valid() << " "
        << "unit:" << host.unit() << " "
        << "host_name:" << host.host_name() << " "
        << "app_use_type:" << host.app_use_type() << " "
        << "site:" << host.site() << " ";
  return osstm.str();
}

static std::string ToString(const IPHostArray& hosts) {
  std::ostringstream osstm;
  osstm << "iphosts size:" << hosts.size() << std::endl;
  for (unsigned int i = 0; i < hosts.size(); ++i) {
    osstm << "iphosts[" << i << "] " << ToString(hosts.get(i)) << std::endl;
  }
  return osstm.str();
}

#if 0 /* unused */
static pthread_mutex_t g_vipclient_init_mutex = PTHREAD_MUTEX_INITIALIZER;
/**
* @brief 全局执行一次的初始化函数，线程安全,
*        ！！！注意：内部会拉取线程，如果有FORK需要在FORK后执行
*
* Init()本身不支持线程安全，使用全局锁来达到线程安全的目的
* @param[in] jmenv_dom Defaults to "jmenv.tbsite.net". vipserver服务器集群域名
* @return bool  成功返回 true
                失败返回 false
*/
static bool GlobalInit(const char* jmenv_dom = "jmenv.tbsite.net") {
  bool result = false;
  pthread_mutex_lock(&g_vipclient_init_mutex);
  middleware::vipclient::VipClientApi::CreateApi();
  middleware::vipclient::Option option;
  // option.set_cache_path("./");
  // option.set_log_path("./");
  if (!middleware::vipclient::VipClientApi::Init(jmenv_dom, option)) {
    fprintf(stderr, "global init failed jmenv_dom:%s error:%s\n", jmenv_dom,
            strerror(errno));
    middleware::vipclient::VipClientApi::DestoryApi();
    result = false;
  } else {
    result = true;
  }
  pthread_mutex_unlock(&g_vipclient_init_mutex);
  return result;
}

/**
 * @brief 全局执行一次反初始化函数，线程安全
 *
 * UnInit()和DestoryApi()本身不支持线程安全
 * 这里使用了一个全局锁g_vipclient_init_mutex来达到线程安全的目的
 */
static void GlobalUnInit() {
  pthread_mutex_lock(&g_vipclient_init_mutex);
  middleware::vipclient::VipClientApi::UnInit();
  middleware::vipclient::VipClientApi::DestoryApi();
  pthread_mutex_unlock(&g_vipclient_init_mutex);
}

/**
* @brief 获取所有可用的IP
*
* 如果没有IP都不可用，进行降级处理，返回所有的IP
* @param[in] domain   请求域名
* @param[out] ip_list   存放IP的指针
* @return bool  true 成功
                false 失败
*/
static bool QueryAllValidIp(const char* domain,
                            std::vector<std::string>* ip_list) {
  middleware::vipclient::IPHostArray iphosts;
  if (!middleware::vipclient::VipClientApi::QueryAllIp(domain, &iphosts,
                                                       5 * 1000)) {
    fprintf(stderr, "query all ip failed %s %s\n", VipClientApi::Errstr(),
            strerror(errno));
    return false;
  }
  ip_list->clear();
  for (unsigned int i = 0; i < iphosts.size(); ++i) {
    const middleware::vipclient::IPHost& iphost = iphosts.get(i);
    if (iphost.valid()) {
      ip_list->push_back(iphost.ip());
    }
  }
  /*
  if (ip_list->empty()) { //一个合法的IP也没有，则进行降级保护，返回所有IP
      fprintf(stderr, "no valid ip downgrading %s", domain);
      for (unsigned int i =0; i < iphosts.size(); ++i) {
          const middleware::vipclient::IPHost& iphost = iphosts.get(i);
          ip_list->push_back(iphost.ip());
      }
  }*/
  return true;
}

/**
* @brief 获取可用的IP，线程安全
*
* 如果所有IP都不可用，那么会进行降级保护，返回任意一个IP
* @param[in] domain 请求的域名
* @param[out] ip 获取到的一个合法IP
* @return bool  true  成功
                false  失败
*/
static bool QueryValidIp(const char* domain, std::string* ip) {
  middleware::vipclient::IPHost iphost;
  if (middleware::vipclient::VipClientApi::QueryIp(domain, &iphost, 5 * 1000)) {
    *ip = iphost.ip();
    return true;
  }
  /*
  //获取合法的IP失败，进行降级处理，返回任意一个IP
  fprintf(stderr, "query ip failed %s %s downgrading\n", VipClientApi::Errstr(),
  strerror(errno)); std::vector<std::string> ip_list; if(QueryAllValidIp(domain,
  &ip_list)){ if (ip_list.size() > 0) { *ip = ip_list[rand()%ip_list.size()];
          return true;
      }
  }
  fprintf(stderr, "query ip failed %s %s\n", VipClientApi::Errstr(),
  strerror(errno));
  */
  return false;
}

/**
* @brief 跨过缓存去服务器实时获取可用IP，线程安全
*
* 这个函数会跨越缓存去服务器获取数据，不能频繁调用
* 并没有提供全部的实现，需要用户自己实现用于同步的信号量
* @param[in] domain 请求的域名
* @param[out] ip 获取到的IP信息
* @return bool  true  成功
                false  失败
*/
static bool RealQueryIp(const char* domain, std::string* ip) {
  class MySemaphore {
   public:
    void Post() {
      // TODO 用户自己实现等待信号量
      throw "user not implement semaphore";
    }
    void Wait() {
      // TODO 用户自己实现唤醒等待线程
      throw "user not implement semaphore";
    }
  };

  class QueryAdapter : public middleware::vipclient::QueryHandler {
   public:
    QueryAdapter(const char* domain, std::string* result)
        : domain_(domain), result_(result), ok_(false) {}
    virtual ~QueryAdapter() {}
    virtual void OnError(ErrorCode error_code) {
      fprintf(stderr, "OnError %d %s %s\n", error_code,
              middleware::vipclient::VipClientApi::StrErrno(error_code),
              strerror(errno));
      semaphore_.Post();
    }
    virtual void OnResult(const IPHostArray& iphosts) {
      if (iphosts.size() >= 1) {
        *result_ = iphosts.get(0).ip();
        ok_ = true;
      } else {
        fprintf(stderr, "OnResult %s no iphost\n", domain_.c_str());
        ok_ = false;
      }
      semaphore_.Post();
    }
    void Wait() { semaphore_.Wait(); }
    bool ok() const { return ok_; }

   private:
    std::string domain_;
    std::string* result_;
    bool ok_;
    MySemaphore semaphore_;
  } adapter(domain, ip);

  if (!middleware::vipclient::VipClientApi::AsyncQueryIp(domain, &adapter,
                                                         5 * 1000)) {
    fprintf(stderr, "async query failed %s %s %s\n", domain,
            middleware::vipclient::VipClientApi::Errstr(), strerror(errno));
  } else {
    adapter.Wait();
  }

  return adapter.ok();
}
#endif
}  // namespace helper
}  // namespace vipclient
}  // namespace middleware

#endif  // MIDDLEWARE_VIPCLIENT_CPP_HELPER_H_
