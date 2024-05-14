#ifndef MIDDLEWARE_VIPCLIENT_ERROR_CODE_H_
#define MIDDLEWARE_VIPCLIENT_ERROR_CODE_H_
namespace middleware {
namespace vipclient {

enum ErrorCode {
  OK = 0,

  INTERNAL_ERROR = -10001,  //内部错误，或者在关闭的过程中调用异步接口（不允许）

  PARAM_ERROR = -20001,  //参数错误，指针为空，字符串为空，缓存长度太小等
  NOT_CREATE_API = -20002,  //未调用VipClientApi::CreateApi()
  NOT_INIT = -20003,        //未调用VipClientApi::Init(...)
  TRY_AGAIN_500MS =
      -20004,  //请求已发出，需过段时间获取结果，TryXXX系列函数会返回此值
  TOO_MANY_REQUEST = -20005,  //过度请求，同时请求的域名数过多导致
  ACCESS_CACHE_FAILED =
      -20006,  //写缓存失败，初始化时如果缓存目录无权限返回此值

  INVALID_VIPCLUSTER_DOMAIN = -30001,  //不合法的vipserver域名，无法解析的域名
  NO_VIPSERVER_AVALIABAL = -30002,  //无vipserver机器可用
  CONNECT_TO_VIPCLUSTER_FAIL =
      -30003,  //连接vipserver集群失败，或者异步接口未回调前调用关闭接口
  REQUEST_TO_VIPCLUSTER_TIME_OUT = -30004,  //发送给vipserver的请求超时
  VIPCLUSTER_BAD_SERVICE = -30005,  // vipserver集群服务错误，返回值无法解析

  DOMAIN_NOT_FIND = -40001,    //不存在的域名
  DOMAIN_HAS_NO_IP = -40002,   //域名下无IP
  CLUSTER_NOT_FOUND = -40003,  //没有此集群名
  INVALID_TOKEN = -40004       // token非法
};

}
}  // namespace middleware

#endif  // MIDDLEWARE_VIPCLIENT_ERROR_CODE_H_
