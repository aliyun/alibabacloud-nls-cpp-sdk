@echo off

set winRar="C:\Program Files (x86)\WinRAR\WinRAR.exe"
set VCTargetsPath=D:\Microsoft Visual Studio\2022\Community\Msbuild\Microsoft\VC\v160
set vsVersion=Visual Studio 14 2015
set libInstallPath=%~dp0lib\windows\14.0
set vcDir="%VS140COMNTOOLS%..\..\VC\bin"
set vc32Dir="%VS140COMNTOOLS%..\..\VC\bin\amd64_x86"

set path=C:\Strawberry\perl\bin;%path%;
set path=C:\Program Files\NASM;%path%;
set path=D:\Microsoft Visual Studio\Shared\NuGetPackages\microsoft.windows.sdk.buildtools\10.0.22000.194\bin\10.0.22000.0\x86;%path%;
set path=D:\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%path%;
set path=D:\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64;%path%;

echo vs version: %vsVersion%
echo vc dir: %vcDir%
echo vc32 dir: %vc32Dir%
echo lib install path: %libInstallPath%

rem set tmpBuild32Dir=build_lib32

set project_folder=%~dp0..\
set base_folder=%project_folder%build
set build_folder=%base_folder%\build_win32
set install_folder=%base_folder%\install\NlsSdk3.X_win32
set install_include_folder=%base_folder%\install\NlsSdk3.X_win32\include
set install_demo_folder=%base_folder%\install\NlsSdk3.X_win32\demo
set install_bin_folder=%base_folder%\install\NlsSdk3.X_win32\bin

set sdk_folder=%build_folder%\nlsCppSdk
set thirdparty_folder=%build_folder%\thirdparty

set openssl_folder=%thirdparty_folder%\openssl-prefix
set openssl_lib_folder=%openssl_folder%\lib

set jsoncpp_folder=%thirdparty_folder%\jsoncpp-prefix
set jsoncpp_lib_folder=%jsoncpp_folder%\lib

set event_folder=%thirdparty_folder%\libevent-prefix
set event_lib_folder=%event_folder%\lib

set opus_folder=%thirdparty_folder%\opus-prefix
set opus_lib_folder=%opus_folder%\lib

set uuid_folder=%thirdparty_folder%\uuid-prefix
set uuid_include_folder=%uuid_folder%\include

set log4cpp_folder=%thirdparty_folder%\log4cpp-prefix
set log4cpp_lib_folder=%log4cpp_folder%\lib


::initBuildDirectory
	echo Prepare the build directory.

	echo base_folder: %base_folder%
	if exist %base_folder% (
		echo %base_folder% is exist
	) else (
		echo %base_folder% is not exist
		md %base_folder%
	)

	echo build_folder: %build_folder%
	if exist %build_folder% (
		echo %build_folder% is exist
	) else (
		echo %build_folder% is not exist
		md %build_folder%
	)

	echo install_folder: %install_folder%
	if exist %install_folder% (
		echo %install_folder% is exist
	) else (
		echo %install_folder% is not exist
		md %install_folder%
	)
	
	echo sdk_folder: %sdk_folder%
	if exist %sdk_folder% (
		echo %sdk_folder% is exist
	) else (
		echo %sdk_folder% is not exist
		md %sdk_folder%
	)
	
	echo thirdparty_folder: %thirdparty_folder%
	if exist %thirdparty_folder% (
		echo %thirdparty_folder% is exist
	) else (
		echo %thirdparty_folder% is not exist
		md %thirdparty_folder%
	)

::goto:eof


rem build X86

call %vc32Dir%\vcvarsamd64_x86.bat



::buildOpenssl
	echo Begin build openssl-1.1.1l

	if exist %openssl_lib_folder% (
		echo %openssl_lib_folder% is exist
	) else (
		echo %openssl_lib_folder% is not exist
		md %openssl_lib_folder%
		md %openssl_lib_folder%\x86\Debug
		md %openssl_lib_folder%\x86\Release
	)
	if exist %openssl_folder% (
		echo %openssl_folder% is exist
	) else (
		echo %openssl_folder% is not exist
		md %openssl_folder%
	)

	echo cd %openssl_folder%
	cd %openssl_folder%
	
	echo unzip %project_folder%thirdparty\openssl-1.1.1l.tar.gz to %openssl_folder%
 	%winRar% x -ad -y "%project_folder%thirdparty\openssl-1.1.1l.tar.gz"

	set openssl_src_folder=%openssl_folder%\openssl-1.1.1l\openssl-1.1.1l
	echo cd %openssl_src_folder%
	cd %openssl_src_folder%


rem X86 Release
	echo Begin build X86 Release
	set prefix_dir=%openssl_folder%\x86\Release
	set openssl_dir=%openssl_folder%\x86\Release\ssl
	if exist %prefix_dir% (
		echo %prefix_dir% is exist
	) else (
		echo %prefix_dir% is not exist
		md %prefix_dir%
		md %openssl_dir%
	)
	perl Configure VC-WIN32 threads no-asm no-shared --prefix=%prefix_dir% --openssldir=%openssl_dir%
	nmake
::	nmake test
	nmake install
	nmake clean

	copy /y %prefix_dir%\lib\libcrypto.lib %openssl_lib_folder%\x86\Release
	copy /y %prefix_dir%\lib\libssl.lib %openssl_lib_folder%\x86\Release
	copy /y %prefix_dir%\lib\ossl_static.pdb %openssl_lib_folder%\x86\Release
	
rem X86 Debug
	echo Begin build X86 Debug
	set prefix_dir=%openssl_folder%\x86\Debug
	set openssl_dir=%openssl_folder%\x86\Debug\ssl
	if exist %prefix_dir% (
		echo %prefix_dir% is exist
	) else (
		echo %prefix_dir% is not exist
		md %prefix_dir%
		md %openssl_dir%
	)
	perl Configure VC-WIN32 threads no-asm no-shared --debug --prefix=%prefix_dir% --openssldir=%openssl_dir%
	nmake
::	nmake test
	nmake install
	nmake clean
	
	copy /y %prefix_dir%\lib\libcrypto.lib %openssl_lib_folder%\x64\Debug
	copy /y %prefix_dir%\lib\libssl.lib %openssl_lib_folder%\x64\Debug
	copy /y %prefix_dir%\lib\ossl_static.pdb %openssl_lib_folder%\x64\Debug

pause
::goto:eof


::buildLibevent
	echo Begin build Libevent.

	if exist %event_lib_folder% (
		echo %event_lib_folder% is exist
	) else (
		echo %event_lib_folder% is not exist
		md %event_lib_folder%
		md %event_lib_folder%\x86\Debug
		md %event_lib_folder%\x86\Release
	)
	if exist %event_folder% (
		echo %event_folder% is exist
	) else (
		echo %event_folder% is not exist
		md %event_folder%
	)
	
	echo cd %event_folder%
	cd %event_folder%
	
	echo unzip %project_folder%thirdparty\libevent-2.1.12-stable.tar.gz to %event_folder%
 	%winRar% x -ad -y "%project_folder%thirdparty\libevent-2.1.12-stable.tar.gz"

	set event_src_folder=%event_folder%\libevent-2.1.12-stable\libevent-2.1.12-stable
	echo cd %event_src_folder%
	cd %event_src_folder%

	md build
	cd build
	xcopy %project_folder%thirdparty\libevent_win_prj\* %event_src_folder%\build\ /y/s

		
rem x86	
	
	set prefix_dir=%event_src_folder%\x86\Release
	if exist %prefix_dir% (
		echo %prefix_dir% is exist
	) else (
		echo %prefix_dir% is not exist
		md %prefix_dir%
	)
	
	cmake ./ -G "%vsVersion% Win64" .. -DEVENT__DISABLE_OPENSSL=ON
	start libevent.sln /p:Configuration=Release;Platform=x86 /t:Clean,Build	
	
	copy /y %prefix_dir%\lib\libevent.lib %event_lib_folder%\x86\Release
	
	
	set prefix_dir=%event_src_folder%\x64\Debug
	if exist %prefix_dir% (
		echo %prefix_dir% is exist
	) else (
		echo %prefix_dir% is not exist
		md %prefix_dir%
	)
	
	cmake ./ -G "%vsVersion% Win64" .. -DEVENT__DISABLE_OPENSSL=ON
	start libevent.sln /p:Configuration=Debug;Platform=x86 /t:Clean,Build	
	
	copy /y %prefix_dir%\lib\libevent.lib %event_lib_folder%\x86\Debug

	
::goto:eof


::buildLog4Cpp
	echo Begin build Log4Cpp.
	
	if exist %log4cpp_lib_folder% (
		echo %log4cpp_lib_folder% is exist
	) else (
		echo %log4cpp_lib_folder% is not exist
		md %log4cpp_lib_folder%
		md %log4cpp_lib_folder%\x86\Debug
		md %log4cpp_lib_folder%\x86\Release
	)
	if exist %log4cpp_folder% (
		echo %log4cpp_folder% is exist
	) else (
		echo %log4cpp_folder% is not exist
		md %log4cpp_folder%
	)
	
	echo cd %log4cpp_folder%
	cd %log4cpp_folder%
	
	echo unzip %project_folder%thirdparty\log4cpp-1.1.3.tar.gz to %log4cpp_folder%
	%winRar% x -ad -y "%project_folder%thirdparty\log4cpp-1.1.3.tar.gz"

	set log4cpp_src_folder=%log4cpp_folder%\log4cpp-1.1.3\log4cpp
	echo cd %log4cpp_src_folder%
	cd %log4cpp_src_folder%
	
	
	xcopy %project_folder%thirdparty\log4cpp_win_prj\vs2015 %log4cpp_src_folder%\vs2015 /y/s
	
rem build X86
		
	msbuild vs2015\log4cpp.sln /p:Configuration=Debug /t:Clean,Build
	msbuild vs2015\log4cpp.sln /p:Configuration=Release /t:Clean,Build
		
rem install lib files	
	cd %log4cpp_src_folder%
	
	copy /y vs2015\log4cppLIB\Release\log4cppLib.lib %log4cpp_lib_folder%\x86\Release\log4cpp.lib
	copy /y vs2015\log4cppLIB\Release\log4cppLib.pdb %log4cpp_lib_folder%\x86\Release\log4cpp.pdb
	copy /y vs2015\log4cppLIB\Debug\log4cppD.lib %log4cpp_lib_folder%\x86\Debug\log4cpp.lib
	copy /y vs2015\log4cppLIB\Debug\log4cppLib.pdb %log4cpp_lib_folder%\x86\Debug\log4cpp.pdb

::goto:eof


::buildOpus
	echo Begin build Opus.
	
	if exist %opus_lib_folder% (
		echo %opus_lib_folder% is exist
	) else (
		echo %opus_lib_folder% is not exist
		md %opus_lib_folder%
		md %opus_lib_folder%\x86\Debug
		md %opus_lib_folder%\x86\Release
	)
	if exist %opus_folder% (
		echo %opus_folder% is exist
	) else (
		echo %opus_folder% is not exist
		md %opus_folder%
	)
	
	echo cd %opus_folder%
	cd %opus_folder%
	
	echo unzip %project_folder%thirdparty\opus-1.2.1.tar.gz to %opus_folder%
	%winRar% x -ad -y "%project_folder%thirdparty\opus-1.2.1.tar.gz"

	set opus_src_folder=%opus_folder%\opus-1.2.1\opus-1.2.1
	cd %opus_src_folder%

	msbuild win32\VS2015\opus.sln  /p:Configuration=Debug;Platform=Win32 /t:Clean;Build
	msbuild win32\VS2015\opus.sln  /p:Configuration=Release;Platform=Win32 /t:Clean;Build

rem install lib files	
	copy /y win32\VS2015\x86\Debug\opus.lib %opus_lib_folder%\x86\Debug
	copy /y win32\VS2015\x86\Debug\opus.pdb %opus_lib_folder%\x86\Debug
	copy /y win32\VS2015\x86\Release\opus.lib %opus_lib_folder%\x86\Release
	copy /y win32\VS2015\x86\Release\opus.pdb %opus_lib_folder%\x86\Release
	
rem 	pause
::goto:eof


::buildJsoncpp
	echo Begin build JsonCpp.
	
	if exist %jsoncpp_lib_folder% (
		echo %jsoncpp_lib_folder% is exist
	) else (
		echo %jsoncpp_lib_folder% is not exist
		md %jsoncpp_lib_folder%
		md %jsoncpp_lib_folder%\x86\Debug
		md %jsoncpp_lib_folder%\x86\Release
	)
	if exist %jsoncpp_folder% (
		echo %jsoncpp_folder% is exist
	) else (
		echo %jsoncpp_folder% is not exist
		md %jsoncpp_folder%
	)
	
	echo cd %jsoncpp_folder%
	cd %jsoncpp_folder%
	
	echo unzip %project_folder%thirdparty\jsoncpp-1.9.4.zip to %jsoncpp_folder%
	%winRar% x -ad -y "%project_folder%thirdparty\jsoncpp-1.9.4.zip" %jsoncpp_folder%
	
	set jsoncpp_src_folder=%jsoncpp_folder%\jsoncpp-1.9.4\jsoncpp-master
	echo cd %jsoncpp_src_folder%
	cd %jsoncpp_src_folder%
	
	echo copy %project_folder%thirdparty\jsoncpp_win_prj\makefiles to %jsoncpp_src_folder%\makefiles
	md %jsoncpp_src_folder%\makefiles
	md %jsoncpp_src_folder%\makefiles\vs2015
	copy /y %project_folder%thirdparty\jsoncpp_win_prj\makefiles\vs2015 %jsoncpp_src_folder%\makefiles\vs2015
	
rem build X86

	msbuild makefiles\vs2015\jsoncpp.sln  /p:Configuration=Debug;Platform=Win32 /t:Build
	msbuild makefiles\vs2015\jsoncpp.sln  /p:Configuration=Release;Platform=Win32 /t:Build

rem install lib files	

	copy /y %jsoncpp_src_folder%\makefiles\vs2015\x86\Debug\lib_json.lib %jsoncpp_lib_folder%\x86\Debug\lib_json.lib
	copy /y %jsoncpp_src_folder%\makefiles\vs2015\x86\Debug\lib_json.pdb %jsoncpp_lib_folder%\x86\Debug\lib_json.pdb
	copy /y %jsoncpp_src_folder%\makefiles\vs2015\x86\Release\lib_json.lib %jsoncpp_lib_folder%\x86\Release\lib_json.lib
	copy /y %jsoncpp_src_folder%\makefiles\vs2015\x86\Release\lib_json.pdb %jsoncpp_lib_folder%\x86\Release\lib_json.pdb

::goto:eof


::buildNlsSdk
	echo Begin build NlsSdk.
	
rem x86	 Debug
	cd %build_folder%
	rd /S /Q %sdk_folder%
	md %sdk_folder%
	cd %sdk_folder%
	cmake -G "%vsVersion% Win32" ..\.. -DENABLE_X86=ON
	msbuild nlsCppSdk2.0.sln /p:Configuration=Debug /t:Clean,Build
	copy /y nlsCppSdk\Debug\alibabacloud-idst-speech.lib %libInstallPath%\x86\Debug
	copy /y nlsCppSdk\Debug\alibabacloud-idst-speech.dll %libInstallPath%\x86\Debug
	
rem x86	Release
	cd %build_folder%
	rd /S /Q %sdk_folder%
	md %sdk_folder%
	cd %sdk_folder%
	cmake -G "%vsVersion% Win32" ..\.. -DENABLE_Release=ON -DENABLE_X86=ON
	msbuild nlsCppSdk2.0.sln /p:Configuration=Release /t:Clean,Build
	copy /y nlsCppSdk\Release\alibabacloud-idst-speech.lib %libInstallPath%\x86\Release
	copy /y nlsCppSdk\Release\alibabacloud-idst-speech.dll %libInstallPath%\x86\Release
	
rem 	pause
::goto:eof	



::install

set pack_base_folder=%install_folder%\lib\14.0

set x86_folder=%pack_base_folder%\x86
set x86_debug_folder=%x64_folder%\Debug
set x86_release_folder=%x64_folder%\Release
set build_x86_debug_folder=%build_folder%\nlsCppSdk\x86\Debug
set build_x86_release_folder=%build_folder%\nlsCppSdk\x86\Release


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


rem 拷贝全部库

copy /y %build_x86_debug_folder%\nlsCppSdk.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\nlsCppSdk.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\nlsCppSdk.pdb %x86_debug_folder%\

copy /y %build_x86_debug_folder%\libssl-1_1-x64.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libssl-1_1-x64.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libssl-1_1-x64.pdb %x86_debug_folder%\

copy /y %build_x86_debug_folder%\libcrypto-1_1-x64.lib %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libcrypto-1_1-x64.dll %x86_debug_folder%\
copy /y %build_x86_debug_folder%\libcrypto-1_1-x64.pdb %x86_debug_folder%\

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

copy /y %build_x86_release_folder%\libssl-1_1-x64.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\libssl-1_1-x64.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\libssl-1_1-x64.pdb %x86_release_folder%\

copy /y %build_x86_release_folder%\libcrypto-1_1-x64.lib %x86_release_folder%\
copy /y %build_x86_release_folder%\libcrypto-1_1-x64.dll %x86_release_folder%\
copy /y %build_x86_release_folder%\libcrypto-1_1-x64.pdb %x86_release_folder%\

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


rem 拷贝头文件

copy /y %project_folder%\nlsCppSdk\framework\feature\da\dialogAssistantRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\feature\sr\speechRecognizerRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\feature\st\speechTranscriberRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\feature\sy\speechSynthesizerRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\common\nlsClient.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\common\nlsEvent.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\common\nlsGlobal.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\framework\item\iNlsRequest.h %install_include_folder%\
copy /y %project_folder%\nlsCppSdk\token\include\nlsToken.h %install_include_folder%\


rem 拷贝demo源代码

copy /y %project_folder%\demo\Windows\* %install_demo_folder%\

copy /y %project_folder%\version %install_folder%\
copy /y %project_folder%\readme.md %install_folder%\

copy /y %build_x86_release_folder%\speechTranscriberDemo.exe %install_bin_folder%\stReleaseDemo.exe
copy /y %build_x86_debug_folder%\speechTranscriberDemo.exe %install_bin_folder%\stDebugDemo.exe

::----------------------------

cd %install_folder%\..

rem 压缩
%winRar% a -r "%install_folder%\..\NlsSdk3.X_win32.zip" .\NlsSdk3.X_win32
	


rem pause



