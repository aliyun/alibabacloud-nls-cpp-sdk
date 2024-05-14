#ifndef MIDDLEWARE_VIPCLIENT_OPTION_H_
#define MIDDLEWARE_VIPCLIENT_OPTION_H_
namespace middleware {
namespace vipclient {

class OptionImpl;
class Option {
 public:
  Option();
  ~Option();
  Option(const Option& other);
  Option& operator=(const Option& other);

 public:
  //日志占用空间大小（以字节为单位）
  //默认值：(256LL<<20)字节，即256MB
  unsigned long long max_log_size() const;
  void set_max_log_size(unsigned long long s);

  //日志路径，默认值：./
  //日志路径环境变量："VIPCLIENT_SET_LOG_PATH"
  //日志级别环境变量："VIPCLIENT_SET_LOG_LEVEL"
  const char* log_path() const;
  void set_log_path(const char* p);

  //缓存路径，默认值：./
  //缓存路径环境变量："VIPCLIENT_SET_CACHE_PATH"
  const char* cache_path() const;
  void set_cache_path(const char* p);

  //容灾路径，默认值：/home/admin/
  //容灾路径环境变量："VIPCLIENT_SET_FAILOVER_PATH"
  const char* failover_path() const;
  void set_failover_path(const char* s);

  //本机IP，默认值：空字符串，自动获取第一个非本机IP
  const char* local_ip() const;
  void set_local_ip(const char* n);

  //是否多进程共享数据，默认值：false
  bool process_share() const;
  void set_process_share(bool s);

  //控制多进程共享路径
  //默认值：/tmp/.vipclient-process-share/
  const char* share_path() const;
  void set_share_path(const char* path);

  //应用名，默认值：程序名
  const char* app_name() const;
  void set_app_name(const char* name);

  //错误请求的间隔时间，防止不断请求错误的域名，单位秒，默认值30秒
  //控制环境变量:VIPCLIENT_SET_BAD_QUERY_SPAN_SECONDS
  long bad_query_span() const;
  void set_bad_query_span(long seconds);

  //设置是本地缓存优先：如果是，那么每次先查询本地缓存，不存在再去服务器请求数据，默认值false
  //控制环境变量:VIPCLIENT_SET_LOCAL_CACHE_FIRST
  bool local_cache_first() const;
  void set_local_cache_first(bool b);

  //设置是否查询全量IP，包含所有机房的IP
  bool query_full_api() const;
  void set_query_full_api(bool b);

 private:
  OptionImpl* impl_;
};

}  // namespace vipclient
}  // namespace middleware

#endif  // MIDDLEWARE_VIPCLIENT_OPTION_H_
