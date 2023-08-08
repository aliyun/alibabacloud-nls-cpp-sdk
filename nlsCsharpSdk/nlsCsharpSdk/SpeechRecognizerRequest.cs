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
    /// 一句话识别
    /// </summary>

    public class SpeechRecognizerRequest : ISpeechRecognizer
    {
        /// <summary>
        /// 一句话识别请求的Native指针.
        /// </summary>
        public IntPtr native_request;

        #region Start the request of speech recognizer
        /// <summary>
        /// 启动一句话识别. 异步操作, 成功返回started事件, 失败返回TaskFailed事件.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Start(SpeechRecognizerRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRstart(request.native_request);
        }
        #endregion

        #region Stop the request of speech recognizer
        /// <summary>
        /// 会与服务端确认关闭, 正常停止一句话识别操作. 异步操作, 失败返回TaskFailed.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Stop(SpeechRecognizerRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRstop(request.native_request);
        }
        #endregion

        #region Cancel the request of speech recognizer
        /// <summary>
        /// 直接关闭一句话识别过程.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Cancel(SpeechRecognizerRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRcancel(request.native_request);
        }
        #endregion

        #region Send audio data into the speech recognizer
        /// <summary>
        /// 发送语音数据.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
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
        /// 成功则返回>=0, 失败返回负值错误码.
        /// 由于音频格式不确定, 传入音频字节数和传出音频字节数, 无法通过比较判断成功与否, 故成功返回>=0.
        /// </returns>
        public int SendAudio(SpeechRecognizerRequest request, byte[] data, UInt64 dataSize, EncoderType type)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsendAudio(request.native_request, data, dataSize, (int)type);
        }
        #endregion

        #region Set parameters of SpeechRecognizerRequest
        /// <summary>
        /// 设置一句话识别服务URL地址.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 服务url字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetUrl(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetUrl(request.native_request, value);
        }

        /// <summary>
        /// 设置appKey.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// appKey字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetAppKey(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetAppKey(request.native_request, value);
        }

        /// <summary>
        /// 口令认证. 所有的请求都必须通过SetToken方法认证通过, 才可以使用.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 申请的token字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetToken(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetToken(request.native_request, value);
        }

        /// <summary>
        /// 设置音频数据编码格式字段Format.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 传送的音频数据格式, 目前支持pcm|opus|opu.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetFormat(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetFormat(request.native_request, value);
        }

        /// <summary>
        /// 设置音频数据采样率.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 目前支持16000, 8000. 默认是1600.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetSampleRate(SpeechRecognizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetSampleRate(request.native_request, value);
        }

        /// <summary>
        /// 设置定制模型.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 定制模型id字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetCustomizationId(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetCustomizationId(request.native_request, value);
        }

        /// <summary>
        /// 设置泛热词.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 定制泛热词id字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetVocabularyId(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetVocabularyId(request.native_request, value);
        }

        /// <summary>
        /// 设置是否返回中间识别结果.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetIntermediateResult(SpeechRecognizerRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetIntermediateResult(request.native_request, value);
        }

        /// <summary>
        /// 设置是否在后处理中添加标点.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetPunctuationPrediction(SpeechRecognizerRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetPunctuationPrediction(request.native_request, value);
        }

        /// <summary>
        /// 设置是否在后处理中执行数字转换.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetInverseTextNormalization(SpeechRecognizerRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetInverseTextNormalization(request.native_request, value);
        }

        /// <summary>
        /// 设置字段enable_voice_detection设置.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 是否启动自定义静音检测, 可选, 默认是False. 云端默认静音检测时间800ms.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetEnableVoiceDetection(SpeechRecognizerRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetEnableVoiceDetection(request.native_request, value);
        }

        /// <summary>
        /// 设置字段max_start_silence.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 允许的最大开始静音, 可选, 单位是毫秒. 超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
        /// 需要先设置SetEnableVoiceDetection为true. 建议时间2~5秒.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetMaxStartSilence(SpeechRecognizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetMaxStartSilence(request.native_request, value);
        }

        /// <summary>
        /// 设置字段max_end_silence.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 允许的最大结束静音, 可选, 单位是毫秒. 超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
        /// 需要先设置SetEnableVoiceDetection为true. 建议时间0~5秒.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetMaxEndSilence(SpeechRecognizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetMaxEndSilence(request.native_request, value);
        }

        /// <summary>
        /// 设置Socket接收超时时间.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 超时时间.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetTimeout(SpeechRecognizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetTimeout(request.native_request, value);
        }

        /// <summary>
        /// 设置输出文本的编码格式.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 编码格式 UTF-8 or GBK.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetOutputFormat(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetOutputFormat(request.native_request, value);
        }

        /// <summary>
        /// 获得设置的输出文本的编码格式.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回字符串, 否则返回unknown.</returns>
        public string GetOutputFormat(SpeechRecognizerRequest request)
        {
            IntPtr get = NativeMethods.SRgetOutputFormat(request.native_request);
            string format = "unknown";
            if (get != IntPtr.Zero)
            {
                format = Marshal.PtrToStringAnsi(get);
            }
            return format;
        }

        /// <summary>
        /// 参数设置.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 参数.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetPayloadParam(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetPayloadParam(request.native_request, value);
        }

        /// <summary>
        /// 设置用户自定义参数.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 参数.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetContextParam(SpeechRecognizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRsetContextParam(request.native_request, value);
        }

        /// <summary>
        /// 设置用户自定义ws阶段http header参数.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="key">
        /// 参数名称.
        /// </param>
        /// <param name="value">
        /// 参数内容.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int AppendHttpHeaderParam(SpeechRecognizerRequest request, string key, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SRappendHttpHeaderParam(request.native_request, key, value);
        }
        #endregion

        #region Set CallbackDelegate of SpeechRecognizer
        /// <summary>
        /// 从Native获取NlsEvent
        /// </summary>
        /// <returns></returns>
        private static int GetNlsEvent(out NLS_EVENT_STRUCT nls)
        {
            return NativeMethods.SRGetNlsEvent(out nls);
        }

        NlsCallbackDelegate onRecognitionStarted =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onRecognitionTaskFailed =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onRecognitionResultChanged =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onRecognitionCompleted =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onRecognitionClosed =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        #endregion

        #region Set Callback of SpeechRecognizer
        /// <summary>
        /// 设置一句话识别开始回调函数. 在语音识别可以开始时, sdk内部线程上报该回调.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnRecognitionStarted(
            SpeechRecognizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SROnRecognitionStarted(request.native_request, onRecognitionStarted, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置错误回调函数. 在请求过程中出现异常错误时, sdk内部线程上报该回调.
        /// 用户可以在事件的消息头中检查状态码和状态消息, 以确认失败的具体原因.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnTaskFailed(
            SpeechRecognizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SROnTaskFailed(request.native_request, onRecognitionTaskFailed, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置一句话识别中间结果回调函数. 设置SetEnableIntermediateResult字段为true, 才会有中间结果.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnRecognitionResultChanged(
            SpeechRecognizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SROnRecognitionResultChanged(request.native_request, onRecognitionResultChanged, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置一句话识别结束回调函数, 在语音识别完成时, sdk内部线程上报该回调.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnRecognitionCompleted(
            SpeechRecognizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SROnRecognitionCompleted(request.native_request, onRecognitionCompleted, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置通道关闭回调函数, 在请求过程中通道关闭时, sdk内部线程上报该回调.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnChannelClosed(
            SpeechRecognizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SROnChannelClosed(request.native_request, onRecognitionClosed, (IntPtr)toCppParam);
            return;
        }
        #endregion
    }
}
