@echo off

set winRar="C:\Program Files (x86)\WinRAR\WinRAR.exe"

set project_folder=%~dp0..\
set base_folder=%project_folder%build
set build_folder=%base_folder%\build_win64
set install_folder=%base_folder%\install\NlsSdk3.X_win64
set install_include_folder=%base_folder%\install\NlsSdk3.X_win64\include
set install_demo_folder=%base_folder%\install\NlsSdk3.X_win64\demo
set install_bin_folder=%base_folder%\install\NlsSdk3.X_win64\bin


::install

set pack_base_folder=%install_folder%\lib\14.0

set x64_folder=%pack_base_folder%\x64
set x64_debug_folder=%x64_folder%\Debug
set x64_release_folder=%x64_folder%\Release
set build_x64_debug_folder=%build_folder%\nlsCppSdk\x64\Debug
set build_x64_release_folder=%build_folder%\nlsCppSdk\x64\Release


if exist %x64_folder% (
    echo %x64_folder% is exist
) else (
	echo %x64_folder% is not exist
	md %x64_folder%
)

if exist %x64_debug_folder% (
    echo %x64_debug_folder% is exist
) else (
	echo %x64_debug_folder% is not exist
	md %x64_debug_folder%
)

if exist %x64_release_folder% (
    echo %x64_release_folder% is exist
) else (
	echo %x64_release_folder% is not exist
	md %x64_release_folder%
)

if exist %install_include_folder% (
    echo %install_include_folder% is exist
) else (
	echo %install_include_folder% is not exist
	md %install_include_folder%
)

if exist %install_demo_folder% (
    echo %install_demo_folder% is exist
) else (
	echo %install_demo_folder% is not exist
	md %install_demo_folder%
)

if exist %install_bin_folder% (
    echo %install_bin_folder% is exist
) else (
	echo %install_bin_folder% is not exist
	md %install_bin_folder%
)

echo "Begin copy sdk_x64 files: "%install_folder%

cd %install_folder%\..


copy /y %build_x64_debug_folder%\nlsCppSdk.lib %x64_debug_folder%\
copy /y %build_x64_debug_folder%\nlsCppSdk.dll %x64_debug_folder%\
copy /y %build_x64_debug_folder%\nlsCppSdk.pdb %x64_debug_folder%\

copy /y %build_x64_debug_folder%\libssl-1_1-x64.lib %x64_debug_folder%\
copy /y %build_x64_debug_folder%\libssl-1_1-x64.dll %x64_debug_folder%\
copy /y %build_x64_debug_folder%\libssl-1_1-x64.pdb %x64_debug_folder%\

copy /y %build_x64_debug_folder%\libcrypto-1_1-x64.lib %x64_debug_folder%\
copy /y %build_x64_debug_folder%\libcrypto-1_1-x64.dll %x64_debug_folder%\
copy /y %build_x64_debug_folder%\libcrypto-1_1-x64.pdb %x64_debug_folder%\

copy /y %build_x64_debug_folder%\libeay32.lib %x64_debug_folder%\
copy /y %build_x64_debug_folder%\libeay32.dll %x64_debug_folder%\
copy /y %build_x64_debug_folder%\libeay32.pdb %x64_debug_folder%\

copy /y %build_x64_debug_folder%\ssleay32.lib %x64_debug_folder%\
copy /y %build_x64_debug_folder%\ssleay32.dll %x64_debug_folder%\
copy /y %build_x64_debug_folder%\ssleay32.pdb %x64_debug_folder%\

copy /y %build_x64_debug_folder%\libcurld.lib %x64_debug_folder%\
copy /y %build_x64_debug_folder%\libcurld.dll %x64_debug_folder%\
copy /y %build_x64_debug_folder%\libcurld.pdb %x64_debug_folder%\

copy /y %build_x64_debug_folder%\pthreadVC2.lib %x64_debug_folder%\
copy /y %build_x64_debug_folder%\pthreadVC2.dll %x64_debug_folder%\
copy /y %build_x64_debug_folder%\pthreadVC2.pdb %x64_debug_folder%\


copy /y %build_x64_release_folder%\nlsCppSdk.lib %x64_release_folder%\
copy /y %build_x64_release_folder%\nlsCppSdk.dll %x64_release_folder%\
copy /y %build_x64_release_folder%\nlsCppSdk.pdb %x64_release_folder%\

copy /y %build_x64_release_folder%\libssl-1_1-x64.lib %x64_release_folder%\
copy /y %build_x64_release_folder%\libssl-1_1-x64.dll %x64_release_folder%\
copy /y %build_x64_release_folder%\libssl-1_1-x64.pdb %x64_release_folder%\

copy /y %build_x64_release_folder%\libcrypto-1_1-x64.lib %x64_release_folder%\
copy /y %build_x64_release_folder%\libcrypto-1_1-x64.dll %x64_release_folder%\
copy /y %build_x64_release_folder%\libcrypto-1_1-x64.pdb %x64_release_folder%\

copy /y %build_x64_release_folder%\libeay32.lib %x64_release_folder%\
copy /y %build_x64_release_folder%\libeay32.dll %x64_release_folder%\
copy /y %build_x64_release_folder%\libeay32.pdb %x64_release_folder%\

copy /y %build_x64_release_folder%\ssleay32.lib %x64_release_folder%\
copy /y %build_x64_release_folder%\ssleay32.dll %x64_release_folder%\
copy /y %build_x64_release_folder%\ssleay32.pdb %x64_release_folder%\

copy /y %build_x64_release_folder%\libcurl.lib %x64_release_folder%\
copy /y %build_x64_release_folder%\libcurl.dll %x64_release_folder%\
copy /y %build_x64_release_folder%\libcurl.pdb %x64_release_folder%\

copy /y %build_x64_release_folder%\pthreadVC2.lib %x64_release_folder%\
copy /y %build_x64_release_folder%\pthreadVC2.dll %x64_release_folder%\
copy /y %build_x64_release_folder%\pthreadVC2.pdb %x64_release_folder%\


copy /y %project_folder%\nlsCppSdk\framework\feature\da\dialogAssistantRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\feature\sr\speechRecognizerRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\feature\st\speechTranscriberRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\feature\sy\speechSynthesizerRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\common\nlsClient.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\common\nlsEvent.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\common\nlsGlobal.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\item\iNlsRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\token\include\nlsToken.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\token\include\FileTrans.h %install_include_folder%\


copy /y %project_folder%\demo\Windows\* %install_demo_folder%\

copy /y %project_folder%\version %install_folder%\
copy /y %project_folder%\readme.md %install_folder%\

copy /y %build_x64_release_folder%\speechTranscriberDemo.exe %install_bin_folder%\stReleaseDemo.exe
copy /y %build_x64_debug_folder%\speechTranscriberDemo.exe %install_bin_folder%\stDebugDemo.exe

copy /y %build_x64_release_folder%\speechRecognizerDemo.exe %install_bin_folder%\srReleaseDemo.exe
copy /y %build_x64_debug_folder%\speechRecognizerDemo.exe %install_bin_folder%\srDebugDemo.exe

copy /y %build_x64_release_folder%\speechSynthesizerDemo.exe %install_bin_folder%\syReleaseDemo.exe
copy /y %build_x64_debug_folder%\speechSynthesizerDemo.exe %install_bin_folder%\syDebugDemo.exe

copy /y %build_x64_release_folder%\fileTransferDemo.exe %install_bin_folder%\ftReleaseDemo.exe
copy /y %build_x64_debug_folder%\fileTransferDemo.exe %install_bin_folder%\ftDebugDemo.exe

::----------------------------

cd %install_folder%\..


%winRar% a -r "%install_folder%\..\NlsSdk3.X_win64.zip" .\NlsSdk3.X_win64
	


rem pause