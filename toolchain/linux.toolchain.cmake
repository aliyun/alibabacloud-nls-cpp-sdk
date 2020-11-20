# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_MACOSX_RPATH 0)

# specify the cross compiler
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fPIC -fvisibility=hidden -fvisibility-inlines-hidden")

#forbidden C++11
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

#CMAKE_MODULE_LINKER_FLAGS
#CMAKE_SHARED_LINKER_FLAGS
#CMAKE_STATIC_LINKER_FLAGS( -Wl,-Bsymbolic-functions)
set (CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,-Bsymbolic")
set (CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Bsymbolic")
