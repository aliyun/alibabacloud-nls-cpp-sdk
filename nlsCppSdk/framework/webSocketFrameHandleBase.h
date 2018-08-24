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

#ifndef NLS_SDK_WEBSOCKET_FRAME_HANDLE_BASE_H
#define NLS_SDK_WEBSOCKET_FRAME_HANDLE_BASE_H

template<typename T>
class HandleBaseOneParamWithReturnVoid {
public:
	HandleBaseOneParamWithReturnVoid();
	virtual ~HandleBaseOneParamWithReturnVoid();
	virtual void handlerFrame(T) = 0;
};

template<typename T>
HandleBaseOneParamWithReturnVoid<T>::HandleBaseOneParamWithReturnVoid() {

}

template<typename T>
HandleBaseOneParamWithReturnVoid<T>::~HandleBaseOneParamWithReturnVoid() {

}

#endif
