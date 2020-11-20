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

#ifndef ALIBABANLS_COMMON_TOKEN_H_
#define ALIBABANLS_COMMON_TOKEN_H_

#include "Global.h"
#include <string>

namespace AlibabaNlsCommon {

	class ALIBABANLS_COMMON_EXPORT NlsToken {

    public:

        virtual ~NlsToken();
        NlsToken();

         /**
            * @brief 申请获取tokenId.
            * @note 调用之前, 请先设置请求参数.
            * @return 成功则返回0; 失败返回-1.
            */
         int applyNlsToken();

		 /**
		 	* @brief 获取错误信息.
		 	* @return 成功则返回错误信息; 失败返回NULL.
		 	*/
		 const char* getErrorMsg();

		 /**
			 * @brief 获取tokenId.
			 * @return 成功则返回token字符串; 失败返回NULL.
			 */
		 const char* getToken();

		 /**
			 * @brief token有效期时间戳(秒).
			 * @return 成功则返回有效期时间戳, 失败返回0.
		 	*/
		 unsigned int getExpireTime();

        /**
            * @brief 设置阿里云账号的KeySecret
            * @param KeySecret	Secret字符串
            * @return void
            */
        void setKeySecret(const std::string & KeySecret);

        /**
            * @brief 设置阿里云账号的KeyId
            * @param KeyId	Id字符串
            * @return void
            */
        void setAccessKeyId(const std::string & accessKeyId);

        /**
            * @brief 设置域名
            * @param domain	域名字符串
            * @return void
            */
        void setDomain(const std::string & domain);

        /**
            * @brief 设置API版本
            * @param serverVersion	API版本字符串
            * @return void
            */
        void setServerVersion(const std::string & serverVersion);

        /**
            * @brief 设置服务路径
            * @param serverResourcePath	服务路径字符串
            * @return void
            */
        void setServerResourcePath(const std::string & serverResourcePath);

		/**
            * @brief 设置RegionId
            * @param regionId	服务地区
            * @return void
            */
		void setRegionId(const std::string & regionId);

		/**
            * @brief 设置功能
            * @param action	功能
            * @return void
            */
		void setAction(const std::string & action);

    private:
        std::string accessKeySecret_;
        std::string accessKeyId_;

        std::string domain_;
        std::string serverVersion_;
        std::string serverResourcePath_;

		std::string regionId_;
		std::string action_;

		std::string errorMsg_;
		std::string tokenId_;
		unsigned int expireTime_;

		int paramCheck();
    };

}

#endif //ALIBABANLS_COMMON_TOKEN_H_
