#ifndef MIDDLEWARE_VIPCLIENT_FOR_CPP_H_
#define MIDDLEWARE_VIPCLIENT_FOR_CPP_H_
#include "error_code.h"
namespace middleware {
namespace vipclient {

class IPHost;
class IPHostArray;

class QueryHandler {
 public:
  virtual ~QueryHandler() {}

  /**
   * @brief 错误回调
   *
   * 异步请求发生错误回调此接口
   * @param[in] error_code 错误码详情见error_code.h的错误码说明
   */
  virtual void OnError(ErrorCode error_code) = 0;

  /**
   * @brief 获取结果 回调
   *
   * 异步请求成功回调此接口
   * @param[in] iphosts
   * 获取到的IPHost信息，如果是获取一个那么数组长度为1，数组长度一定>=1
   */
  virtual void OnResult(const IPHostArray& iphosts) = 0;
};

/*
下表列出了所有用于控制程序参数的环境变量及其意义和默认值，当相应环境变量不为空时，Option的对应值会被忽略
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| 环境变量名                            | 意义                          | 默认值
| 备注                                            |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_C_MAX_BLOCK_TIME_MS    | C客户端所有同步接口的超时时间    | 5000
| 单位毫秒                                        |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_ALOG_CONFIGURE_FILE    | 日志配置文件                   | "" |
程序无需配置文件                                 |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_LOG_LEVEL              | 日志级别                       | 4 |
5-DEBUG,4-INFO,3-WARN,2-ERROR,1-FATAL,0-DISABLE |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_LOG_PATH               | 日志路径                       | "." |
确保写权限，会创建vipclient-logs                 |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_PROCESS_SHARE          | 共享内存开关                   | 0 |
1-打开，0-关闭                                   |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_PROCESS_SHARE_PATH     | 共享内存的控制路径              |
"/tmp/.vipclient-process-share/" | 保证目录有写权限 |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_MAX_CONCURRENT_REQUEST | 客户端最大并发请求数            | 1000
| 最小为10，最大为3000                             |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_BAD_QUERY_SPAN_SECONDS | 同一个错误域名的请求间隔时间    | 10 |
单位秒，最小值5秒，最大值5*60秒                    |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_CACHE_PATH             | 缓存路径                      | "." |
确保写权限，会创建vipclient-cache                 |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_LOCAL_CACHE_FIRST      | 设置是否本地缓存优先获取        | 0 |
0： 内存-->服务器---->本地缓存                    | | | | | 1：
内存-->本地缓存-->服务器                      |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_ENABLE_AUTH                | 开启或关闭鉴权和多租户功能      | 0 |
0： 关闭                                         | | | | | 1： 开启 |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
"VIPCLIENT_SET_FAILOVER_PATH"  设置容灾目录，默认为程序当前工作目录 "."
"VIPCLIENT_SET_FAILOVER_BACKUP_HOUR" 设置容灾备份时间小时点，默认为凌晨3点 3
*/
class Option;
class VipClientApi {
 public:
  /**
   * @brief 创建Api，不支持多线程调用
   *
   * 创建客户端的全局执行体
   * 在调用VipClientApi的所有接口前必须先执行此函数
   * 创建后应调用Init进行初始化，一般在程序启动时执行
   */
  static void CreateApi();

  /**
   * @brief 销毁Api，不支持多线程调用
   *
   * 销毁客户端的全局执行体
   * 应先调用UnInit进行反初始化后再调用此函数
   * 必须保证无任何其他VipClientApi调用后调用，一般在程序退出时执行
   */
  static void DestoryApi();

  /**
   * @brief 获取错误码，支持多线程调用
   *
   * 接口调用失败时，可获取当前线程执行阶段发生的错误
   * @return int 错误码，详情见ErrorCode定义
   */
  static int Errno();

  /**
   * @brief 获取错误字符串，支持多线程调用
   *
   * 接口调用失败时，可获取当前线程执行阶段发生的错误解释字符串
   * @return const char*    错误详细解释
   */
  static const char* Errstr();

  /**
   * @brief 获取错误详细信息，支持多线程调用
   *
   * 根据错误码获取字符串解释
   * @param[out] error_num 错误码，由Errno返回
   * @return const char*  错误详细解释
   */
  static const char* StrErrno(int error_num);

  /**
   * @brief 初始化客户端，不支持多线程调用
   *        ！！！注意：内部会拉取线程，如果有FORK需要在FORK后执行
   * 初始化客户端，里面会拉取线程执行缓存更新，拉取Callback执行体
   * 在应用开始工作前执行
   * @param[in] jmenv_dom  vipserver顶级域名
   * @param[in] option 选项，详细说明见Option的说明
   * @return bool  true  成功
   *               false 失败，可通过Errno获取错误码
   */
  static bool Init(const char* jmenv_dom, const Option& option);

  /**
   * @brief 反初始化客户端，不支持多线程调用
   *
   * 停止所有线程执行体，并执行清理工作
   * 必须保证无任何其他Api调用后调用再调用，一般在应用停止工作后执行
   * 调用次接口后可调用DestoryApi销毁Api，也可以再次Init后继续使用Api
   */
  static void UnInit();

  /**
   * @brief 同步获取域名下的一个IPHost，支持多线程调用
   *
   * 使用内部定义的负载算法获取一个IPHost，获取顺序如下
   * Option.set_local_cache_first(false)(默认)的获取顺序为：内存-->服务器-->本地缓存
   * Option.set_local_cache_first(true)的获取顺序为：内存-->本地缓存-->服务器
   * @param[in] domain    域名
   * @param[out] iphost    存放IPHost的指针
   * @param[in] timeout_ms    超时时间毫秒数，<0为阻塞，=0同调用Try系列接口
   * @return bool   true  成功
   *                false 失败，可通过Errno获取错误码
   */
  static bool QueryIp(const char* domain, IPHost* iphost, long timeout_ms);

  /**
   * @brief 同步获取域名下的IP列表，支持多线程调用
   *
   * 获取域名下的所有IP，包括可用和不可用，Valid字段区分
   * 获取顺序同QueryIp()
   * @param[in] domain    请求域名
   * @param[out] iphosts     存放IP列表的指针
   * @param[in] timeout_ms   超时时间，<0为阻塞，=0同调用Try系列接口
   * @return bool  true  成功
   *               false 失败，可通过Errno获取错误码
   */
  static bool QueryAllIp(const char* domain, IPHostArray* iphosts,
                         long timeout_ms);

  static bool QueryAllValidIp(const char* domain, IPHostArray* iphosts,
                              long timeout_ms);

  static bool QueryUnitAllIp(const char* domain, const char* unit,
                             IPHostArray* iphosts, long timeout_ms);

  static bool QueryClusterIp(const char* domain, const char* cluster,
                             IPHost* iphost, long timeout_ms);

  static bool QueryClusterAllIp(const char* domain, const char* cluster,
                                IPHostArray* iphosts, long timeout_ms);

  /**
  * @brief 尝试获取指定域名下一个IP信息，支持多线程调用
  *
  * 获取一个IPHost列表，注意对IPHost的是否有效的valid的判断
  * 获取顺序：内存-->本地缓存
  * @param[in] domain    请求域名
  * @param[out] iphost    存放IPHost的指针
  * @return bool    true 成功
                    false 失败，可通过Errno获取错误码
  */
  static bool TryQueryIp(const char* domain, IPHost* iphost);

  static bool TryQueryAllIp(const char* domain, IPHostArray* iphosts);

  static bool TryQueryUnitAllIp(const char* domain, const char* unit,
                                IPHostArray* iphosts);

  static bool TryQueryClusterIp(const char* domain, const char* cluster,
                                IPHost* iphost);

  static bool TryQueryClusterAllIp(const char* domain, const char* cluster,
                                   IPHostArray* iphosts);

  /**
   * @brief 异步获取指定域名下一个IPHost信息，支持多线程调用
   *
   * 异步获取一个IPHost信息，结果通过QueryHandler回调
   * 如果接口返回失败，不会有任何回调
   * 如果回调未执行前调用UnInit，会弹出正在请求的回调
   * 此接口不会查询缓存，而是直接去vipserver集群请求，勿过度请求
   * @param[in] domain    域名
   * @param[in] handler    回调处理接口，详情见QueryHandler说明
   *                       重要：由用户保证其生命周期，回调前不允许删除
   *                             如果用户是new出来的，那么用户负责在回调后delete
   * @param[in] timeout_ms    超时时间
   * @return bool    true  成功
   *                 false  失败，可通过Errno获取错误码
   */
  static bool AsyncQueryIp(const char* domain, QueryHandler* handler,
                           long timeout_ms);

  static bool AsyncQueryAllIp(const char* domain, QueryHandler* handler,
                              long timeout_ms);

  static bool AsyncQueryClusterIp(const char* domain, const char* cluster,
                                  QueryHandler* handler, long timeout_ms);

  static bool AsyncQueryClusterAllIp(const char* domain, const char* cluster,
                                     QueryHandler* handler, long timeout_ms);

  /**
   * @brief 同步添加IP信息到域名，支持多线程调用
   *
   * 添加IP信息到域名，函数只关心每个IPHost的ip、port、weight三个值
   * @param[in] domain     待添加的域名
   * @param[in] iphosts
   * 需要添加的IPHost信息数组，注意ip、port、weight为必填值，其余值不关心
   * @param[in] token      域名秘钥，可从登陆OPS来查看
   * @param[in] timeout_ms 超时时间
   * @return bool    true  成功
   *                 false 失败，可通过Errno获取错误码
   */
  static bool AddIpToDomain(const char* domain, const IPHostArray& iphosts,
                            const char* token, long timeout_ms);
  static bool AddIpToDomain(const char* domain, const char* iphosts,
                            const char* token, long timeout_ms);

  /**
   * @brief 同步删除域名下的IP信息，支持多线程调用
   *
   * 删除某个域名下的IP信息，函数只关心每个IPHost的ip、port、weight三个值
   * @param[in] domain
   * @param[in] iphosts
   * 需要删除的IPHost信息数组，注意ip、port、weight为必填值，其余值不关心
   * @param[in] token    域名秘钥，可从登陆OPS来查看
   * @param[in] timeout_ms    超时时间
   * @return bool    true 成功
   *                 false 失败，可通过Errno获取错误码
   */
  static bool RemoveIpFromDomain(const char* domain, const IPHostArray& iphosts,
                                 const char* token, long timeout_ms);
  static bool RemoveIpFromDomain(const char* domain, const char* iphosts,
                                 const char* token, long timeout_ms);

  /**
   * @brief 根据userid获取Unit
   *
   * @param[in] id    userId
   * @param[out] out_buff    用于存放unit的缓存，推荐128个字节
   * @param[in] buff_len    存放结果的缓存的大小
   * @param[in] timeout_ms    超时时间
   * @return bool    true 成功
   *                 false 失败，可通过Errno获取错误码
   */
  static bool GetUnitByUserId(unsigned long long id, char* out_buff,
                              unsigned int buff_len, long timeout_ms);

  /**
   * @brief 根据IP获取Unit
   *
   * @param[in] ip    ip地址
   * @param[out] out_buff   用于存放unit的缓存，推荐128个字节
   * @param[in] buff_len   存放结果的缓存的大小
   * @param[in] timeout_ms    超时时间
   * @return bool    true 成功
   *                 false 失败，可通过Errno获取错误码
   */
  static bool GetUnitByIp(const char* ip, char* out_buff, unsigned int buff_len,
                          long timeout_ms);

  //获取本机机房信息
  static bool GetLocalSite(char* out_buff, unsigned int buff_len);

 private:
  VipClientApi();
  ~VipClientApi();
  VipClientApi(const VipClientApi&);
  const VipClientApi& operator=(const VipClientApi&);
};

}  // namespace vipclient
}  // namespace middleware

#endif  // MIDDLEWARE_VIPCLIENT_FOR_CPP_H_
