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
using System.Text;
using System.Runtime.InteropServices;


namespace nlsCsharpSdk
{
    /// <summary>
    /// 语音合成
    /// </summary>

    public class SpeechSynthesizerRequest : ISpeechSynthesizer
    {
        /// <summary>
        /// 语音合成请求的Native指针.
        /// </summary>
        public IntPtr native_request;

        #region Start the request of speech synthesizer
        /// <summary>
        /// 启动语音合成. 异步操作, 成功返回started事件, 失败返回TaskFailed事件.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Start(SpeechSynthesizerRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYstart(request.native_request);
        }
        #endregion

        #region Stop the request of speech synthesizer
        /// <summary>
        /// 会与服务端确认关闭, 正常停止语音合成操作. 异步操作, 失败返回TaskFailed.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Stop(SpeechSynthesizerRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYstop(request.native_request);
        }
        #endregion

        #region Cancel the request of speech synthesizer
        /// <summary>
        /// 直接关闭语音合成过程. 调用cancel之后不会在上报任何回调事件.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int Cancel(SpeechSynthesizerRequest request)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYcancel(request.native_request);
        }
        #endregion

        #region Set parameters of SpeechSynthesizerRequest
        /// <summary>
        /// 设置语音合成服务URL地址
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 服务url字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetUrl(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetUrl(request.native_request, value);
        }

        /// <summary>
        /// 设置appKey.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// appKey字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetAppKey(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetAppKey(request.native_request, value);
        }

        /// <summary>
        /// 口令认证. 所有的请求都必须通过SetToken方法认证通过, 才可以使用.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 申请的token字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetToken(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetToken(request.native_request, value);
        }

        /// <summary>
        /// 设置合成的音频数据编码格式.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认是pcm. 支持的格式pcm, wav, mp3。
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetFormat(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetFormat(request.native_request, value);
        }

        /// <summary>
        /// 设置音频数据采样率.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 目前支持16000, 8000. 默认是1600.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetSampleRate(SpeechSynthesizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetSampleRate(request.native_request, value);
        }

        /// <summary>
        /// 待合成音频文本内容text设置.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 待合成文本字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetText(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            byte[] text = Encoding.UTF8.GetBytes(value);
            UInt32 textSize = (UInt32)text.Length;
            return NativeMethods.SYsetText(request.native_request, text, textSize);
        }

        /// <summary>
        /// 发音人voice设置.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 发音人字符串, 包含"xiaoyun", "xiaogang". 可选参数, 默认是xiaoyun.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetVoice(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetVoice(request.native_request, value);
        }

        /// <summary>
        /// 音量volume设置.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 音量, 范围是0~100, 可选参数, 默认50.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetVolume(SpeechSynthesizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetVolume(request.native_request, value);
        }

        /// <summary>
        /// 语速speech_rate设置.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 语速, 范围是-500~500, 可选参数, 默认是0.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetSpeechRate(SpeechSynthesizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetSpeechRate(request.native_request, value);
        }

        /// <summary>
        /// 语调pitch_rate设置.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 语调, 范围是-500~500, 可选参数, 默认是0.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetPitchRate(SpeechSynthesizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetPitchRate(request.native_request, value);
        }

        /// <summary>
        /// 合成方法method设置.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 语调, 默认是0.
        /// 0 统计参数合成: 基于统计参数的语音合成, 优点是能适应的韵律特征的范围较宽, 合成器比特率低, 资源占用小, 性能高, 音质适中.
        /// 1 波形拼接合成: 基于高质量音库提取学习合成, 资源占用相对较高, 音质较好, 更加贴近真实发音, 但没有参数合成稳定.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetMethod(SpeechSynthesizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetMethod(request.native_request, value);
        }

        /// <summary>
        /// 是否开启字幕功能. 开启后可获得详细的合成信息, 比如每个字的时间戳.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetEnableSubtitle(SpeechSynthesizerRequest request, bool value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetEnableSubtitle(request.native_request, value);
        }

        /// <summary>
        /// 参数设置.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 参数.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetPayloadParam(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetPayloadParam(request.native_request, value);
        }

        /// <summary>
        /// 设置输出文本的编码格式. 默认GBK.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 编码格式 UTF-8 or GBK.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetOutputFormat(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetOutputFormat(request.native_request, value);
        }

        /// <summary>
        /// 获得设置的输出文本的编码格式.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回字符串, 否则返回unknown.</returns>
        public string GetOutputFormat(SpeechSynthesizerRequest request)
        {
            IntPtr get = NativeMethods.SYgetOutputFormat(request.native_request);
            string format = "unknown";
            if (get != IntPtr.Zero)
            {
                format = Marshal.PtrToStringAnsi(get);
            }
            return format;
        }

        /// <summary>
        /// 设置Socket接收超时时间.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 超时时间.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetTimeout(SpeechSynthesizerRequest request, int value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetTimeout(request.native_request, value);
        }

        /// <summary>
        /// 设置用户自定义参数.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 参数.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int SetContextParam(SpeechSynthesizerRequest request, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYsetContextParam(request.native_request, value);
        }

        /// <summary>
        /// 设置用户自定义ws阶段http header参数.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="key">
        /// 参数名称.
        /// </param>
        /// <param name="value">
        /// 参数内容.
        /// </param>
        /// <returns>成功则返回0, 否则返回负值错误码.</returns>
        public int AppendHttpHeaderParam(SpeechSynthesizerRequest request, string key, string value)
        {
            if (request.native_request == IntPtr.Zero)
            {
                return (int)NlsResultCode.NativeRequestEmpty;
            }
            return NativeMethods.SYappendHttpHeaderParam(request.native_request, key, value);
        }
        #endregion


        #region Set CallbackDelegate of SpeechSynthesizer
        /// <summary>
        /// 从Native获取NlsEvent
        /// </summary>
        /// <returns>返回语音合成数据长度.</returns>
        private static int GetNlsEvent(out NLS_EVENT_STRUCT nls)
        {
            return NativeMethods.SYGetNlsEvent(out nls);
        }

        NlsCallbackDelegate onSynthesisTaskFailed =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onSynthesisDataReceived =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onSynthesisCompleted =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onSynthesisClosed =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        NlsCallbackDelegate onMetaInfo =
            (handler) =>
            {
                SpeechParamStruct callback = new SpeechParamStruct();
                callback = (SpeechParamStruct)Marshal.PtrToStructure(handler, typeof(SpeechParamStruct));
                int ret = GetNlsEvent(out callback.nlsEvent);
                callback.callback(ref callback.nlsEvent, ref callback.user);
            };
        #endregion

        #region Set Callback of SpeechSynthesizer
        /// <summary>
        /// 设置错误回调函数. 在请求过程中出现异常错误时, sdk内部线程上报该回调.
        /// 用户可以在事件的消息头中检查状态码和状态消息, 以确认失败的具体原因.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnTaskFailed(
            SpeechSynthesizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SYOnTaskFailed(request.native_request, onSynthesisTaskFailed, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置语音合成二进制音频数据接收回调函数. 接收到服务端发送的二进制音频数据时，sdk内部线程上报该回调函数.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnBinaryDataReceived(
            SpeechSynthesizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SYOnBinaryDataReceived(request.native_request, onSynthesisDataReceived, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置语音合成结束回调函数, 在语音合成完成时，sdk内部线程该回调上报.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnSynthesisCompleted(
            SpeechSynthesizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SYOnSynthesisCompleted(request.native_request, onSynthesisCompleted, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置通道关闭回调函数, 语音合成连接通道关闭时，sdk内部线程该回调上报.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnChannelClosed(
            SpeechSynthesizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SYOnChannelClosed(request.native_request, onSynthesisClosed, (IntPtr)toCppParam);
            return;
        }

        /// <summary>
        /// 设置文本对应的日志信息接收回调函数, 
        /// 接收到服务端送回文本对应的日志信息，增量返回对应的字幕信息时，sdk内部线程上报该回调函数.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <param name="callback">
        /// 用户传入的回调函数.
        /// </param>
        /// <param name="para">
        /// 用户信息.
        /// </param>
        /// <returns></returns>
        public void SetOnMetaInfo(
            SpeechSynthesizerRequest request, CallbackDelegate callback, string para = null)
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
            NativeMethods.SYOnMetaInfo(request.native_request, onMetaInfo, (IntPtr)toCppParam);
            return;
        }
        #endregion
    }
}
