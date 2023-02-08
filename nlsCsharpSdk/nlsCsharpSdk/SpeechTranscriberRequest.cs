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

using nlsCsharpSdk.CPlusPlus;
using System.Runtime.InteropServices;

namespace nlsCsharpSdk
{
    /// <summary>
    /// 实时转写
    /// </summary>
    /// 
    public class SpeechTranscriberRequest : ISpeechTranscriber
    {
        /// <summary>
        /// 实时识别请求的Native指针.
        /// </summary>
        public IntPtr native_request;

        #region Start the request of speech transcriber
        /// <summary>
        /// 启动实时音频流识别. 异步操作, 成功返回started事件, 失败返回TaskFailed事件.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Start(SpeechTranscriberRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STstart(request.native_request);
        }
        #endregion

        #region Stop the request of speech transcriber
        /// <summary>
        /// 会与服务端确认关闭, 正常停止实时音频流识别操作. 异步操作, 失败返回TaskFailed.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Stop(SpeechTranscriberRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STstop(request.native_request);
        }
        #endregion

        #region Cancel the request of speech transcriber
        /// <summary>
        /// 直接关闭实时音频流识别过程.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Cancel(SpeechTranscriberRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STcancel(request.native_request);
        }
        #endregion

        #region Send Control message into the speech transcriber
        /// <summary>
        /// 要求服务端更新识别参数. 异步操作, 失败返回TaskFailed.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="message">
        ///.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Control(SpeechTranscriberRequest request, string message)
        {
            return -1;
        }
        #endregion

        #region Send audio data into the speech transcriber
        /// <summary>
        /// 发送语音数据.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="data">
        /// 语音数据.
        /// </param>
        /// <param name="dataSize">
        /// 语音数据长度(建议每次100ms左右数据).
        /// </param>
        /// <param name="type">
        /// ENCODER_NONE 表示原始音频进行传递, 建议每次100ms音频数据,支持16K和8K;
        /// ENCODER_OPU 表示以定制OPUS压缩后进行传递, 只支持20ms 16K16b1c;
        /// ENCODER_OPUS 表示以OPUS压缩后进行传递, 只支持20ms, 支持16K16b1c和8K16b1c.
        /// </param>
        /// <returns>
        /// 成功则返回>=0, 失败返回负值错误码
        /// 由于音频格式不确定, 传入音频字节数和传出音频字节数, 无法通过比较判断成功与否, 故成功返回>=0.
        /// </returns>
        public int SendAudio(SpeechTranscriberRequest request, byte[] data, UInt64 dataSize, EncoderType type)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsendAudio(request.native_request, data, dataSize, (int)type);
        }
        #endregion

        #region Set parameters of SpeechTranscriberRequest
        /// <summary>
        /// 设置实时音频流识别服务URL地址.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 服务url字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetUrl(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetUrl(request.native_request, value);
        }

        /// <summary>
        /// 设置appKey.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// appKey字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetAppKey(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetAppKey(request.native_request, value);
        }

        /// <summary>
        /// 口令认证. 所有的请求都必须通过SetToken方法认证通过, 才可以使用.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 申请的token字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetToken(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetToken(request.native_request, value);
        }

        /// <summary>
        /// 设置音频数据编码格式字段Format.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 传送的音频数据格式, 目前支持pcm|opus|opu.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetFormat(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetFormat(request.native_request, value);
        }

        /// <summary>
        /// 设置音频数据采样率.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 目前支持16000, 8000. 默认是1600.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetSampleRate(SpeechTranscriberRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetSampleRate(request.native_request, value);
        }

        /// <summary>
        /// 设置是否返回中间识别结果.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetIntermediateResult(SpeechTranscriberRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetIntermediateResult(request.native_request, value);
        }

        /// <summary>
        /// 设置是否在后处理中添加标点.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetPunctuationPrediction(SpeechTranscriberRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetPunctuationPrediction(request.native_request, value);
        }

        /// <summary>
        /// 设置是否在后处理中执行数字转换.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetInverseTextNormalization(SpeechTranscriberRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetInverseTextNormalization(request.native_request, value);
        }

        /// <summary>
        /// 设置是否使用语义断句.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetSemanticSentenceDetection(SpeechTranscriberRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetSemanticSentenceDetection(request.native_request, value);
        }

        /// <summary>
        /// 设置vad阀值. 可选参数, 静音时长超过该阈值会被认为断句.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// vad阀值. 合法参数范围200～2000(ms), 默认值800ms. 
        /// vad断句与语义断句为互斥关系, 不能同时使用. 调用此设置前, 请将语义断句setSemanticSentenceDetection设置为false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetMaxSentenceSilence(SpeechTranscriberRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetMaxSentenceSilence(request.native_request, value);
        }

        /// <summary>
        /// 设置定制模型.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 定制模型id字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetCustomizationId(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetCustomizationId(request.native_request, value);
        }

        /// <summary>
        /// 设置泛热词.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 定制泛热词id字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetVocabularyId(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetVocabularyId(request.native_request, value);
        }

        /// <summary>
        /// 设置Socket接收超时时间.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 超时时间.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetTimeout(SpeechTranscriberRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetTimeout(request.native_request, value);
        }

        /// <summary>
        /// 设置是否开启nlp服务.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetEnableNlp(SpeechTranscriberRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetEnableNlp(request.native_request, value);
        }

        /// <summary>
        /// 设置nlp模型名称，开启NLP服务后必填.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// value nlp模型名称字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetNlpModel(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetNlpModel(request.native_request, value);
        }

        /// <summary>
        /// 设置session id.用于请求异常断开重连时，服务端识别是同一个会话
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// value session id 字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetSessionId(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetSessionId(request.native_request, value);
        }

        /// <summary>
        /// 设置输出文本的编码格式
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// value 编码格式 UTF-8 or GBK.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetOutputFormat(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetOutputFormat(request.native_request, value);
        }

        /// <summary>
        /// 获得设置的输出文本的编码格式.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回字符串, 否则返回unknown.</returns>
        public string GetOutputFormat(SpeechTranscriberRequest request)
        {
            IntPtr get = NativeMethods.STgetOutputFormat(request.native_request);
            string format = "unknown";
            if (get != IntPtr.Zero)
            {
                format = Marshal.PtrToStringAnsi(get);
            }
            return format;
        }

        /// <summary>
        /// 参数设置
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// value json格式字符串, 类似"{\"test1\":\"01\", \"test2\":\"15\"}".
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetPayloadParam(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetPayloadParam(request.native_request, value);
        }

        /// <summary>
        /// 设置用户自定义参数
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// value json格式字符串, 类似"{\"network\":{\"ip\":\"100.101.102.103\"}}".
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetContextParam(SpeechTranscriberRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STsetContextParam(request.native_request, value);
        }

        /// <summary>
        /// 设置用户自定义ws阶段http header参数
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="key">
        ///
        /// </param>
        /// <param name="value">
        ///
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int AppendHttpHeaderParam(SpeechTranscriberRequest request, string key, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.STappendHttpHeaderParam(request.native_request, key, value);
        }
        #endregion


        #region Set CallbackDelegate of SpeechTranscriber
        /// <summary>
        /// 从Native获取NlsEvent
        /// </summary>
        /// <returns></returns>
        private static int GetNlsEvent(out NLS_EVENT_STRUCT nls)
        {
            return NativeMethods.STGetNlsEvent(out nls);
        }

        NlsCallbackDelegate onTranscriptionStarted =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onTranscriptionTaskFailed =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onTranscriptionResultChanged =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onTranscriptionCompleted =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onTranscriptionClosed =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onSentenceBegin =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct)); // thrown
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onSentenceEnd =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onSentenceSemantics =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        #endregion

        #region Set Callback of SpeechTranscriber
        /// <summary>
        /// 设置实时识别开始回调函数. 在语音识别可以开始时, sdk内部线程上报该回调.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnTranscriptionStarted(
            SpeechTranscriberRequest request, CallbackDelegate callback, string para = null)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return;
            }
            SpeechParamStruct user_param = new SpeechParamStruct();
            user_param.user = para;
            user_param.callback = callback;
            user_param.nlsEvent = new NLS_EVENT_STRUCT();
            IntPtr toCppParam = Marshal.AllocHGlobal(Marshal.SizeOf(user_param));
            Marshal.StructureToPtr(user_param, toCppParam, false);
            NativeMethods.STOnTranscriptionStarted(request.native_request, onTranscriptionStarted, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置错误回调函数. 在请求过程中出现异常错误时, sdk内部线程上报该回调.
        /// 用户可以在事件的消息头中检查状态码和状态消息, 以确认失败的具体原因.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>

        public void SetOnTaskFailed(
            SpeechTranscriberRequest request, CallbackDelegate callback, string para = null)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return;
            }
            SpeechParamStruct user_param = new SpeechParamStruct();
            user_param.user = para;
            user_param.callback = callback;
            user_param.nlsEvent = new NLS_EVENT_STRUCT();
            IntPtr toCppParam = Marshal.AllocHGlobal(Marshal.SizeOf(user_param));
            Marshal.StructureToPtr(user_param, toCppParam, false);
            NativeMethods.STOnTaskFailed(request.native_request, onTranscriptionTaskFailed, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置实时识别中间结果回调函数. 设置SetEnableIntermediateResult字段为true, 才会有中间结果.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnTranscriptionResultChanged(
            SpeechTranscriberRequest request, CallbackDelegate callback, string para = null)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return;
            }
            SpeechParamStruct user_param = new SpeechParamStruct();
            user_param.user = para;
            user_param.callback = callback;
            user_param.nlsEvent = new NLS_EVENT_STRUCT();
            IntPtr toCppParam = Marshal.AllocHGlobal(Marshal.SizeOf(user_param));
            Marshal.StructureToPtr(user_param, toCppParam, false);
            NativeMethods.STOnTranscriptionResultChanged(request.native_request, onTranscriptionResultChanged, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置实时音频流识别结束回调函数, 在语音识别完成时, sdk内部线程上报该回调.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnTranscriptionCompleted(
            SpeechTranscriberRequest request, CallbackDelegate callback, string para = null)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return;
            }
            SpeechParamStruct user_param = new SpeechParamStruct();
            user_param.user = para;
            user_param.callback = callback;
            user_param.nlsEvent = new NLS_EVENT_STRUCT();
            IntPtr toCppParam = Marshal.AllocHGlobal(Marshal.SizeOf(user_param));
            Marshal.StructureToPtr(user_param, toCppParam, false);
            NativeMethods.STOnTranscriptionCompleted(request.native_request, onTranscriptionCompleted, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置通道关闭回调函数, 在请求过程中通道关闭时, sdk内部线程上报该回调.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnChannelClosed(
            SpeechTranscriberRequest request, CallbackDelegate callback, string para = null)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return;
            }
            SpeechParamStruct user_param = new SpeechParamStruct();
            user_param.user = para;
            user_param.callback = callback;
            user_param.nlsEvent = new NLS_EVENT_STRUCT();
            IntPtr toCppParam = Marshal.AllocHGlobal(Marshal.SizeOf(user_param));
            Marshal.StructureToPtr(user_param, toCppParam, false);
            NativeMethods.STOnChannelClosed(request.native_request, onTranscriptionClosed, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置一句话开始回调, 检测到一句话的开始时, sdk内部线程上报该回调.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnSentenceBegin(
            SpeechTranscriberRequest request, CallbackDelegate callback, string para = null)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return;
            }
            SpeechParamStruct user_param = new SpeechParamStruct();
            user_param.user = para;
            user_param.callback = callback;
            user_param.nlsEvent = new NLS_EVENT_STRUCT();
            IntPtr toCppParam = Marshal.AllocHGlobal(Marshal.SizeOf(user_param));
            Marshal.StructureToPtr(user_param, toCppParam, false);
            NativeMethods.STOnSentenceBegin(request.native_request, onSentenceBegin, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置一句话结束回调函数, 检测到了一句话的结束时, sdk内部线程上报该回调.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnSentenceEnd(
            SpeechTranscriberRequest request, CallbackDelegate callback, string para = null)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return;
            }
            SpeechParamStruct user_param = new SpeechParamStruct();
            user_param.user = para;
            user_param.callback = callback;
            user_param.nlsEvent = new NLS_EVENT_STRUCT();
            IntPtr toCppParam = Marshal.AllocHGlobal(Marshal.SizeOf(user_param));
            Marshal.StructureToPtr(user_param, toCppParam, false);
            NativeMethods.STOnSentenceEnd(request.native_request, onSentenceEnd, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置二次处理结果回调函数, 表示对实时转写的原始结果进行处理后的结果, 开启enable_nlp后返回.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnSentenceSemantics(
            SpeechTranscriberRequest request, CallbackDelegate callback, string para = null)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return;
            }
            SpeechParamStruct user_param = new SpeechParamStruct();
            user_param.user = para;
            user_param.callback = callback;
            user_param.nlsEvent = new NLS_EVENT_STRUCT();
            IntPtr toCppParam = Marshal.AllocHGlobal(Marshal.SizeOf(user_param));
            Marshal.StructureToPtr(user_param, toCppParam, false);
            NativeMethods.STOnSentenceSemantics(request.native_request, onSentenceSemantics, (IntPtr)toCppParam);
            return;
        }
        #endregion
    }
}
