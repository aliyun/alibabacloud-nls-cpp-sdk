using System;
using System.IO;
using System.Threading;
using System.Windows.Forms;
using nlsCsharpSdk;

namespace nlsCsharpSdkDemo
{
    public partial class nlsCsharpSdkDemo : Form
    {
        private NlsClient nlsClient;
        private SpeechTranscriberRequest stPtr;
        private SpeechSynthesizerRequest syPtr;
        private SpeechRecognizerRequest srPtr;
        private NlsToken tokenPtr;
        private UInt64 expireTime;

        private string appKey;
        private string akId;
        private string akSecret;
        private string token;
        private string url;

        static bool running;  // 刷新Label的flag
        static string cur_nls_result;

        static bool st_send_audio_flag = false;
        static bool st_audio_loop_flag = false;
        static Thread st_send_audio;
        static string cur_st_result;
        static string cur_st_completed;
        static string cur_st_closed;

        static bool sr_send_audio_flag = false;
        static bool sr_audio_loop_flag = false;
        static Thread sr_send_audio;
        static string cur_sr_result;
        static string cur_sr_completed;
        static string cur_sr_closed;

        static string cur_sy_completed;
        static string cur_sy_closed;


        private void FlushLab()
        {
            while (running)
            {
                if (cur_nls_result != null && cur_nls_result.Length > 0)
                {
                    nlsResult.Text = cur_nls_result;
                }

                if (cur_st_result != null && cur_st_result.Length > 0)
                {
                    stResult.Text = cur_st_result;
                }
                if (cur_st_completed != null && cur_st_completed.Length > 0)
                {
                    stCompleted.Text = cur_st_completed;
                }
                if (cur_st_closed != null && cur_st_closed.Length > 0)
                {
                    stClosed.Text = cur_st_closed;
                }

                if (cur_sr_result != null && cur_sr_result.Length > 0)
                {
                    srResult.Text = cur_sr_result;
                }
                if (cur_sr_completed != null && cur_sr_completed.Length > 0)
                {
                    srCompleted.Text = cur_sr_completed;
                }
                if (cur_sr_closed != null && cur_sr_closed.Length > 0)
                {
                    srClosed.Text = cur_sr_closed;
                }

                if (cur_sy_completed != null && cur_sy_completed.Length > 0)
                {
                    syCompleted.Text = cur_sy_completed;
                }
                if (cur_sy_closed != null && cur_sy_closed.Length > 0)
                {
                    syClosed.Text = cur_sy_closed;
                }
                Thread.Sleep(200);
            }
        }
        private void STAudioLab()
        {
            string file_name = System.Environment.CurrentDirectory + @"\audio_files\test3.wav";
            System.Diagnostics.Debug.WriteLine("st audio file_name = {0}", file_name);
            FileStream fs = new FileStream(file_name, FileMode.Open, FileAccess.Read);
            BinaryReader br = new BinaryReader(fs);

            while (st_audio_loop_flag)
            {
                if (st_send_audio_flag)
                {
                    byte[] byData = br.ReadBytes((int)640);
                    if (byData.Length > 0)
                    {
                        stPtr.SendAudio(stPtr, byData, (UInt64)byData.Length, EncoderType.ENCODER_PCM);
                    }
                    else
                    {
                        br.Close();
                        fs.Dispose();
                        fs = new FileStream(file_name, FileMode.Open, FileAccess.Read);
                        br = new BinaryReader(fs);
                    }
                }
                Thread.Sleep(20);
            }
            br.Close();
            fs.Dispose();
        }

        private void SRAudioLab()
        {
            string file_name = System.Environment.CurrentDirectory + @"\audio_files\test3.wav";
            System.Diagnostics.Debug.WriteLine("sr audio file_name = {0}", file_name);
            FileStream fs = new FileStream(file_name, FileMode.Open, FileAccess.Read);
            BinaryReader br = new BinaryReader(fs);

            while (sr_audio_loop_flag)
            {
                if (sr_send_audio_flag)
                {
                    byte[] byData = br.ReadBytes((int)640);
                    if (byData.Length > 0)
                    {
                        srPtr.SendAudio(srPtr, byData, (UInt64)byData.Length, EncoderType.ENCODER_PCM);
                    }
                    else
                    {
                        br.Close();
                        fs.Dispose();
                        fs = new FileStream(file_name, FileMode.Open, FileAccess.Read);
                        br = new BinaryReader(fs);
                    }
                }
                Thread.Sleep(20);
            }
            br.Close();
            fs.Dispose();
        }

        public nlsCsharpSdkDemo()
        {
            InitializeComponent();
            System.Windows.Forms.Control.CheckForIllegalCrossThreadCalls = false;//设置该属性 为false

            nlsClient = new NlsClient();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            int ret = nlsClient.SetLogConfig("nlsLog", LogLevel.LogDebug, 400, 10);
            if (ret == 0)
                nlsResult.Text = "OpenLog Success";
            else
                nlsResult.Text = "OpenLog Failed";
        }

        private void label1_Click(object sender, EventArgs e)
        {

        }

        private void nlsCsharpSdkDemo_Load(object sender, EventArgs e)
        {

        }

        private void button1_Click_1(object sender, EventArgs e)
        {
            string version = nlsClient.GetVersion();
            nlsResult.Text = version;
        }

        private void button1_Click_2(object sender, EventArgs e)
        {
            nlsClient.StartWorkThread(-1);
            nlsResult.Text = "StartWorkThread and init NLS success.";
            running = true;
            Thread t = new Thread(FlushLab);
            t.Start();
        }

        private void button1_Click_3(object sender, EventArgs e)
        {
            nlsClient.ReleaseInstance();
            nlsResult.Text = "Release NLS success.";
        }

        #region TranscriberButton
        // create transcriber
        private void button1_Click_4(object sender, EventArgs e)
        {
            stPtr = nlsClient.CreateTranscriberRequest();
            if (stPtr.native_request != IntPtr.Zero)
            {
                nlsResult.Text = "CreateTranscriberRequest Success";
            }
            else
            {
                nlsResult.Text = "CreateTranscriberRequest Failed";
            }
            cur_st_result = "null";
            cur_st_closed = "null";
            cur_st_completed = "null";
        }

        // release transcriber
        private void button2_Click(object sender, EventArgs e)
        {
            if (stPtr.native_request != IntPtr.Zero)
            {
                nlsClient.ReleaseTranscriberRequest(stPtr);
                stPtr.native_request = IntPtr.Zero;
                nlsResult.Text = "ReleaseTranscriberRequest Success";
            }
            else
            {
                nlsResult.Text = "TranscriberRequest is nullptr";
            }
            cur_st_result = "null";
            cur_st_closed = "null";
            cur_st_completed = "null";
        }

        // start transcriber
        private void button3_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (stPtr.native_request != IntPtr.Zero)
            {
                stPtr.SetAppKey(stPtr, appKey);
                stPtr.SetToken(stPtr, token);
                stPtr.SetUrl(stPtr, url);
                stPtr.SetFormat(stPtr, "pcm");
                stPtr.SetSampleRate(stPtr, 16000);
                stPtr.SetIntermediateResult(stPtr, true);
                stPtr.SetPunctuationPrediction(stPtr, true);
                stPtr.SetInverseTextNormalization(stPtr, true);

                stPtr.SetOnTranscriptionStarted(stPtr, DemoOnTranscriptionStarted, IntPtr.Zero);
                stPtr.SetOnChannelClosed(stPtr, DemoOnTranscriptionClosed, IntPtr.Zero);
                stPtr.SetOnTaskFailed(stPtr, DemoOnTranscriptionTaskFailed, IntPtr.Zero);
                stPtr.SetOnSentenceEnd(stPtr, DemoOnSentenceEnd, IntPtr.Zero);
                stPtr.SetOnTranscriptionResultChanged(stPtr, DemoOnTranscriptionResultChanged, IntPtr.Zero);

                ret = stPtr.Start(stPtr);

                if (st_audio_loop_flag == false)
                {
                    st_audio_loop_flag = true;
                    st_send_audio = new Thread(STAudioLab);
                    st_send_audio.Start();
                }
            }

            if (ret != 0)
            {
                nlsResult.Text = "Transcriber Start failed";
            }
            else
            {
                nlsResult.Text = "Transcriber Start success";
            }
        }

        // stop transcriber
        private void btnSTstop_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (stPtr.native_request != IntPtr.Zero)
                ret = stPtr.Stop(stPtr);

            st_send_audio_flag = false;
            st_audio_loop_flag = false;
            if (ret != 0)
            {
                nlsResult.Text = "Transcriber Stop failed";
            }
            else
            {
                nlsResult.Text = "Transcriber Stop success";
            }
        }
        #endregion

        #region Info
        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            akId = tAkId.Text;
        }

        private void tAppKey_TextChanged(object sender, EventArgs e)
        {
            appKey = tAppKey.Text;
        }

        private void tAkSecret_TextChanged(object sender, EventArgs e)
        {
            akSecret = tAkSecret.Text;
        }

        private void tToken_TextChanged(object sender, EventArgs e)
        {
            token = tToken.Text;
        }

        private void tUrl_TextChanged(object sender, EventArgs e)
        {
            url = tUrl.Text;
        }
        #endregion

        #region TokenButton
        // create token
        private void button3_Click_1(object sender, EventArgs e)
        {
            int ret = -1;
            tokenPtr = nlsClient.CreateNlsToken();
            if (tokenPtr.native_token != IntPtr.Zero)
            {
                if (akId != null && akSecret != null & akId.Length > 0 && akSecret.Length > 0)
                {
                    tokenPtr.SetAccessKeyId(tokenPtr, akId);
                    tokenPtr.SetKeySecret(tokenPtr, akSecret);

                    ret = tokenPtr.ApplyNlsToken(tokenPtr);
                    if (ret == -1)
                    {
                        System.Diagnostics.Debug.WriteLine("ApplyNlsToken failed");
                        nlsResult.Text = tokenPtr.GetErrorMsg(tokenPtr);
                    }
                    else
                    {
                        System.Diagnostics.Debug.WriteLine("ApplyNlsToken success");
                        token = tokenPtr.GetToken(tokenPtr);
                        tToken.Text = token;
                        expireTime = tokenPtr.GetExpireTime(tokenPtr);
                        nlsResult.Text = "ExpireTime:" + expireTime.ToString();
                    }
                }
                else
                {
                    nlsResult.Text = "CreateToken Failed, akId or Secret is null";
                }
            }
            else
            {
                nlsResult.Text = "CreateToken Failed";
            }
        }

        // release token
        private void button4_Click(object sender, EventArgs e)
        {
            if (tokenPtr.native_token != IntPtr.Zero)
            {
                nlsClient.ReleaseNlsToken(tokenPtr);
                tokenPtr.native_token = IntPtr.Zero;
                nlsResult.Text = "ReleaseNlsToken Success";
            }
            else
            {
                nlsResult.Text = "ReleaseNlsToken is nullptr";
            }
        }
        #endregion

        #region TranscriberCallback
        private CallbackDelegate DemoOnTranscriptionStarted =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionStarted = {0}", e.msg);
                cur_st_completed = "msg : " + e.msg;

                st_send_audio_flag = true;
            };
        private CallbackDelegate DemoOnTranscriptionClosed =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionClosed = {0}", e.msg);
                cur_st_closed = "msg : " + e.msg;
            };
        private CallbackDelegate DemoOnTranscriptionTaskFailed =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionTaskFailed = {0}", e.msg);
                cur_st_completed = "msg : " + e.msg;
                st_send_audio_flag = false;
                st_audio_loop_flag = false;
            };
        private CallbackDelegate DemoOnTranscriptionResultChanged =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionResultChanged = {0}", e.result);
                cur_st_result = "middle result : " + e.result;
            };
        private CallbackDelegate DemoOnSentenceEnd =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSentenceEnd = {0}", e.msg);
                cur_st_completed = "sentenceEnd : " + e.result;
            };
        #endregion

        #region SynthesizerCallback
        private CallbackDelegate DemoOnBinaryDataReceived =
            (ref NLS_EVENT_STRUCT e) =>
            {
                //System.Diagnostics.Debug.WriteLine("DemoOnBinaryDataReceived dataSize = {0}", e.binaryDataSize);
                //System.Diagnostics.Debug.WriteLine("DemoOnBinaryDataReceived taskId = {0}", e.taskId);
                //cur_sy_completed = e.taskId + ", binaryDataSize : " + e.binaryDataSize;

                FileStream fs;
                String filename = e.taskId + ".pcm";
                if (File.Exists(filename))
                {
                    fs = new FileStream(filename, FileMode.Append, FileAccess.Write);
                }
                else
                {
                    fs = new FileStream(filename, FileMode.Create, FileAccess.Write);
                }
                fs.Write(e.binaryData, 0, e.binaryDataSize);
                fs.Close();
            };
        private CallbackDelegate DemoOnSynthesisClosed =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisClosed = {0}", e.msg);
                cur_sy_closed = "msg : " + e.msg;
            };
        private CallbackDelegate DemoOnSynthesisTaskFailed =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisTaskFailed = {0}", e.msg);
                cur_sy_completed = "msg : " + e.msg;
            };
        private CallbackDelegate DemoOnSynthesisCompleted =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisCompleted = {0}", e.msg);
                cur_sy_completed = "result : " + e.msg;
            };
        private CallbackDelegate DemoOnMetaInfo =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnMetaInfo = {0}", e.msg);
                //cur_sy_completed = "metaInfo : " + e.msg;
            };
        #endregion

        // create synthesizer
        private void button12_Click(object sender, EventArgs e)
        {
            syPtr = nlsClient.CreateSynthesizerRequest(TtsVersion.ShortTts);
            if (syPtr.native_request != IntPtr.Zero)
            {
                nlsResult.Text = "CreateSynthesizerRequest Success";
            }
            else
            {
                nlsResult.Text = "CreateSynthesizerRequest Failed";
            }
            cur_sy_closed = "null";
            cur_sy_completed = "null";
        }

        // start synthesizer
        private void button10_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (syPtr.native_request != IntPtr.Zero)
            {
                syPtr.SetAppKey(syPtr, appKey);
                syPtr.SetToken(syPtr, token);
                syPtr.SetUrl(syPtr, url);
                syPtr.SetText(syPtr, "今天天气真不错，我想去操场踢足球。");
                syPtr.SetVoice(syPtr, "siqi");
                syPtr.SetVolume(syPtr, 50);
                syPtr.SetFormat(syPtr, "wav");
                syPtr.SetSampleRate(syPtr, 16000);
                syPtr.SetSpeechRate(syPtr, 0);
                syPtr.SetPitchRate(syPtr, 0);
                syPtr.SetEnableSubtitle(syPtr, true);

                syPtr.SetOnSynthesisCompleted(syPtr, DemoOnSynthesisCompleted, IntPtr.Zero);
                syPtr.SetOnBinaryDataReceived(syPtr, DemoOnBinaryDataReceived, IntPtr.Zero);
                syPtr.SetOnTaskFailed(syPtr, DemoOnSynthesisTaskFailed, IntPtr.Zero);
                syPtr.SetOnChannelClosed(syPtr, DemoOnSynthesisClosed, IntPtr.Zero);
                syPtr.SetOnMetaInfo(syPtr, DemoOnMetaInfo, IntPtr.Zero);

                ret = syPtr.Start(syPtr);
            }

            if (ret != 0)
            {
                nlsResult.Text = "Synthesizer Start failed";
            }
            else
            {
                nlsResult.Text = "Synthesizer Start success";
            }
        }

        // cancel synthesizer
        private void button9_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (syPtr.native_request != IntPtr.Zero)
                ret = syPtr.Cancel(syPtr);

            if (ret != 0)
            {
                nlsResult.Text = "Synthesizer Cancel failed";
            }
            else
            {
                nlsResult.Text = "Synthesizer Cancel success";
            }
        }

        #region RecognizerCallback
        private CallbackDelegate DemoOnRecognitionStarted =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionStarted = {0}", e.msg);
                cur_sr_completed = "msg : " + e.msg;

                sr_send_audio_flag = true;
            };
        private CallbackDelegate DemoOnRecognitionClosed =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionClosed = {0}", e.msg);
                cur_sr_closed = "msg : " + e.msg;
            };
        private CallbackDelegate DemoOnRecognitionTaskFailed =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionTaskFailed = {0}", e.msg);
                cur_sr_completed = "msg : " + e.msg;

                sr_send_audio_flag = false;
                sr_audio_loop_flag = false;
            };
        private CallbackDelegate DemoOnRecognitionResultChanged =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionResultChanged = {0}", e.msg);
                cur_sr_result = "middle result : " + e.result;
            };
        private CallbackDelegate DemoOnRecognitionCompleted =
            (ref NLS_EVENT_STRUCT e) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionCompleted = {0}", e.msg);
                cur_sr_completed = "final result : " + e.result;
            };
        #endregion

        // create recognizer
        private void button8_Click(object sender, EventArgs e)
        {
            srPtr = nlsClient.CreateRecognizerRequest();
            if (srPtr.native_request != IntPtr.Zero)
            {
                nlsResult.Text = "CreateRecognizerRequest Success";
            }
            else
            {
                nlsResult.Text = "CreateRecognizerRequest Failed";
            }
            cur_sr_result = "null";
            cur_sr_closed = "null";
            cur_sr_completed = "null";
        }

        // start recognizer
        private void button6_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (srPtr.native_request != IntPtr.Zero)
            {
                srPtr.SetAppKey(srPtr, appKey);
                srPtr.SetToken(srPtr, token);
                srPtr.SetUrl(srPtr, url);
                srPtr.SetFormat(srPtr, "pcm");
                srPtr.SetSampleRate(srPtr, 16000);
                srPtr.SetIntermediateResult(srPtr, true);
                srPtr.SetPunctuationPrediction(srPtr, true);
                srPtr.SetInverseTextNormalization(srPtr, true);

                srPtr.SetOnRecognitionStarted(srPtr, DemoOnRecognitionStarted, IntPtr.Zero);
                srPtr.SetOnChannelClosed(srPtr, DemoOnRecognitionClosed, IntPtr.Zero);
                srPtr.SetOnTaskFailed(srPtr, DemoOnRecognitionTaskFailed, IntPtr.Zero);
                srPtr.SetOnRecognitionResultChanged(srPtr, DemoOnRecognitionResultChanged, IntPtr.Zero);
                srPtr.SetOnRecognitionCompleted(srPtr, DemoOnRecognitionCompleted, IntPtr.Zero);

                ret = srPtr.Start(srPtr);

                if (sr_audio_loop_flag == false)
                {
                    sr_audio_loop_flag = true;
                    sr_send_audio = new Thread(SRAudioLab);
                    sr_send_audio.Start();
                }
            }

            if (ret != 0)
            {
                nlsResult.Text = "Recognizer Start failed";
            }
            else
            {
                nlsResult.Text = "Recognizer Start success";
            }
        }

        // stop recognizer
        private void button5_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (srPtr.native_request != IntPtr.Zero)
                ret = srPtr.Stop(srPtr);

            sr_send_audio_flag = false;
            sr_audio_loop_flag = false;
            if (ret != 0)
            {
                nlsResult.Text = "Recognizer Stop failed";
            }
            else
            {
                nlsResult.Text = "Recognizer Stop success";
            }
        }

        // release recognizer
        private void button7_Click(object sender, EventArgs e)
        {
            if (srPtr.native_request != IntPtr.Zero)
            {
                nlsClient.ReleaseRecognizerRequest(srPtr);
                srPtr.native_request = IntPtr.Zero;
                nlsResult.Text = "ReleaseRecognizerRequest Success";
            }
            else
            {
                nlsResult.Text = "ReleaseRecognizerRequest is nullptr";
            }
            cur_sr_result = "null";
            cur_sr_closed = "null";
            cur_sr_completed = "null";
        }

        private void label6_Click(object sender, EventArgs e)
        {

        }

        private void label21_Click(object sender, EventArgs e)
        {

        }

        private void label14_Click(object sender, EventArgs e)
        {

        }

        private void btnSYrelease_Click(object sender, EventArgs e)
        {
            if (syPtr.native_request != IntPtr.Zero)
            {
                nlsClient.ReleaseSynthesizerRequest(syPtr);
                syPtr.native_request = IntPtr.Zero;
                nlsResult.Text = "ReleaseSynthesizerRequest Success";
            }
            else
            {
                nlsResult.Text = "ReleaseSynthesizerRequest is nullptr";
            }
            cur_sy_closed = "null";
            cur_sy_completed = "null";
        }

        private void syCompleted_Click(object sender, EventArgs e)
        {

        }

        private void srCompleted_Click(object sender, EventArgs e)
        {

        }
    }
}
