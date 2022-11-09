@echo off

set winRar="C:\Program Files (x86)\WinRAR\WinRAR.exe"


set project_folder=%~dp0..\
set base_folder=%project_folder%build
set build_folder=%base_folder%\build_win32
set install_folder=%base_folder%\install\NlsSdk3.X_win32

set sdk_folder=%build_folder%\nlsCppSdk
set thirdparty_folder=%build_folder%\thirdparty

set openssl_folder=%thirdparty_folder%\openssl-prefix
set openssl_include_folder=%openssl_folder%\include

set curl_folder=%thirdparty_folder%\curl-prefix
set curl_include_folder=%curl_folder%\include

set jsoncpp_folder=%thirdparty_folder%\jsoncpp-prefix
set jsoncpp_include_folder=%jsoncpp_folder%\include

set event_folder=%thirdparty_folder%\libevent-prefix
set event_include_folder=%event_folder%\include

set opus_folder=%thirdparty_folder%\opus-prefix
set opus_include_folder=%opus_folder%\include

set uuid_folder=%thirdparty_folder%\uuid-prefix
set uuid_include_folder=%uuid_folder%\include

set log4cpp_folder=%thirdparty_folder%\log4cpp-prefix
set log4cpp_include_folder=%log4cpp_folder%\include


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



::buildCurl
	echo Begin build curl-7.79.1

	if exist %curl_folder% (
		echo %curl_folder% is exist
	) else (
		echo %curl_folder% is not exist
		md %curl_folder%
		md %curl_include_folder%
	)

	echo cd %curl_folder%
	cd %curl_folder%
	
	echo unzip %project_folder%thirdparty\curl-7.79.1.tar.gz to %curl_folder%
 	%winRar% x -ad -y "%project_folder%thirdparty\curl-7.79.1.tar.gz"

	set curl_src_folder=%curl_folder%\curl-7.79.1\curl-7.79.1
	xcopy %curl_src_folder%\include %curl_include_folder% /y/s
	
rem pause
::goto:eof


::buildUuid
	echo Begin build uuid-1.0.3

	if exist %uuid_folder% (
		echo %uuid_folder% is exist
	) else (
		echo %uuid_folder% is not exist
		md %uuid_folder%
		md %uuid_include_folder%
	)

	echo cd %uuid_folder%
	cd %uuid_folder%
	
	echo unzip %project_folder%thirdparty\libuuid-1.0.3.tar.gz to %uuid_folder%
 	%winRar% x -ad -y "%project_folder%thirdparty\libuuid-1.0.3.tar.gz"

	set uuid_src_folder=%uuid_folder%\libuuid-1.0.3\libuuid-1.0.3
	md %project_folder%\nlsCppSdk\win\uuid
	copy /y %uuid_src_folder%\*.c %project_folder%\nlsCppSdk\win\uuid
	copy /y %uuid_src_folder%\*.h %project_folder%\nlsCppSdk\win\uuid

rem pause
::goto:eof


::buildJsoncpp
	echo Begin build JsonCpp.
	
	if exist %jsoncpp_folder% (
		echo %jsoncpp_folder% is exist
	) else (
		echo %jsoncpp_folder% is not exist
		md %jsoncpp_folder%
		md %jsoncpp_include_folder%
	)
	
	echo cd %jsoncpp_folder%
	cd %jsoncpp_folder%
	
	echo unzip %project_folder%thirdparty\jsoncpp-1.9.4.zip to %jsoncpp_folder%
	%winRar% x -ad -y "%project_folder%thirdparty\jsoncpp-1.9.4.zip"

	set jsoncpp_src_folder=%jsoncpp_folder%\jsoncpp-1.9.4\jsoncpp-master
	xcopy %jsoncpp_src_folder%\include %jsoncpp_include_folder% /y/s
	
	echo cd %jsoncpp_src_folder%
	cd %jsoncpp_src_folder%
	
	echo copy %project_folder%thirdparty\jsoncpp_win_prj\makefiles to %jsoncpp_src_folder%\makefiles
	md %jsoncpp_src_folder%\makefiles
	md %jsoncpp_src_folder%\makefiles\vs2015
	copy /y %project_folder%thirdparty\jsoncpp_win_prj\makefiles\vs2015 %jsoncpp_src_folder%\makefiles\vs2015

::goto:eof


::buildLibevent
	echo Begin build Libevent.

	if exist %event_folder% (
		echo %event_folder% is exist
	) else (
		echo %event_folder% is not exist
		md %event_folder%
		md %event_include_folder%
	)
	
	echo cd %event_folder%
	cd %event_folder%

	echo unzip %project_folder%thirdparty\libevent-2.1.12-stable.tar.gz to %event_folder%
 	%winRar% x -ad -y "%project_folder%thirdparty\libevent-2.1.12-stable.tar.gz"

	set event_src_folder=%event_folder%\libevent-2.1.12-stable\libevent-2.1.12-stable
	xcopy %event_src_folder%\include %event_include_folder% /y/s
	copy /y %event_src_folder%\WIN32-Code\nmake\event2\event-config.h %event_include_folder%\event2

	set event_prj_folder=%event_src_folder%\build
	if exist %event_prj_folder% (
		echo %event_prj_folder% is exist
	) else (
		echo %event_prj_folder% is not exist
		md %event_prj_folder%
	)
	xcopy %project_folder%thirdparty\libevent_win_prj\* %event_prj_folder% /y/s

::goto:eof


::buildOpenssl
	echo Begin build openssl-1.1.1l

	if exist %openssl_folder% (
		echo %openssl_folder% is exist
	) else (
		echo %openssl_folder% is not exist
		md %openssl_folder%
		md %openssl_include_folder%
	)

	echo cd %openssl_folder%
	cd %openssl_folder%
	
	echo unzip %project_folder%thirdparty\openssl-1.1.1l.tar.gz to %openssl_folder%
 	%winRar% x -ad -y "%project_folder%thirdparty\openssl-1.1.1l.tar.gz"

	set openssl_src_folder=%openssl_folder%\openssl-1.1.1l\openssl-1.1.1l
	xcopy %openssl_src_folder%\include %openssl_include_folder% /y/s
	copy /y %project_folder%\thirdparty\openssl_win_prj\opensslconf.h %openssl_include_folder%\openssl
	
::goto:eof



::buildLog4Cpp
	echo Begin build Log4Cpp.
	
	if exist %log4cpp_folder% (
		echo %log4cpp_folder% is exist
	) else (
		echo %log4cpp_folder% is not exist
		md %log4cpp_folder%
		md %log4cpp_include_folder%
	)
	
	echo cd %log4cpp_folder%
	cd %log4cpp_folder%
	
	echo unzip %project_folder%thirdparty\log4cpp-1.1.3.tar.gz to %log4cpp_folder%
	%winRar% x -ad -y "%project_folder%thirdparty\log4cpp-1.1.3.tar.gz"

	set log4cpp_src_folder=%log4cpp_folder%\log4cpp-1.1.3\log4cpp
	xcopy %log4cpp_src_folder%\include %log4cpp_include_folder% /y/s

	xcopy %project_folder%thirdparty\log4cpp_win_prj\vs2015 %log4cpp_src_folder%\vs2015 /y/s

::goto:eof


::buildOpus
	echo Begin build Opus.

	if exist %opus_folder% (
		echo %opus_folder% is exist
	) else (
		echo %opus_folder% is not exist
		md %opus_folder%
		md %opus_include_folder%
		md %opus_include_folder%\opus
	)
	
	echo cd %opus_folder%
	cd %opus_folder%
	
	echo unzip %project_folder%thirdparty\opus-1.2.1.tar.gz to %opus_folder%
	%winRar% x -ad -y "%project_folder%thirdparty\opus-1.2.1.tar.gz"

	set opus_src_folder=%opus_folder%\opus-1.2.1\opus-1.2.1
	xcopy %opus_src_folder%\include %opus_include_folder%\opus /y/s

rem pause
::goto:eof

