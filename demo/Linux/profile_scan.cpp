/*
 * Copyright 2021 Alibaba Group Holding Limited
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


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <iostream>
#include <unistd.h>
#include "profile_scan.h"

void get_profile_info (const char *exec, PROFILE_INFO *info) {
  FILE *fd = NULL;
  char buff[1024];
  char cmd_string[1024];
  char exec_tmp_file[64];
  PROFILE_INFO *cur_info = info;

  snprintf(exec_tmp_file, 64, "%s.tmp", exec);
  snprintf(cmd_string, 1024, "ps -aux | grep %s > %s", exec, exec_tmp_file);
  int ret = system(cmd_string);
  if (ret >= 0) {
    fd = fopen(exec_tmp_file, "r");
    if (fd) {
      while (fgets(buff, sizeof(buff), fd) != NULL) {

        if (strstr(buff, "--appkey") != NULL) {
          //      std::cout << "get return: " << buff << std::endl;
          sscanf(buff, "%s %u %f %f",
              cur_info->usr_name,
              &cur_info->pid,
              &cur_info->ave_cpu_percent,
              &cur_info->ave_mem_percent
              );
          //      std::cout << "get cpu:" << cur_info->ave_cpu_percent << "%" << std::endl;
          break;
        }
        memset(buff, 0, sizeof(buff));

      } // while

      fclose(fd);  
    }
  }
  return;
}
