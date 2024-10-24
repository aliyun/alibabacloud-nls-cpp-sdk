#!/bin/bash -e

echo "Command:"
echo "./build/demo/tests/run_pressure_test.sh <step:st/fs/sy> <num>"
echo "eg: ./build/demo/tests/run_pressure_test.sh st 0"
echo "eg: ./build/demo/tests/run_pressure_test.sh st 1-9"

STEP_FLAG=$1
NUM_FLAG=$2
START_STEP=0
END_STEP=0
if [ $# == 0 ]; then
  echo "Command:"
  echo "./build/demo/tests/run_pressure_test.sh <step:st/fs/sy>"
  echo "eg: ./build/demo/tests/run_pressure_test.sh st 0"
  echo "eg: ./build/demo/tests/run_pressure_test.sh st 1-9"
  echo " export NLS_PRE_URL_ENV | NLS_AK_PRE_ENV | NLS_SK_PRE_ENV | NLS_APPKEY_PRE_ENV | NLS_TOKEN_PRE_ENV"
  echo " export NLS_AK_ENV | NLS_SK_ENV | NLS_APPKEY_ENV | NLS_TOKEN_ENV"
  echo " export NLS_FT_AUDIO_LINK_ENV | OSS_TEST_AUDIO_SOURCE_ENV | OSS_TEST_TEXT_SOURCE_ENV"
  exit 1
fi
if [ $# == 1 ]; then
  STEP_FLAG=$1
  NUM_FLAG=0
fi

if echo "$NUM_FLAG" | grep -Eq '^[0-9]+-[0-9]+$'; then
  # Extract the start and end numbers from the string "0-15"
  START_STEP=$(echo "$NUM_FLAG" | cut -d'-' -f1)
  END_STEP=$(echo "$NUM_FLAG" | cut -d'-' -f2)
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
echo "START_STEP:" $START_STEP
echo "END_STEP:" $END_STEP

echo "环境检测 >>>"
NLS_AK_ENV_VALUE="$NLS_AK_PRE_ENV"
if [ -z "$NLS_AK_ENV_VALUE" ]; then
  NLS_AK_ENV_VALUE="$NLS_AK_ENV"
fi
echo "NLS_AK_ENV_VALUE:" $NLS_AK_ENV_VALUE

NLS_SK_ENV_VALUE="$NLS_SK_PRE_ENV"
if [ -z "$NLS_SK_ENV_VALUE" ]; then
  NLS_SK_ENV_VALUE="$NLS_SK_ENV"
fi
echo "NLS_SK_ENV_VALUE:" $NLS_SK_ENV_VALUE

NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
if [ -z "$NLS_APPKEY_ENV_VALUE" ]; then
  NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
fi
echo "NLS_APPKEY_ENV_VALUE:" $NLS_APPKEY_ENV_VALUE

NLS_TOKEN_ENV_VALUE="$NLS_TOKNE_PRE_ENV"
if [ -z "$NLS_TOKEN_ENV_VALUE" ]; then
  NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
fi
echo "NLS_TOKEN_PRE_ENV:" $NLS_TOKEN_ENV_VALUE

NLS_SH_URL="wss://nls-gateway-cn-shanghai.aliyuncs.com/ws/v1"
NLS_BJ_URL="wss://nls-gateway-cn-beijing.aliyuncs.com/ws/v1"
NLS_SH_INTERNAL_URL="ws://nls-gateway-cn-shanghai-internal.aliyuncs.com:80/ws/v1"
NLS_BJ_INTERNAL_URL="ws://nls-gateway-cn-beijing-internal.aliyuncs.com:80/ws/v1"
NLS_SZ_INTERNAL_URL="ws://nls-gateway-cn-shenzhen-internal.aliyuncs.com:80/ws/v1"
NLS_PRE_URL=$NLS_PRE_URL_ENV
NLS_URL=$NLS_PRE_URL
OSS_TEST_AUDIO_SOURCE=$OSS_TEST_AUDIO_SOURCE_ENV
OSS_TEST_TEXT_SOURCE=$OSS_TEST_TEXT_SOURCE_ENV

txt_test_path=$workspace_path/testcases.txt
RUN_LOG_COUNT=5

echo "OSS_TEST_AUDIO_SOURCE:" $OSS_TEST_AUDIO_SOURCE
echo "OSS_TEST_TEXT_SOURCE:" $OSS_TEST_TEXT_SOURCE

if [ -z "$NLS_APPKEY_ENV_VALUE" ]; then
  echo "APPKEY is invalid, please set NLS_APPKEY_ENV/NLS_APPKEY_PRE_ENV"
  exit 1
else
  if [ -z "$NLS_TOKEN_ENV_VALUE" ]; then
    if [ -z "$NLS_AK_ENV_VALUE" ] || [ -z "$NLS_SK_ENV_VALUE" ]; then
      echo "AK or SK is invalid, please set NLS_AK_ENV/NLS_AK_PRE_ENV or NLS_SK_ENV/NLS_SK_PRE_ENV or NLS_TOKEN_PRE_ENV/NLS_TOKEN_ENV"
      exit 1
    fi
  fi
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
  cur_log_file=$7
  cur_background=$8

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $NLS_URL --appkey $NLS_APPKEY_ENV_VALUE --token $NLS_TOKEN_ENV_VALUE
   --threads $cur_threads --time $cur_time --type $cur_type
   --audioDir $audio_source_dir/16k/wav --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $RUN_LOG_COUNT"
  if [ "$cur_background" -eq 2 ]; then
    run_cmd=$run_cmd" --setrlimit $cur_threads"
  fi
  run_cmd=$run_cmd" > $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1"
  if [ "$cur_background" -eq 1 ] || [ "$cur_background" -eq 2 ]; then
    run_cmd=$run_cmd" &"
  fi
  run_cmd="("$run_cmd") || exit 1"
  echo "  run:" $run_cmd
  eval $run_cmd
  tail -n 16 $cur_workspace_path/$cur_class_name$cur_class_num.txt
}

run_st_monkey_test() {
  cur_class_name=$1
  cur_class_num=$2
  cur_demo_path=$3
  cur_threads=$4
  cur_time=$5
  cur_type=$6
  cur_log_file=$7
  cur_background=$8
  cur_special_type=$9

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $NLS_URL --appkey $NLS_APPKEY_ENV_VALUE --token $NLS_TOKEN_ENV_VALUE
   --threads $cur_threads --time $cur_time --type $cur_type
   --audioDir $audio_source_dir/16k/wav --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $RUN_LOG_COUNT --special $cur_special_type >
   $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1"
  if [ "$cur_background" -eq 1 ]; then
    run_cmd=$run_cmd" &"
  fi
  run_cmd="("$run_cmd") || exit 1"
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
  cur_background=$8

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $NLS_URL --appkey $NLS_APPKEY_ENV_VALUE --token $NLS_TOKEN_ENV_VALUE
   --threads $cur_threads --time $cur_time --format pcm
   --voice $cur_voice --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $RUN_LOG_COUNT --textFile $txt_test_path >
   $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1"
  if [ "$cur_background" -eq 1 ]; then
    run_cmd=$run_cmd" &"
  fi
  run_cmd="("$run_cmd") || exit 1"
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
  cur_background=$9

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $NLS_URL --appkey $NLS_APPKEY_ENV_VALUE --token $NLS_TOKEN_ENV_VALUE
   --threads $cur_threads --time $cur_time --format $cur_type
   --voice $cur_voice --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $RUN_LOG_COUNT"
  if [ "$cur_background" -eq 2 ]; then
    run_cmd=$run_cmd" --setrlimit $cur_threads"
  fi
  run_cmd=$run_cmd" > $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1"
  if [ "$cur_background" -eq 1 ] || [ "$cur_background" -eq 2 ]; then
    run_cmd=$run_cmd" &"
  fi
  run_cmd="("$run_cmd") || exit 1"
  echo "  run:" $run_cmd
  eval $run_cmd
  tail -n 16 $cur_workspace_path/$cur_class_name$cur_class_num.txt
}

run_tts_monkey() {
  cur_class_name=$1
  cur_class_num=$2
  cur_demo_path=$3
  cur_threads=$4
  cur_time=$5
  cur_type=$6
  cur_voice=$7
  cur_log_file=$8
  cur_background=$9

  cur_workspace_path=$workspace_result_path/$cur_class_name/$cur_class_name$cur_class_num
  rm -rf $cur_workspace_path
  mkdir -p $cur_workspace_path
  cd $cur_workspace_path
  run_cmd="$cur_demo_path --url $NLS_URL --appkey $NLS_APPKEY_ENV_VALUE --token $NLS_TOKEN_ENV_VALUE
   --threads $cur_threads --time $cur_time --format $cur_type
   --voice $cur_voice --logFile $cur_workspace_path/$cur_log_file
   --logFileCount $RUN_LOG_COUNT"
  if [ "$cur_background" -eq 3 ]; then
    run_cmd=$run_cmd" --special 50"
  elif [ "$cur_background" -eq 2 ]; then
    run_cmd=$run_cmd" --setrlimit $cur_threads"
  fi
  run_cmd=$run_cmd" > $cur_workspace_path/$cur_class_name$cur_class_num.txt 2>&1"
  if [ "$cur_background" -eq 1 ] || [ "$cur_background" -eq 2 ] || [ "$cur_background" -eq 3 ]; then
    run_cmd=$run_cmd" &"
  fi
  run_cmd="("$run_cmd") || exit 1"
  echo "  run:" $run_cmd
  eval $run_cmd
  tail -n 16 $cur_workspace_path/$cur_class_name$cur_class_num.txt
}

# run ...
if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"st" ];then
  echo "开始压测实时语音识别 >>>"
  st_workspace_path=$workspace_result_path/st
  mkdir -p $st_workspace_path
  st_demo_path=$git_root_path/stDemo 
  st_monkey_path=$git_root_path/stMT
  audio_source_dir=$workspace_path/audio
  run_log=log-Transcriber
  run_1h=3600
  run_12h=43200
  run_24h=86400
  run_7d=604800
  run_time_s=$run_12h

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 1 ] && [ "$END_STEP" -ge 1 ]; }; then
    echo "  >>> 1. 开始压测实时语音识别 正常wav音频12h稳定性压测 pcm 预发环境"
    run_time_s=$run_12h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_speech_test stS 1 $st_demo_path 50 $run_time_s pcm $run_log 1
    echo "  <<< 1. 开始压测实时语音识别 正常wav音频12h稳定性压测 pcm 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 2 ] && [ "$END_STEP" -ge 2 ]; }; then
    echo "  >>> 2. 开始压测实时语音识别 正常wav音频12h稳定性压测 opus 预发环境"
    run_time_s=$run_12h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_speech_test stS 2 $st_demo_path 50 $run_time_s opus $run_log 1
    echo "  <<< 2. 开始压测实时语音识别 正常wav音频12h稳定性压测 opus 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 3 ] && [ "$END_STEP" -ge 3 ]; }; then
    echo "  >>> 3. 开始压测实时语音识别 正常wav音频12h稳定性压测 opus 上海内外环境"
    run_time_s=$run_12h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_SH_INTERNAL_URL
    run_speech_test stS 3 $st_demo_path 2 $run_time_s opus $run_log 1
    echo "  <<< 3. 开始压测实时语音识别 正常wav音频12h稳定性压测 opus 上海内外环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 4 ] && [ "$END_STEP" -ge 4 ]; }; then
    echo "  >>> 4. 开始测试实时语音识别 正常wav音频12h稳定性压测 opus 北京内网环境"
    run_time_s=$run_12h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_BJ_INTERNAL_URL
    run_speech_test stS 4 $st_demo_path 2 $run_time_s opus $run_log 1
    echo "  <<< 4. 开始测试实时语音识别 正常wav音频12h稳定性压测 opus 北京内网环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 5 ] && [ "$END_STEP" -ge 5 ]; }; then
    echo "  >>> 5. 开始测试实时语音识别 正常wav音频12h稳定性压测 opus 深圳内网环境"
    run_time_s=$run_12h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_SZ_INTERNAL_URL
    run_speech_test stS 5 $st_demo_path 2 $run_time_s opus $run_log 1
    echo "  <<< 5. 开始测试实时语音识别 正常wav音频12h稳定性压测 opus 深圳内网环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 6 ] && [ "$END_STEP" -ge 6 ]; }; then
    echo "  >>> 6. 开始测试实时语音识别 正常wav音频24h*7稳定性压测 opus 上海生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_SH_URL
    run_speech_test stS 6 $st_demo_path 2 $run_time_s opus $run_log 1
    echo "  <<< 6. 开始测试实时语音识别 正常wav音频24h*7稳定性压测 opus 上海生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 7 ] && [ "$END_STEP" -ge 7 ]; }; then
    echo "  >>> 7. 开始测试实时语音识别 正常wav音频24h*7稳定性压测 opus 北京生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_BJ_URL
    run_speech_test stS 7 $st_demo_path 2 $run_time_s opus $run_log 1
    echo "  <<< 7. 开始测试实时语音识别 正常wav音频24h*7稳定性压测 opus 北京生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 8 ] && [ "$END_STEP" -ge 8 ]; }; then
    echo "  >>> 8. 开始测试实时语音识别 200并发模拟高并发弱网场景1h压测 pcm 预发环境"
    run_time_s=$run_1h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_speech_test stS 8 $st_demo_path 200 $run_time_s pcm $run_log 1
    echo "  <<< 8. 开始测试实时语音识别 200并发模拟高并发弱网场景1h压测 pcm 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 9 ] && [ "$END_STEP" -ge 9 ]; }; then
    echo "  >>> 9. 开始测试实时语音识别 200并发模拟高并发弱网场景1h压测 opus 预发环境"
    run_time_s=$run_1h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_speech_test stS 9 $st_demo_path 200 $run_time_s opus $run_log 1
    echo "  <<< 9. 开始测试实时语音识别 200并发模拟高并发弱网场景1h压测 opus 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 10 ] && [ "$END_STEP" -ge 10 ]; }; then
    echo "  >>> 10. 开始测试实时语音识别 client端限制上下行带宽模拟弱网场景24h压测 opus 预发环境"
    run_time_s=$run_24h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_speech_test stS 10 $st_demo_path 50 $run_time_s opus $run_log 1
    echo "  <<< 10. 开始测试实时语音识别 client端限制上下行带宽模拟弱网场景24h压测 opus 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 11 ] && [ "$END_STEP" -ge 11 ]; }; then
    echo "  >>> 11. 开始测试实时语音识别 client端限制socket数模拟弱网场景24h压测 opus 预发环境"
    run_time_s=$run_24h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_speech_test stS 11 $st_demo_path 50 $run_time_s opus $run_log 2
    echo "  >>> 11. 开始测试实时语音识别 client端限制socket数模拟弱网场景24h压测 opus 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 12 ] && [ "$END_STEP" -ge 12 ]; }; then
    echo "  >>> 12. 开始测试实时语音识别 10并发24h*7 MONKEY测试 opus 上海生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_SH_URL
    run_log=log-TranscriberMonkey
    run_st_monkey_test stM 12 $st_monkey_path 10 $run_time_s opus $run_log 1 0
    echo "  <<< 12. 开始测试实时语音识别 10并发24h*7 MONKEY测试 opus 上海生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 13 ] && [ "$END_STEP" -ge 13 ]; }; then
    echo "  >>> 13. 开始测试实时语音识别 10并发24h*7 MONKEY测试 opus 北京生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_BJ_URL
    run_log=log-TranscriberMonkey
    run_st_monkey_test stM 13 $st_monkey_path 10 $run_time_s opus $run_log 1 0
    echo "  <<< 13. 开始测试实时语音识别 10并发24h*7 MONKEY测试 opus 北京生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 14 ] && [ "$END_STEP" -ge 14 ]; }; then
    echo "  >>> 14. 开始测试实时语音识别 5并发24h*7 链接时随机释放专项测试start后0-2000ms随机释放 opus 上海生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_SH_URL
    run_log=log-TranscriberMonkey
    run_st_monkey_test stM 14 $st_monkey_path 5 $run_time_s opus $run_log 1 1
    echo "  <<< 14. 开始测试实时语音识别 5并发24h*7 链接时随机释放专项测试start后0-2000ms随机释放 opus 上海生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 15 ] && [ "$END_STEP" -ge 15 ]; }; then
    echo "  >>> 15. 开始测试实时语音识别 5并发24h*7 链接时随机释放专项测试start后0-2000ms随机释放 opus 北京生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_BJ_URL
    run_log=log-TranscriberMonkey
    run_st_monkey_test stM 15 $st_monkey_path 5 $run_time_s opus $run_log 1 1
    echo "  <<< 15. 开始测试实时语音识别 5并发24h*7 链接时随机释放专项测试start后0-2000ms随机释放 opus 北京生产环境"
  fi
fi

if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"fs" ];then
  echo "开始压测流式语音合成 >>>"
  fs_workspace_path=$workspace_result_path/fs
  mkdir -p $fs_workspace_path
  fs_demo_path=$git_root_path/fsDemo
  run_1h=3600
  run_time_s=$run_1h
  run_log=log-FlowingSynthesizer
  run_voice=cosyvoice-zeyou-a7b8064

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 1 ] && [ "$END_STEP" -ge 1 ]; }; then
    echo "  >>> 1. 开始压测流式语音合成 1h性能压测 预发环境"
    run_time_s=$run_1h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_streaminput_tts_test fsS 1 $fs_demo_path 5 $run_time_s $run_voice $run_log 1
    echo "  <<< 1. 开始压测流式语音合成 1h性能压测 预发环境"
  fi
fi

if [ x${STEP_FLAG} == x"all" ] || [ x${STEP_FLAG} == x"sy" ];then
  echo "开始压测语音合成 >>>"
  sy_workspace_path=$workspace_result_path/sy
  mkdir -p $sy_workspace_path
  sy_demo_path=$git_root_path/syDemo 
  sy_monkey_path=$git_root_path/syMT
  run_log=log-Synthesizer
  run_1h=3600
  run_24h=86400
  run_7d=604800
  run_time_s=$run_12h
  run_voice=aixia

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 1 ] && [ "$END_STEP" -ge 1 ]; }; then
    echo "  >>> 1. 开始压测语音合成 200并发模拟高并发弱网场景1h压测 预发环境"
    run_time_s=$run_1h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_tts_test syS 1 $sy_demo_path 200 $run_time_s pcm $run_voice $run_log 1
    echo "  <<< 1. 开始压测语音合成 200并发模拟高并发弱网场景1h压测 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 2 ] && [ "$END_STEP" -ge 2 ]; }; then
    echo "  >>> 2. 开始压测语音合成 client端限制上下行带宽模拟弱网场景24h压测 预发环境"
    run_time_s=$run_24h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_tts_test syS 2 $sy_demo_path 50 $run_time_s pcm $run_voice $run_log 1
    echo "  <<< 2. 开始压测语音合成 client端限制上下行带宽模拟弱网场景24h压测 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 3 ] && [ "$END_STEP" -ge 3 ]; }; then
    echo "  >>> 3. 开始压测语音合成 client端限制socket数模拟弱网场景24h压测 预发环境"
    run_time_s=$run_24h
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_PRE_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_PRE_ENV"
    NLS_URL=$NLS_PRE_URL
    run_tts_test syS 3 $sy_demo_path 50 $run_time_s pcm $run_voice $run_log 2
    echo "  <<< 3. 开始压测语音合成 client端限制socket数模拟弱网场景24h压测 预发环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 4 ] && [ "$END_STEP" -ge 4 ]; }; then
    echo "  >>> 4. 开始压测语音合成 24h*7稳定性压测 上海生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_SH_URL
    run_tts_test syS 4 $sy_demo_path 2 $run_time_s pcm $run_voice $run_log 1
    echo "  <<< 4. 开始压测语音合成 24h*7稳定性压测 上海生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 5 ] && [ "$END_STEP" -ge 5 ]; }; then
    echo "  >>> 5. 开始压测语音合成 24h*7稳定性压测 北京生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_BJ_URL
    run_tts_test syS 5 $sy_demo_path 2 $run_time_s pcm $run_voice $run_log 1
    echo "  <<< 5. 开始压测语音合成 24h*7稳定性压测 北京生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 6 ] && [ "$END_STEP" -ge 6 ]; }; then
    echo "  >>> 6. 开始压测语音合成 24h*7稳定性压测 上海生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_SH_URL
    run_tts_monkey syM 6 $sy_monkey_path 5 $run_time_s pcm $run_voice $run_log 1
    echo "  <<< 6. 开始压测语音合成 24h*7稳定性压测 上海生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 7 ] && [ "$END_STEP" -ge 7 ]; }; then
    echo "  >>> 7. 开始压测语音合成 24h*7稳定性压测 北京生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_BJ_URL
    run_tts_monkey syM 7 $sy_monkey_path 5 $run_time_s pcm $run_voice $run_log 1
    echo "  <<< 7. 开始压测语音合成 24h*7稳定性压测 北京生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 8 ] && [ "$END_STEP" -ge 8 ]; }; then
    echo "  >>> 8. 开始压测语音合成 24h*7 链接时随机释放专项测试start后0-2000ms随机释放 上海生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_SH_URL
    run_tts_monkey syM 8 $sy_monkey_path 5 $run_time_s pcm $run_voice $run_log 3
    echo "  <<< 8. 开始压测语音合成 24h*7 链接时随机释放专项测试start后0-2000ms随机释放 上海生产环境"
  fi

  if { [ "$START_STEP" -eq 0 ] && [ "$END_STEP" -eq 0 ]; } || { [ "$START_STEP" -le 9 ] && [ "$END_STEP" -ge 9 ]; }; then
    echo "  >>> 9. 开始压测语音合成 24h*7 链接时随机释放专项测试start后0-2000ms随机释放 北京生产环境"
    run_time_s=$run_7d
    NLS_APPKEY_ENV_VALUE="$NLS_APPKEY_ENV"
    NLS_TOKEN_ENV_VALUE="$NLS_TOKEN_ENV"
    NLS_URL=$NLS_BJ_URL
    run_tts_monkey syM 9 $sy_monkey_path 5 $run_time_s pcm $run_voice $run_log 3
    echo "  <<< 9. 开始压测语音合成 24h*7 链接时随机释放专项测试start后0-2000ms随机释放 北京生产环境"
  fi
fi
