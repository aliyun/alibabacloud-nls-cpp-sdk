/*
 * Copyright 2009-2017 Alibaba Cloud All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ALIBABANLS_COMMON_SERVICEREQUEST_H_
#define ALIBABANLS_COMMON_SERVICEREQUEST_H_

#include <map>
#include "Url.h"

namespace AlibabaNlsCommon {

class  ServiceRequest {
 public:
  typedef std::string ParameterNameType;
  typedef std::string ParameterValueType;
  typedef std::map<ParameterNameType, ParameterValueType> ParameterCollection;
  typedef std::map<ParameterNameType, ParameterValueType>::iterator ParameterCollectionIterator;

  virtual ~ServiceRequest();

  const char* content()const;
  size_t contentSize()const;
  bool hasContent()const;
  ParameterCollection parameters()const;
  std::string product()const;
  std::string resourcePath()const;
  std::string version()const;
  std::string action()const;
  std::string task()const;
  std::string taskId()const;

 protected:
  ServiceRequest(const std::string &product, const std::string &version);
  ServiceRequest(const ServiceRequest &other);
  ServiceRequest& operator=(const ServiceRequest &other);

  void addParameter(const ParameterNameType &name,
                    const ParameterValueType &value);
  ParameterValueType parameter(const ParameterNameType &name)const;
  void removeParameter(const ParameterNameType &name);
  void setContent(const char *data, size_t size);
  void setParameter(const ParameterNameType &name,
                    const ParameterValueType &value);
  void setParameters(const ParameterCollection &params);
  void setResourcePath(const std::string &path);
  void setVersion(const std::string &version);
  void setAction(const std::string &action);
  void setTask(const std::string &task);
  void setTaskId(const std::string &taskId);

 private:
  char *content_;
  size_t contentSize_;
  ParameterCollection params_;
  std::string product_;
  std::string resourcePath_;
  std::string version_;
  std::string action_;
  std::string task_;
  std::string taskId_;
};

}  // namespace AlibabaNlsCommon

#endif // !ALIBABANLS_COMMON_SERVICEREQUEST_H_
