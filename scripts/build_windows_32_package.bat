@echo off

set winRar="C:\Program Files (x86)\WinRAR\WinRAR.exe"

set project_folder=%~dp0..\
set base_folder=%project_folder%build
set build_folder=%base_folder%\build_win32
set install_folder=%base_folder%\install\NlsSdk3.X_win32
set install_include_folder=%base_folder%\install\NlsSdk3.X_win32\include
set install_demo_folder=%base_folder%\install\NlsSdk3.X_win32\demo
set install_bin_folder=%base_folder%\install\NlsSdk3.X_win32\bin


::install

set pack_base_folder=%install_folder%\lib\14.0

set x86_folder=%pack_base_folder%\x86
set x86_debug_folder=%x86_folder%\Debug
set x86_release_folder=%x86_folder%\Release
set build_x86_debug_folder=%build_folder%\nlsCppSdk\Win32\Debug
set build_x86_release_folder=%build_folder%\nlsCppSdk\Win32\Release


if exist %x86_folder% (
    echo %x86_folder% is exist
) else (
	echo %x86_folder% is not exist
	md %x86_folder%
)

if exist %x86_debug_folder% (
    echo %x86_debug_folder% is exist
) else (
	echo %x86_debug_folder% is not exist
	md %x86_debug_folder%
)

if exist %x86_release_folder% (
    echo %x86_release_folder% is exist
) else (
	echo %x86_release_folder% is not exist
	md %x86_release_folder%
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

echo "Begin copy sdk_x86 files: "%install_folder%

cd %install_folder%\..


copy /y %build_x86_debug_folder%\nlsCppSdk.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\nlsCppSdk.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\nlsCppSdk.pdb %x86_debug_folder%\

copy /y %build_x86_debug_folder%\libssl-1_1.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libssl-1_1.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libssl-1_1.pdb %x86_debug_folder%\

copy /y %build_x86_debug_folder%\libcrypto-1_1.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libcrypto-1_1.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libcrypto-1_1.pdb %x86_debug_folder%\

copy /y %build_x86_debug_folder%\libeay32.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libeay32.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libeay32.pdb %x86_debug_folder%\

copy /y %build_x86_debug_folder%\ssleay32.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\ssleay32.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\ssleay32.pdb %x86_debug_folder%\

copy /y %build_x86_debug_folder%\libcurld.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libcurld.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libcurld.pdb %x86_debug_folder%\

copy /y %build_x86_debug_folder%\pthreadVC2.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\pthreadVC2.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\pthreadVC2.pdb %x86_debug_folder%\


copy /y %build_x86_release_folder%\nlsCppSdk.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\nlsCppSdk.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\nlsCppSdk.pdb %x86_release_folder%\

copy /y %build_x86_release_folder%\libssl-1_1.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\libssl-1_1.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\libssl-1_1.pdb %x86_release_folder%\

copy /y %build_x86_release_folder%\libcrypto-1_1.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\libcrypto-1_1.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\libcrypto-1_1.pdb %x86_release_folder%\

copy /y %build_x86_release_folder%\libeay32.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\libeay32.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\libeay32.pdb %x86_release_folder%\

copy /y %build_x86_release_folder%\ssleay32.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\ssleay32.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\ssleay32.pdb %x86_release_folder%\

copy /y %build_x86_release_folder%\libcurl.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\libcurl.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\libcurl.pdb %x86_release_folder%\

copy /y %build_x86_release_folder%\pthreadVC2.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\pthreadVC2.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\pthreadVC2.pdb %x86_release_folder%\


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

copy /y %build_x86_release_folder%\speechTranscriberDemo.exe %install_bin_folder%\stReleaseDemo.exe
copy /y %build_x86_debug_folder%\speechTranscriberDemo.exe %install_bin_folder%\stDebugDemo.exe

copy /y %build_x86_release_folder%\speechRecognizerDemo.exe %install_bin_folder%\srReleaseDemo.exe
copy /y %build_x86_debug_folder%\speechRecognizerDemo.exe %install_bin_folder%\srDebugDemo.exe

copy /y %build_x86_release_folder%\speechSynthesizerDemo.exe %install_bin_folder%\syReleaseDemo.exe
copy /y %build_x86_debug_folder%\speechSynthesizerDemo.exe %install_bin_folder%\syDebugDemo.exe

copy /y %build_x86_release_folder%\fileTransferDemo.exe %install_bin_folder%\ftReleaseDemo.exe
copy /y %build_x86_debug_folder%\fileTransferDemo.exe %install_bin_folder%\ftDebugDemo.exe

::----------------------------

cd %install_folder%\..


%winRar% a -r "%install_folder%\..\NlsSdk3.X_win32.zip" .\NlsSdk3.X_win32
	


rem pause