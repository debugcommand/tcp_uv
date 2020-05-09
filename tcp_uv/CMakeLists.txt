cmake_minimum_required(VERSION 2.8)
project(tcp_uv)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	inc
       	uvnet
	)

#message($(CMAKE_CURRENT_SOURCE_DIR))
#message($(CMAKE_CURRENT_BINARY_DIR))

file(GLOB SRC_REDIS "*.cpp" "redis/*.cpp")
file(GLOB SRC_LIST "*.cpp" "src/*.cpp")
file(GLOB SRC_UVNET "*.cpp" "uvnet/*.cpp")

set (LIB_UV_LIB,"/usr/local/lib")
link_directories(${LIB_UV_LIB} )
#link_directories(/usr/local/lib)
if(${CMAKE_BUILD_TYPE} MATCHES "debug")
	set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")	
endif()
	set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
#message(${CMAKE_BUILD_TYPE})
add_executable(tcp_uv ${SRC_LIST} ${SRC_UVNET} ${SRC_REDIS})
target_link_libraries(tcp_uv libuv_a.a dl pthread)
