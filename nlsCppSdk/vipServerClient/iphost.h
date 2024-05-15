#ifndef MIDDLEWARE_VIPCLIENT_IPHOST_INFO_H_
#define MIDDLEWARE_VIPCLIENT_IPHOST_INFO_H_
namespace middleware {
namespace vipclient {

class IPHostImpl;
class IPHost {
 public:
  IPHost();
  IPHost(const IPHostImpl& impl);
  ~IPHost();
  IPHost(const IPHost& other);
  IPHost& operator=(const IPHost& other);

 public:
  int port() const;                  //端口
  int weight() const;                //权重
  bool valid() const;                //是否有效
  const char* ip() const;            // ip地址
  const char* unit() const;          //单元名字
  const char* host_name() const;     // host
  const char* app_use_type() const;  //应用类型
  const char* site() const;          //机房信息
  const char* cluster() const;       //集群
  double original_weight() const;    //原始权重带浮点

  void set_port(int v);
  void set_weight(int v);
  void set_valid(bool v);
  void set_ip(const char* v);
  void set_unit(const char* v);
  void set_host_name(const char* v);
  void set_app_use_type(const char* v);
  void set_site(const char* v);
  void set_cluster(const char* v);
  void set_original_weight(double d);

 private:
  IPHostImpl* impl_;
};

class IPHostArrayImpl;
class IPHostArray {
 public:
  IPHostArray();
  IPHostArray(const IPHostArrayImpl& iphosts);
  ~IPHostArray();
  IPHostArray(const IPHostArray& other);
  IPHostArray& operator=(const IPHostArray& other);

 public:
  void add(const IPHost& iphost);
  void set(unsigned int index, const IPHost& iphost);
  const IPHost& get(unsigned int index) const;
  const IPHost* array_ptr() const;
  unsigned int size() const;
  void clear();

 private:
  IPHostArrayImpl* impl_;
};

}  // namespace vipclient
}  // namespace middleware
#endif  // MIDDLEWARE_VIPCLIENT_IPHOST_INFO_H_
