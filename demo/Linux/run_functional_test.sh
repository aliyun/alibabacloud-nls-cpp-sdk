#!/bin/bash -e

echo "Command:"
echo "./build/demo/tests/run_functional_test.sh <step:st/fs/sr/sy/ft/rm> <meetingUrl>"
echo "eg: ./build/demo/tests/run_functional_test.sh st wss://tingwu"

STEP_FLAG=$1
MEETING_URL=$2
if [ $# == 0 ]; then
  echo "Command:"
  echo "./build/demo/tests/run_functional_test.sh <step:st/fs/sr/sy/ft/rm> <meetingUrl>"
  echo "eg: ./build/demo/tests/run_functional_test.sh st wss://tingwu"
  echo " export NLS_PRE_URL_ENV | NLS_AK_PRE_ENV | NLS_SK_PRE_ENV | NLS_APPKEY_PRE_ENV | NLS_TOKEN_PRE_ENV"
  echo " export NLS_AK_ENV | NLS_SK_ENV | NLS_APPKEY_ENV | NLS_TOKEN_ENV"
  echo " export NLS_FT_AUDIO_LINK_ENV | OSS_TEST_AUDIO_SOURCE_ENV | OSS_TEST_TEXT_SOURCE_ENV"
  exit 1
fi
if [ $# == 1 ]; then
  STEP_FLAG=$1
  MEETING_URL=""
fi

git_root_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
echo "当前GIT路径:" $git_root_path
workspace_path=$git_root_path/tests
echo "当前workspace路径:" $workspace_path
mkdir -p $workspace_path
workspace_result_path=$workspace_path/tests_results/
echo "当前result路径:" $workspace_result_path
mkdir -p $workspace_result_path

echo "STEP_FLAG:" $STEP_FLAG
echo "MEETING_URL:" $MEETING_URL

echo "环境检测 >>>"
NLS_AK_ENV_VALUE="$NLS_AK_PRE_ENV"
echo "NLS_AK_PRE_ENV:" $NLS_AK_ENV_VALUE
NLS_SK_ENV_VALUE="$NLS_SK_PRE_ENV"
echo "NLS_SK_PRE_ENV:" $NLS_SK_ENV_VALUE
NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
echo "NLS_APPKEY_PRE_ENV:" $NLS_APPKEY_ENV_VALUE
NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
echo "NLS_TOKEN_PRE_ENV:" $NLS_TOKEN_ENV_VALUE

NLS_SH_URL="wss://nls-gateway-cn-shanghai.aliyuncs.com/ws/v1"
NLS_BJ_URL="wss://nls-gateway-cn-beijing.aliyuncs.com/ws/v1"
NLS_SH_INTERNAL_URL="ws://nls-gateway-cn-shanghai-internal.aliyuncs.com:80/ws/v1"
NLS_BJ_INTERNAL_URL="ws://nls-gateway-cn-beijing-internal.aliyuncs.com:80/ws/v1"
NLS_SZ_INTERNAL_URL="ws://nls-gateway-cn-shenzhen-internal.aliyuncs.com:80/ws/v1"
NLS_PRE_URL=$NLS_PRE_URL_ENV
NLS_FT_AUDIO_LINK=$NLS_FT_AUDIO_LINK_ENV
OSS_TEST_AUDIO_SOURCE=$OSS_TEST_AUDIO_SOURCE_ENV
OSS_TEST_TEXT_SOURCE=$OSS_TEST_TEXT_SOURCE_ENV

txt_test_path=$workspace_path/testcases.txt

echo "OSS_TEST_AUDIO_SOURCE:" $OSS_TEST_AUDIO_SOURCE
echo "OSS_TEST_TEXT_SOURCE:" $OSS_TEST_TEXT_SOURCE

if [ -z "$NLS_APPKEY_ENV_VALUE" ]; then
  echo "APPKEY is invalid"
  exit 1
else
  if [ -z "$NLS_TOKEN_ENV_VALUE" ]; then
    if [ -z "$NLS_AK_ENV_VALUE" ] || [ -z "$NLS_SK_ENV_VALUE" ]; then
      echo "AK or SK is invalid"
      exit 1
    fi
  fi
fi

if [ -z "$NLS_PRE_URL" ]; then
  echo "NLS_PRE_URL is invalid"
  exit 1
fi
if [ -z "$NLS_FT_AUDIO_LINK" ]; then
  echo "NLS_FT_AUDIO_LINK is invalid"
  exit 1
fi
if [ -z "$OSS_TEST_AUDIO_SOURCE" ]; then
  echo "OSS_TEST_AUDIO_SOURCE is invalid"
  exit 1
fi
if [ -z "$OSS_TEST_TEXT_SOURCE" ]; then
  echo "OSS_TEST_TEXT_SOURCE is invalid"
  exit 1
fi

function build_resources {
  echo "oss cp " $OSS_TEST_AUDIO_SOURCE " to " $workspace_path/audio
  ossutil64 cp -rf $OSS_TEST_AUDIO_SOURCE $workspace_path/audio
  echo "oss cp " $OSS_TEST_TEXT_SOURCE " to " $workspace_path
  ossutil64 cp -f $OSS_TEST_TEXT_SOURCE $workspace_path
  echo "mv " $workspace_path/testcases_mix_300_500.txt " to " $txt_test_path
  mv $workspace_path/testcases_mix_300_500.txt $txt_test_path
}

echo "资源下载 >>>"
build_resources

run_speech_test() {
  cur_class_name=$1
  cur_class_num=$2
  cur_demo_path=$3
  cur_threads=$4
  cur_time=$5
  cur_type=$6
  cur_audio_file=$7
  cur_log_file=$8
  cur_log_file_count=$9

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $NLS_PRE_URL --appkey $NLS_APPKEY_ENV_VALUE --token $NLS_TOKEN_ENV_VALUE
   --threads $cur_threads --time $cur_time --type $cur_type
   --audioFile $audio_source_dir/$cur_audio_file --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $cur_log_file_count >
   $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1 || exit 1"
  echo "  run:" $run_cmd
  eval $run_cmd
  tail -n 16 $cur_workspace_path/$cur_class_name$cur_class_num.txt
}


run_file_transcribe_test() {
  cur_class_name=$1
  cur_class_num=$2
  cur_demo_path=$3
  cur_threads=$4
  cur_time=$5
  cur_type=$6
  cur_audio_link=$7
  cur_log_file=$8
  cur_log_file_count=$9

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --appkey $NLS_APPKEY_ENV_VALUE --akId $NLS_AK_ENV_VALUE --akSecret $NLS_SK_ENV_VALUE
   --threads $cur_threads --time $cur_time --type $cur_type
   --fileLinkUrl $cur_audio_link --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $cur_log_file_count >
   $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1 || exit 1"
  echo "  run:" $run_cmd
  eval $run_cmd
  tail -n 16 $cur_workspace_path/$cur_class_name$cur_class_num.txt
}

run_meeting_test() {
  cur_class_name=$1
  cur_class_num=$2
  cur_demo_path=$3
  cur_threads=$4
  cur_time=$5
  cur_type=$6
  cur_audio_file=$7
  cur_log_file=$8
  cur_log_file_count=$9

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $MEETING_URL --appkey default --token default
   --threads $cur_threads --time $cur_time --type $cur_type
   --audioFile $audio_source_dir/$cur_audio_file --logFile $cur_workspace_path/$cur_log_file
   --intermedia 1 --continued 1 --message 1
   --logFileCount $cur_log_file_count >
   $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1 || exit 1"
  echo "  run:" $run_cmd
  eval $run_cmd
  tail -n 16 $cur_workspace_path/$cur_class_name$cur_class_num.txt
}

run_tts_test() {
  cur_class_name=$1
  cur_class_num=$2
  cur_demo_path=$3
  cur_threads=$4
  cur_time=$5
  cur_type=$6
  cur_voice=$7
  cur_log_file=$8
  cur_log_file_count=$9

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $NLS_PRE_URL --appkey $NLS_APPKEY_ENV_VALUE --token $NLS_TOKEN_ENV_VALUE
   --threads $cur_threads --time $cur_time --format $cur_type
   --voice $cur_voice --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $cur_log_file_count >
   $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1 || exit 1"
  echo "  run:" $run_cmd
  eval $run_cmd
  tail -n 16 $cur_workspace_path/$cur_class_name$cur_class_num.txt
}


run_streaminput_tts_test() {
  cur_class_name=$1
  cur_class_num=$2
  cur_demo_path=$3
  cur_threads=$4
  cur_time=$5
  cur_voice=$6
  cur_log_file=$7
  cur_log_file_count=$8
  cur_text_file=$9

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $NLS_PRE_URL --appkey $NLS_APPKEY_ENV_VALUE --token $NLS_TOKEN_ENV_VALUE
   --threads $cur_threads --time $cur_time --format pcm
   --voice $cur_voice --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $cur_log_file_count --textFile $cur_text_file >
   $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1 || exit 1"
  echo "  run:" $run_cmd
  eval $run_cmd
  tail -n 16 $cur_workspace_path/$cur_class_name$cur_class_num.txt
}


# run ...
if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"st" ];then
  echo "开始测试实时语音识别 >>>"
  st_workspace_path=$workspace_result_path/st
  mkdir -p $st_workspace_path
  st_demo_path=$git_root_path/stDemo 
  audio_source_dir=$workspace_path/audio
  run_threads=50
  run_time_s=30
  run_log=log-Transcriber
  run_log_count=5

  echo "  >>> 1. 开始测试实时语音识别 16k开头停顿10s wav音频"
  run_speech_test st 1 $st_demo_path $run_threads $run_time_s pcm 16k/开头停顿10s今天天气怎么样.wav $run_log $run_log_count
  echo " "

  echo "  >>> 2. 开始测试实时语音识别 16k中间停顿10s wav音频"
  run_speech_test st 2 $st_demo_path $run_threads $run_time_s pcm 16k/说话中间存在10s停顿音频.wav $run_log $run_log_count
  echo " "

  echo "  >>> 3. 开始测试实时语音识别 16k opus音频"
  run_speech_test st 3 $st_demo_path $run_threads $run_time_s opus 16k/wav/13.wav $run_log $run_log_count
  echo " "

  echo "  >>> 4. 开始测试实时语音识别 8k opus音频"
  run_speech_test st 4 $st_demo_path $run_threads $run_time_s opus 8k/1channel_8K_12.wav $run_log $run_log_count
  echo " "

  echo "  >>> 5. 开始测试实时语音识别 16k静音 wav音频"
  run_speech_test st 5 $st_demo_path $run_threads $run_time_s pcm 16k/jingyin.wav $run_log $run_log_count
  echo " "

  echo "  >>> 6. 开始测试实时语音识别 16k无静音 wav音频"
  run_speech_test st 6 $st_demo_path $run_threads $run_time_s pcm 16k/jifeinosilence16k.wav $run_log $run_log_count
  echo " "

  echo "  >>> 7. 开始测试实时语音识别 16k噪音音频"
  run_speech_test st 7 $st_demo_path $run_threads $run_time_s pcm 16k/zaoyin_16K.wav $run_log $run_log_count
  echo " "

  echo "  >>> 8. 开始测试实时语音识别 16k多人长时间对话音频"
  run_speech_test st 8 $st_demo_path $run_threads $run_time_s pcm 16k/五人对话_普通话_706.wav $run_log $run_log_count
  echo " "
fi

if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"fs" ];then
  echo "开始测试流式语音合成 >>>"
  fs_workspace_path=$workspace_result_path/fs
  mkdir -p $fs_workspace_path
  fs_demo_path=$git_root_path/fsDemo 
  run_threads=2
  run_time_s=60
  run_log=log-FlowingSynthesizer
  run_log_count=5
  run_voice=longxiaoxia

  echo "  >>> 1. 开始测试流式语音合成 功能测试"
  run_streaminput_tts_test fs 1 $fs_demo_path $run_threads $run_time_s $run_voice $run_log $run_log_count $txt_test_path
  echo " "
fi

if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"sr" ];then
  echo "开始测试一句话识别 >>>"
  sr_workspace_path=$workspace_result_path/sr
  mkdir -p $sr_workspace_path
  sr_demo_path=$git_root_path/srDemo 
  audio_source_dir=$workspace_path/audio
  run_threads=50
  run_time_s=30
  run_log=log-Recognizer
  run_log_count=5

  echo "  >>> 1. 开始测试一句话识别 16k 开头停顿10s wav音频"
  run_speech_test sr 1 $sr_demo_path $run_threads $run_time_s pcm 16k/开头停顿10s今天天气怎么样.wav $run_log $run_log_count
  echo " "

  echo "  >>> 2. 开始测试一句话识别 16k opus音频"
  run_speech_test sr 2 $sr_demo_path $run_threads $run_time_s opus 16k/wav/13.wav $run_log $run_log_count
  echo " "
fi

if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"sy" ];then
  echo "开始测试语音合成 >>>"
  sy_workspace_path=$workspace_result_path/sy
  mkdir -p $sy_workspace_path
  sy_demo_path=$git_root_path/syDemo 
  run_threads=50
  run_time_s=30
  run_log=log-Synthesizer
  run_log_count=5

  echo "  >>> 1. 开始测试语音合成 16k wav音频"
  run_tts_test sy 1 $sy_demo_path $run_threads $run_time_s wav aixia $run_log $run_log_count
  echo " "

  echo "  >>> 2. 开始测试语音合成 16k pcm音频"
  run_tts_test sy 2 $sy_demo_path $run_threads $run_time_s pcm aixia $run_log $run_log_count
  echo " "

  echo "  >>> 3. 开始测试语音合成 16k mp3音频"
  run_tts_test sy 3 $sy_demo_path $run_threads $run_time_s mp3 aixia $run_log $run_log_count
  echo " "
fi

if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"ft" ];then
  echo "开始测试录音文件转写 >>>"
  ft_workspace_path=$workspace_result_path/ft
  mkdir -p $ft_workspace_path
  ft_demo_path=$git_root_path/ftDemo 
  audio_source_dir=$workspace_path/audio
  run_threads=1
  run_time_s=1
  run_log=log-FileTransfer
  run_log_count=5

  echo "  >>> 1. 开始测试录音文件转写 16k wav音频"
  NLS_AK_ENV_VALUE="$NLS_AK_ENV"
  echo "NLS_AK_VALUE:" $NLS_AK_VALUE
  NLS_SK_ENV_VALUE="$NLS_SK_ENV"
  echo "NLS_SK_ENV:" $NLS_SK_ENV_VALUE
  NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
  echo "NLS_APPKEY_ENV:" $NLS_APPKEY_ENV_VALUE

  if [ -z "$NLS_AK_ENV_VALUE" ] || [ -z "$NLS_SK_ENV_VALUE" ]; then
    echo "AK or SK is invalid"
    exit 1
  fi
  run_file_transcribe_test ft 1 $ft_demo_path $run_threads $run_time_s pcm $NLS_FT_AUDIO_LINK $run_log $run_log_count
  echo " "

  NLS_AK_ENV_VALUE="$NLS_AK_PRE_ENV"
  NLS_SK_ENV_VALUE="$NLS_SK_PRE_ENV"
  NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
fi

if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"rm" ];then
  echo "开始测试听悟实时推流 >>>"
  st_workspace_path=$workspace_result_path/st
  mkdir -p $st_workspace_path
  st_demo_path=$git_root_path/stDemo 
  audio_source_dir=$workspace_path/audio
  run_threads=1
  run_time_s=1
  run_log=log-RealMeeting
  run_log_count=5

  if [ -z "$MEETING_URL" ]; then
    echo "Meeting-Join-URL is invalid"
    exit 1
  else
    echo "  >>> 1. 开始测试听悟实时推流 16k开头停顿10s wav音频"
    run_meeting_test st 1 $st_demo_path $run_threads $run_time_s pcm 16k/开头停顿10s今天天气怎么样.wav $run_log $run_log_count
    echo " "
  fi
fi
