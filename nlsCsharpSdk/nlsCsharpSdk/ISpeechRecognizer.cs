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

namespace nlsCsharpSdk
{
    /// <summary>
    /// 一句话识别
    /// </summary>
    public interface ISpeechRecognizer
    {

        /// <summary>
        /// 设置一句话识别服务URL地址.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 服务url字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetUrl(SpeechRecognizerRequest request, string value);

        /// <summary>
        /// 设置appKey.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// appKey字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetAppKey(SpeechRecognizerRequest request, string value);

        /// <summary>
        /// 口令认证. 所有的请求都必须通过SetToken方法认证通过, 才可以使用.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 申请的token字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetToken(SpeechRecognizerRequest request, string value);

        /// <summary>
        /// 设置音频数据编码格式字段Format.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 传送的音频数据格式, 目前支持pcm|opus|opu.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetFormat(SpeechRecognizerRequest request, string value);

        /// <summary>
        /// 设置音频数据采样率.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 目前支持16000, 8000. 默认是1600.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetSampleRate(SpeechRecognizerRequest request, int value);

        /// <summary>
        /// 设置是否返回中间识别结果.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetIntermediateResult(SpeechRecognizerRequest request, bool value);

        /// <summary>
        /// 设置是否在后处理中添加标点.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetPunctuationPrediction(SpeechRecognizerRequest request, bool value);

        /// <summary>
        /// 设置是否在后处理中执行数字转换.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 可选参数, 默认false.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetInverseTextNormalization(SpeechRecognizerRequest request, bool value);

        /// <summary>
        /// 设置字段enable_voice_detection设置.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 是否启动自定义静音检测, 可选, 默认是False. 云端默认静音检测时间800ms.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetEnableVoiceDetection(SpeechRecognizerRequest request, bool value);

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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetMaxStartSilence(SpeechRecognizerRequest request, int value);

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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetMaxEndSilence(SpeechRecognizerRequest request, int value);

        /// <summary>
        /// 设置定制模型.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 定制模型id字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetCustomizationId(SpeechRecognizerRequest request, string value);

        /// <summary>
        /// 设置泛热词.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 定制泛热词id字符串.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetVocabularyId(SpeechRecognizerRequest request, string value);

        /// <summary>
        /// 设置Socket接收超时时间.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 超时时间.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetTimeout(SpeechRecognizerRequest request, int value);

        /// <summary>
        /// 设置输出文本的编码格式.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 编码格式 UTF-8 or GBK.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetOutputFormat(SpeechRecognizerRequest request, string value);

        /// <summary>
        /// 获得设置的输出文本的编码格式.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回字符串, 否则返回空.</returns>
        string GetOutputFormat(SpeechRecognizerRequest request);

        /// <summary>
        /// 参数设置.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 参数.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetPayloadParam(SpeechRecognizerRequest request, string value);

        /// <summary>
        /// 设置用户自定义参数.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <param name="value">
        /// 参数.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int SetContextParam(SpeechRecognizerRequest request, string value);

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
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int AppendHttpHeaderParam(SpeechRecognizerRequest request, string key, string value);


        /// <summary>
        /// 启动一句话识别. 异步操作, 成功返回started事件, 失败返回TaskFailed事件.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int Start(SpeechRecognizerRequest request);

        /// <summary>
        /// 会与服务端确认关闭, 正常停止一句话识别操作. 异步操作, 失败返回TaskFailed.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int Stop(SpeechRecognizerRequest request);

        /// <summary>
        /// 直接关闭一句话识别过程.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int Cancel(SpeechRecognizerRequest request);

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
        /// 成功则返回0, 失败返回-1.
        /// 由于音频格式不确定, 传入音频字节数和传出音频字节数, 无法通过比较判断成功与否, 故成功返回0.
        /// </returns>
        int SendAudio(SpeechRecognizerRequest request, byte[] data, UInt64 dataSize, EncoderType type);


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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        void SetOnTaskFailed(SpeechRecognizerRequest request, CallbackDelegate callback, string para = null);

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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        void SetOnRecognitionStarted(SpeechRecognizerRequest request, CallbackDelegate callback, string para = null);

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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        void SetOnRecognitionResultChanged(SpeechRecognizerRequest request, CallbackDelegate callback, string para = null);

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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        void SetOnRecognitionCompleted(SpeechRecognizerRequest request, CallbackDelegate callback, string para = null);

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
        /// 用户对象.
        /// </param>
        /// <returns></returns>
        void SetOnChannelClosed(SpeechRecognizerRequest request, CallbackDelegate callback, string para = null);
    }
}
