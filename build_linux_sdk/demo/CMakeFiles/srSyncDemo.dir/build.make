# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.17

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk

# Include any dependencies generated for this target.
include demo/CMakeFiles/srSyncDemo.dir/depend.make

# Include the progress variables for this target.
include demo/CMakeFiles/srSyncDemo.dir/progress.make

# Include the compile flags for this target's objects.
include demo/CMakeFiles/srSyncDemo.dir/flags.make

demo/CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.o: demo/CMakeFiles/srSyncDemo.dir/flags.make
demo/CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.o: ../demo/Linux_Demo/speechRecognizerSyncDemo.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object demo/CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.o"
	cd /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/demo && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.o -c /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/demo/Linux_Demo/speechRecognizerSyncDemo.cpp

demo/CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.i"
	cd /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/demo && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/demo/Linux_Demo/speechRecognizerSyncDemo.cpp > CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.i

demo/CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.s"
	cd /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/demo && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/demo/Linux_Demo/speechRecognizerSyncDemo.cpp -o CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.s

# Object files for target srSyncDemo
srSyncDemo_OBJECTS = \
"CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.o"

# External object files for target srSyncDemo
srSyncDemo_EXTERNAL_OBJECTS =

demo/srSyncDemo: demo/CMakeFiles/srSyncDemo.dir/Linux_Demo/speechRecognizerSyncDemo.cpp.o
demo/srSyncDemo: demo/CMakeFiles/srSyncDemo.dir/build.make
demo/srSyncDemo: lib/libalibabacloud-idst-speech.so
demo/srSyncDemo: ../lib/linux/libalibabacloud-idst-common.so
demo/srSyncDemo: demo/CMakeFiles/srSyncDemo.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable srSyncDemo"
	cd /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/demo && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/srSyncDemo.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
demo/CMakeFiles/srSyncDemo.dir/build: demo/srSyncDemo

.PHONY : demo/CMakeFiles/srSyncDemo.dir/build

demo/CMakeFiles/srSyncDemo.dir/clean:
	cd /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/demo && $(CMAKE_COMMAND) -P CMakeFiles/srSyncDemo.dir/cmake_clean.cmake
.PHONY : demo/CMakeFiles/srSyncDemo.dir/clean

demo/CMakeFiles/srSyncDemo.dir/depend:
	cd /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/demo /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/demo /home/guogang.zhb/github/alibabacloud-nls-cpp-sdk/build_linux_sdk/demo/CMakeFiles/srSyncDemo.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : demo/CMakeFiles/srSyncDemo.dir/depend

