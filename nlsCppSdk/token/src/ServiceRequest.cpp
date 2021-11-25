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

#if !defined _WIN32
#include <string.h>
#endif // _WIN32

#include "ServiceRequest.h"

namespace AlibabaNlsCommon {

ServiceRequest::ServiceRequest(
    const std::string &product, const std::string &version) :
  content_(NULL),
  contentSize_(0),
  params_(),
  product_(product),
  resourcePath_("/"),
  version_(version) {}

ServiceRequest::ServiceRequest(const ServiceRequest &other) : content_(NULL),
  contentSize_(other.contentSize_),
  params_(other.params_),
  product_(other.product_),
  resourcePath_(other.resourcePath_),
  version_(other.version_) {
    setContent(other.content_, other.contentSize_);
  }

ServiceRequest& ServiceRequest::operator=(const ServiceRequest &other) {
  if (this != &other) {
    content_ = NULL;
    contentSize_ = 0;
    params_ = other.params_;
    setContent(other.content_, other.contentSize_);
  }
  return *this;
}

ServiceRequest::~ServiceRequest() {
  if (content_)
    delete[] content_;
}

const char * ServiceRequest::content() const {
  return content_;
}

size_t ServiceRequest::contentSize() const {
  return contentSize_;
}

bool ServiceRequest::hasContent() const {
  return (contentSize_ != 0);
}

void ServiceRequest::setContent(const char * data, size_t size) {
  if (content_)
    delete content_;

  content_ = NULL;
  contentSize_ = 0;

  if (size) {
    contentSize_ = size;
    content_ = new char[size];
    memset(content_, 0x0, contentSize_);
    memcpy(content_, data, size);
    //std::copy(data, data + size, content_);
  }
}

void ServiceRequest::addParameter(const ParameterNameType & name,
                                  const ParameterValueType & value) {
  setParameter(name, value);
}

ServiceRequest::ParameterValueType ServiceRequest::parameter(
    const ParameterNameType &name)const {
  return params_.at(name);
}

ServiceRequest::ParameterCollection ServiceRequest::parameters() const {
  return params_;
}

void ServiceRequest::removeParameter(const ParameterNameType & name) {
  params_.erase(name);
}

void ServiceRequest::setParameter(const ParameterNameType &name,
                                  const ParameterValueType &value) {
  params_[name] = value;
}

void ServiceRequest::setParameters(const ParameterCollection & params) {
  params_ = params;
}

std::string ServiceRequest::version()const {
  return version_;
}

void ServiceRequest::setVersion(const std::string &version) {
  version_ = version;
}

std::string ServiceRequest::product() const {
	return product_;
}

std::string ServiceRequest::resourcePath() const {
	return resourcePath_;
}

void ServiceRequest::setResourcePath(const std::string & path) {
	resourcePath_ = path;
}

void ServiceRequest::setAction(const std::string &action) {
    action_ = action;
}

std::string ServiceRequest::action() const {
  return action_;
}

void ServiceRequest::setTask(const std::string &task) {
  task_ =  task;

  std::string tmp = "Task=";
  tmp += task;
  setContent(tmp.c_str(), tmp.size());
}

std::string ServiceRequest::task()const {
  return task_;
}

void ServiceRequest::setTaskId(const std::string &taskId) {
  taskId_ = taskId;
}

std::string ServiceRequest::taskId()const {
  return taskId_;
}

}
