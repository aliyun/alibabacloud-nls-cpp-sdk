#!/bin/bash -e

if [ ! -d "resource/audio/test0.wav" ];then
  cp resource/audio/test0.wav ./
fi
if [ ! -d "resource/audio/test1.wav" ];then
  cp resource/audio/test1.wav ./
fi
if [ ! -d "resource/audio/test2.wav" ];then
  cp resource/audio/test2.wav ./
fi
if [ ! -d "resource/audio/test3.wav" ];then
  cp resource/audio/test3.wav ./
fi

cp ../lib/*.so ./

g++ -fpermissive -Wno-narrowing -D_GLIBCXX_USE_CXX11_ABI=0 -o srDemo profile_scan.cpp speechRecognizerDemo.cpp -L./ -lalibabacloud-idst-speech -lpthread -lrt -lz -ldl -lanl -I../include/

g++ -fpermissive -Wno-narrowing -D_GLIBCXX_USE_CXX11_ABI=0 -o stDemo profile_scan.cpp speechTranscriberDemo.cpp -L./ -lalibabacloud-idst-speech -lpthread -lrt -lz -ldl -lanl -I../include/

g++ -fpermissive -Wno-narrowing -D_GLIBCXX_USE_CXX11_ABI=0 -o syDemo profile_scan.cpp speechSynthesizerDemo.cpp -L./ -lalibabacloud-idst-speech -lpthread -lrt -lz -ldl -lanl -I../include/

g++ -fpermissive -Wno-narrowing -D_GLIBCXX_USE_CXX11_ABI=0 -o daDemo profile_scan.cpp dialogAssistantDemo.cpp -L./ -lalibabacloud-idst-speech -lpthread -lrt -lz -ldl -lanl -I../include/

g++ -fpermissive -Wno-narrowing -D_GLIBCXX_USE_CXX11_ABI=0 -o ftDemo profile_scan.cpp fileTransferDemo.cpp -L./ -lalibabacloud-idst-speech -lpthread -lrt -lz -ldl -lanl -I../include/
