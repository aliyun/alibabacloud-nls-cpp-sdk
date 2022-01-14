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

#ifndef NLS_SDK_THREAD_DATA_H_
#define NLS_SDK_THREAD_DATA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <utility>
#include <pthread.h>

namespace AlibabaNls {

class DataItf {
 protected:
  virtual ~DataItf() {}
  virtual void Clear() = 0;
  virtual size_t ArrayNum() = 0;
  virtual size_t ElementNum() = 0;
  virtual size_t ElementSize() = 0;
};

template <typename T>
class DataBase : public DataItf {
 public:
  DataBase() {
    pthread_mutex_init(&data_mutex_, NULL);
  }
  virtual ~DataBase() {
    Clear();
    pthread_mutex_destroy(&data_mutex_);
  }
  virtual int Pushback(const T *data, int element_num) {
    if ((NULL != data) && (element_num > 0)) {
      T *data_for_insert = new T[element_num];
      memcpy(data_for_insert, data, sizeof(T) * element_num);
      pthread_mutex_lock(&data_mutex_);
      data_.push_back(std::make_pair(data_for_insert, element_num));
      pthread_mutex_unlock(&data_mutex_);
      return 0;
    } else {
      return -1;
    }
  }
  virtual int Get(T *data, int element_num, int *start_array_idx,
                  int *start_element_idx, bool is_delete_after_get = false) {
    int num_shift = 0;
    if ((NULL != data) && (element_num > 0) && (*start_array_idx >= 0)) {
      pthread_mutex_lock(&data_mutex_);
      for (; (*start_array_idx < data_.size()) && (num_shift < element_num);) {
        if (*start_element_idx + element_num - num_shift >=
            data_[*start_array_idx].second) {
          // get data partly from cur array
          int cur_copy_num =
            data_[*start_array_idx].second - *start_element_idx;
          memcpy(data + num_shift,
                 data_[*start_array_idx].first + *start_element_idx,
                 sizeof(T) * cur_copy_num);
          num_shift += cur_copy_num;
          *start_element_idx = 0;
          if (is_delete_after_get) {
            delete [](data_[*start_array_idx].first);
            data_.erase(data_.begin() + *start_array_idx);
          } else {
            (*start_array_idx)++;
          }
          continue;
        } else {
          // get data entirely from cur array
          int cur_copy_num = element_num - num_shift;
          memcpy(data + num_shift,
                 data_[*start_array_idx].first + *start_element_idx,
                 sizeof(T) * cur_copy_num);
          num_shift += cur_copy_num;
          *start_element_idx += cur_copy_num;
          break;
        }
      }
      pthread_mutex_unlock(&data_mutex_);
    }
    return num_shift;
  }

  virtual int TryGet(T *data, int element_num, int *start_array_idx,
                  int *start_element_idx, bool is_delete_after_get = false) {
    int tmp_start_array_idx = *start_array_idx;
    int tmp_start_element_idx = *start_element_idx;
    if (element_num != Get(data, element_num, start_array_idx,
                           start_element_idx)) {
      *start_array_idx = tmp_start_array_idx;
      *start_element_idx = tmp_start_element_idx;
      if (is_delete_after_get) {
        Flush(start_array_idx);
      }
      return 0;
    } else {
      if (is_delete_after_get) {
        Flush(start_array_idx);
      }
      return element_num;
    }
  }

  virtual void Flush(int *start_array_idx) {
    pthread_mutex_lock(&data_mutex_);
    for (; *start_array_idx > 0 && *start_array_idx < data_.size();) {
      if ((*data_.begin()).first) {
       delete[](*data_.begin()).first;
      }
      data_.erase(data_.begin());
      (*start_array_idx)--;
    }
    pthread_mutex_unlock(&data_mutex_);
  }

  virtual void Clear() {
    pthread_mutex_lock(&data_mutex_);
    for (size_t i = 0; i < data_.size(); ++i) {
      if (data_[i].first) {
        delete [](data_[i].first);
      }
    }
    data_.clear();
    pthread_mutex_unlock(&data_mutex_);
  }
  virtual size_t ArrayNum() {
    size_t array_num = 0;
    pthread_mutex_lock(&data_mutex_);
    array_num = data_.size();
    pthread_mutex_unlock(&data_mutex_);
    return array_num;
  }
  virtual size_t ElementNum() {
    size_t element_num = 0;
    pthread_mutex_lock(&data_mutex_);
    for (size_t i = 0; i < data_.size(); ++i) {
      element_num += data_[i].second;
    }
    pthread_mutex_unlock(&data_mutex_);
    return element_num;
  }
  virtual size_t ElementSize() {
    return sizeof(T);
  }

 protected:
  std::vector< std::pair<T *, int> > data_;
  pthread_mutex_t data_mutex_;
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_THREAD_DATA_H_

