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

#include "profile_scan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

void get_profile_info(const char *exec, PROFILE_INFO *info) {
  char cmd_string[1024];
  PROFILE_INFO *cur_info = info;
  snprintf(cmd_string, 1024, "ps -aux | grep %s", exec);
  // std::cout << " autoClose try to popen (" << cmd_string << ")" << std::endl;

  FILE *fp = popen(cmd_string, "r");
  if (fp != NULL) {
    char buff[2048];
    while (fgets(buff, sizeof(buff), fp) != NULL) {
      if (strstr(buff, "--appkey") != NULL) {
        // std::cout << "get return: " << buff << std::endl;
        sscanf(buff, "%31s %u %f %f", cur_info->usr_name, &cur_info->pid,
               &cur_info->ave_cpu_percent, &cur_info->ave_mem_percent);
        std::cout << "get cpu for autoClose:" << cur_info->ave_cpu_percent
                  << "%" << std::endl;
        break;
      }
      memset(buff, 0, sizeof(buff));
    }  // while

    pclose(fp);
  }
  return;
}
