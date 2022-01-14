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

namespace nlsCsharpSdk
{
    /// <summary>
    /// 实时转写
    /// </summary>
    /// 

    public class SpeechTranscriberRequest : ISpeechTranscriber
    {
        public IntPtr native_request;

        #region Start the request of speech transcriber
        /// <summary>
        /// 启动实时音频流识别. 异步操作, 成功返回started事件, 失败返回TaskFailed事件.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int Start(SpeechTranscriberRequest request)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int Stop(SpeechTranscriberRequest request)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int Cancel(SpeechTranscriberRequest request)
        {
            return NativeMethods.STcancel(request.native_request);
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
        /// 成功则返回0, 失败返回-1.
        /// 由于音频格式不确定, 传入音频字节数和传出音频字节数, 无法通过比较判断成功与否, 故成功返回0.
        /// </returns>
        public int SendAudio(SpeechTranscriberRequest request, byte[] data, UInt64 dataSize, EncoderType type)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetUrl(SpeechTranscriberRequest request, string value)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetAppKey(SpeechTranscriberRequest request, string value)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetToken(SpeechTranscriberRequest request, string value)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetFormat(SpeechTranscriberRequest request, string value)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetSampleRate(SpeechTranscriberRequest request, int value)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetIntermediateResult(SpeechTranscriberRequest request, bool value)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetPunctuationPrediction(SpeechTranscriberRequest request, bool value)
        {
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetInverseTextNormalization(SpeechTranscriberRequest request, bool value)
        {
            return NativeMethods.STsetInverseTextNormalization(request.native_request, value);
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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int SetMaxSentenceSilence(SpeechTranscriberRequest request, int value)
        {
            return NativeMethods.STsetMaxSentenceSilence(request.native_request, value);
        }
        #endregion


        #region Set CallbackDelegate of SpeechTranscriber
        static object user_started_obj = null;
        static NLS_EVENT_STRUCT nlsEvent = new NLS_EVENT_STRUCT();
        static CallbackDelegate transcriptionStartedCallback;
        static CallbackDelegate transcriptionTaskFailedCallback;
        static CallbackDelegate transcriptionResultChangedCallback;
        static CallbackDelegate transcriptionCompletedCallback;
        static CallbackDelegate transcriptionClosedCallback;
        static CallbackDelegate sentenceBeginCallback;
        static CallbackDelegate sentenceEndCallback;
        static CallbackDelegate sentenceSemanticsCallback;

        /// <summary>
        /// 从Native获取NlsEvent
        /// </summary>
        /// <returns></returns>
        private static int GetNlsEvent()
        {
            return NativeMethods.STGetNlsEvent(out nlsEvent);
        }

        NlsCallbackDelegate onTranscriptionStarted =
            (status) =>
            {
                int ret = GetNlsEvent();
                transcriptionStartedCallback(ref nlsEvent);
            };
        NlsCallbackDelegate onTranscriptionTaskFailed =
            (status) =>
            {
                int ret = GetNlsEvent();
                transcriptionTaskFailedCallback(ref nlsEvent);
            };
        NlsCallbackDelegate onTranscriptionResultChanged =
            (status) =>
            {
                int ret = GetNlsEvent();
                transcriptionResultChangedCallback(ref nlsEvent);
            };
        NlsCallbackDelegate onTranscriptionCompleted =
            (status) =>
            {
                int ret = GetNlsEvent();
                transcriptionCompletedCallback(ref nlsEvent);
            };
        NlsCallbackDelegate onTranscriptionClosed =
            (status) =>
            {
                int ret = GetNlsEvent();
                transcriptionClosedCallback(ref nlsEvent);
            };
        NlsCallbackDelegate onSentenceBegin =
            (status) =>
            {
                int ret = GetNlsEvent();
                sentenceBeginCallback(ref nlsEvent);
            };
        NlsCallbackDelegate onSentenceEnd =
            (status) =>
            {
                int ret = GetNlsEvent();
                sentenceEndCallback(ref nlsEvent);
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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        public void SetOnTranscriptionStarted(
            SpeechTranscriberRequest request, CallbackDelegate callback, object para = null)
        {
            transcriptionStartedCallback = new CallbackDelegate(callback);
            user_started_obj = para;
            NativeMethods.STOnTranscriptionStarted(request.native_request, onTranscriptionStarted);
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
        /// 用户对象.
        /// </param>
        /// <returns></returns>

        public void SetOnTaskFailed(
            SpeechTranscriberRequest request, CallbackDelegate callback, object para = null)
        {
            transcriptionTaskFailedCallback = new CallbackDelegate(callback);
            user_started_obj = para;
            NativeMethods.STOnTaskFailed(request.native_request, onTranscriptionTaskFailed);
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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        public void SetOnTranscriptionResultChanged(
            SpeechTranscriberRequest request, CallbackDelegate callback, object para = null)
        {
            transcriptionResultChangedCallback = new CallbackDelegate(callback);
            user_started_obj = para;
            NativeMethods.STOnTranscriptionResultChanged(request.native_request, onTranscriptionResultChanged);
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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        public void SetOnTranscriptionCompleted(
            SpeechTranscriberRequest request, CallbackDelegate callback, object para = null)
        {
            transcriptionCompletedCallback = new CallbackDelegate(callback);
            user_started_obj = para;
            NativeMethods.STOnTranscriptionCompleted(request.native_request, onTranscriptionCompleted);
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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        public void SetOnChannelClosed(
            SpeechTranscriberRequest request, CallbackDelegate callback, object para = null)
        {
            transcriptionClosedCallback = new CallbackDelegate(callback);
            user_started_obj = para;
            NativeMethods.STOnChannelClosed(request.native_request, onTranscriptionClosed);
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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        public void SetOnSentenceBegin(
            SpeechTranscriberRequest request, CallbackDelegate callback, object para = null)
        {
            sentenceBeginCallback = new CallbackDelegate(callback);
            user_started_obj = para;
            NativeMethods.STOnSentenceBegin(request.native_request, onSentenceBegin);
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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        public void SetOnSentenceEnd(
            SpeechTranscriberRequest request, CallbackDelegate callback, object para = null)
        {
            sentenceEndCallback = new CallbackDelegate(callback);
            user_started_obj = para;
            NativeMethods.STOnSentenceEnd(request.native_request, onSentenceEnd);
            return;
        }
        #endregion
    }
}
