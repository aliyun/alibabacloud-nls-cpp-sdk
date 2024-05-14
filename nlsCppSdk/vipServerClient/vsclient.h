#ifndef VSCLIENT_H_INCLUDED
#define VSCLIENT_H_INCLUDED

#include <inttypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
#define VIPSRV_MAX_STR_LEN      64
#define VIPSRV_MAX_IP_LEN       22  // strlen(255.255.255.255:65536) and one for '\0'
#define VIPSRV_MAX_MSG_LEN      256
#define VIPSRV_MAX_DOM_LEN      128
#define VIPSRV_MAX_CLUSTERS_LEN 256
#define VIPSRV_MAX_KEY_LEN      (VIPSRV_MAX_DOM_LEN + VIPSRV_MAX_CLUSTERS_LEN + 1)
typedef struct {
  char ip[VIPSRV_MAX_IP_LEN];
  int port;
  int weight;
  bool valid;
  char unit[VIPSRV_MAX_STR_LEN];
  char hostname[VIPSRV_MAX_STR_LEN];
  char appUseType[VIPSRV_MAX_STR_LEN];

  ////for tengine
  // int priority;
  // char site[VIPSRV_MAX_STR_LEN];
  // bool someSite;
} IPHost;

typedef struct {
  uint32_t is_success;
  char er_msg[VIPSRV_MAX_MSG_LEN];  // msg string when error occurs
  char** iplist;
  uint32_t ip_count;
  IPHost** iplist_info;  // compatible old ver
  ////for tengine
  // char self_site[VIPSRV_MAX_STR_LEN];
} VIPSrv_Result;

typedef struct {
  const char* cache_dir;
  const char* jmenv_dom;
} VIPSrv_Config;

/*
下表列出了所有用于控制程序参数的环境变量及其意义和默认值，当相应环境变量不为空时，Option的对应值会被忽略
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| 环境变量名                           | 意义                          | 默认值
| 备注                                            |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_C_MAX_BLOCK_TIME_MS    | C客户端所有同步接口的超时时间 | 5000 |
单位毫秒                                        |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_ALOG_CONFIGURE_FILE    | 日志配置文件                  | "" |
程序无需配置文件                                |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_LOG_LEVEL              | 日志级别                      | 4 |
5-DEBUG,4-INFO,3-WARN,2-ERROR,1-FATAL,0-DISABLE |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_LOG_PATH               | 日志路径                      | "." |
确保写权限，会创建vipclient-logs                |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_PROCESS_SHARE          | 共享内存开关                  | 0 |
1-打开，0-关闭                                  |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_PROCESS_SHARE_PATH     | 共享内存的控制路径            |
"/tmp/.vipclient-process-share/" | 保证目录有写权限 |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_MAX_CONCURRENT_REQUEST | 客户端最大并发请求数          | 1000 |
最小为10，最大为3000                            |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_BAD_QUERY_SPAN_SECONDS | 同一个错误域名的请求间隔时间  | 10 |
单位秒，最小值5秒，最大值5*60秒                 |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_CACHE_PATH             | 缓存路径                      | "." |
确保写权限，会创建vipclient-cache               |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
| VIPCLIENT_SET_LOCAL_CACHE_FIRST      | 设置是否本地缓存优先获取      | 0 | 0：
内存-->服务器---->本地缓存                  | | | | | 1：
内存-->本地缓存-->服务器                    |
+--------------------------------------+-------------------------------+----------------------------------+-------------------------------------------------+
"VIPCLIENT_SET_FAILOVER_PATH"  设置容灾目录，默认为程序当前工作目录 "."
"VIPCLIENT_SET_FAILOVER_BACKUP_HOUR" 设置容灾备份时间小时点，默认为凌晨3点 3
*/

// NOT threadsafe
/*
功能： 初始化客户端资源，包括定时器及router等的初始化和启动；
                  客户端使用前必需要调用该接口进行初始化；
参数：VIPSrv_Config：cache_dir：本地容灾文件根路径；
                                          如：cache_dir = getenv("HOME")
当前用户目录下 VIPSrv_Config：jmenv_dom：地址服务器域名；一般为jmenv.tbsite.net

*/
extern VIPSrv_Result* vipsrv_global_init(VIPSrv_Config*);

/*********
功能：反初始化工作，客户端使用完毕后需要调用该接口，
                        进行资源清理回收
*********/
extern void vipsrv_global_cleanup(void);

/*****
功能：每次接口调用返回的Result使用完毕后要用vipsrv_result_deref(VIPSrv_Result*)进行销毁
参数：VIPSrv_Result*
：接口调用的返回结果的指针；记录接口调用是否与成功，以及IP地址信息
******/
extern void vipsrv_result_deref(VIPSrv_Result*);

// ThreadSafe
/*****
功能：根据域名获取一个有效ip，此ip按照RR形式动态返回
参数：domain：vipserver中的域名，对应一组ip
返回值：VIPSrv_Result* 记录查询是否成功，以及查询到的IP列表的具体信息
包括异步和同步接口，同步接口兼容0.3.8及以前版本
异步接口为tengine定制
*****/
extern VIPSrv_Result* vipsrv_srv_ip(const char*);
extern VIPSrv_Result* async_vipsrv_srv_ip(const char*);

/******
功能：根据域名+集群限制获取一个有效ip，此ip按照RR形式动态返回
参数：domain：vipserver中的域名，对应一组ip
                 Clusters：指定的虚拟集群名
返回值：VIPSrv_Result* 记录查询是否成功，以及查询到的IP列表的具体信息
包括异步和同步接口，同步接口兼容0.3.8及以前版本
异步接口为tengine定制
******/
extern VIPSrv_Result* vipsrv_srv_clusters_ip(const char*, const char*);
extern VIPSrv_Result* async_vipsrv_srv_clusters_ip(const char*, const char*);

/*******
功能：按对称调用机制返回dom 域名对应所有IP列表，
                 包括"可用"与"不可用"的IP，标记区分，若不能查找到，则返回错误
参数：domain：vipserver中的域名，对应一组ip
返回值：VIPSrv_Result* 记录查询是否成功，以及查询到的IP列表的具体信息
*******/
extern VIPSrv_Result* vipsrv_srv_iplist(const char*);
extern VIPSrv_Result* async_vipsrv_srv_iplist(const char*);

/******
功能：按对称调用机制返回dom 域名+集群clusters对应的所有IP列表，
                 包括"可用"与"不可用"的IP，标记区分，若不能查找到，则返回错误
参数：domain：vipserver中的域名，对应一组ip
      Clusters：指定的虚拟集群名
返回值：VIPSrv_Result* 记录查询是否成功，以及查询到的IP列表的具体信息
包括异步和同步接口，同步接口兼容0.3.8及以前版本
异步接口为tengine定制

*******/
extern VIPSrv_Result* vipsrv_srv_clusters_iplist(const char*, const char*);
extern VIPSrv_Result* async_vipsrv_srv_clusters_iplist(const char*,
                                                       const char*);

/******
功能：向指定域名注册（批量）ip地址
参数：domain：指定的域名
          ip_list：需要添加的IP列表；
格式： ip[:port]_weight[_cluster]，例如：8.8.8.8:8080_3,128.0.0.1:80_1
          token： 域名秘钥，可从登陆OPS来查看
返回值：VIPSrv_Result* 记录查询是否成功，以及查询到的IP列表的具体信息
*******/
extern VIPSrv_Result* vipsrv_add_ip4dom(const char*, const char*, const char*);

/******
功能: 向某个域名删除IP地址
参数: domain: 要操作的域名名称
    ip_list: 需要删除的ip列表
格式:  ip[:port]_weight，例如：8.8.8.8:8080_3,128.0.0.1:80_1
    token:  域名秘钥，可从登陆OPS来查看
返回值: VIPSrv_Result* 记录删除是否成功
*******/
extern VIPSrv_Result* vipsrv_remv_ip4dom(const char*, const char*, const char*);

/******
功能：返回指定userId所属单元的所有IP列表，此IP列表是 dom
域名对应IP列表的一个子集返，
                 包括"可用"与"不可用"的IP，标记区分，若不能查找到，则返回错误
参数：domain：vipserver中的域名，对应一组ip；
                         userId：要查询的用户ID；
返回值：VIPSrv_Result* 记录查询是否成功，以及查询到的IP列表的具体信息
包括异步和同步接口，同步接口兼容0.3.8及以前版本
异步接口为tengine定制

*******/
extern VIPSrv_Result* vipsrv_srv_unit_hosts(const char* domain,
                                            uint64_t userId);
extern VIPSrv_Result* async_vipsrv_srv_unit_hosts(const char* domain,
                                                  uint64_t userId);
/*******
功能:封装router接口暴露给外部
说明:这两个接口必须在vipsrv_global_init初始化化之后使用
********/
extern int getUnitByUserId(uint64_t userId, char* pUnitName);
extern int getUnitByip(char* sIp, char* pUnitName);

///*******
//功能:设置获取所有机房所有单元的IP信息
//说明:Tengine专用
//********/
// extern int setQueryFullIp(int b);

#ifdef __cplusplus
}
#endif

#endif
