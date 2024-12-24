
#Written by Nack li <nackli@163.com>
#Copyright (c) 2024. All Rights Reserved.

#version
cmake_minimum_required(VERSION 3.16)
# project name
project(TestApp VERSION 1.0.0)
set(BUILD_NUMBER "" CACHE STRING "Build number")
# set C++ standard
# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

include_directories("./rapidjson/include")

if(CMAKE_CROSSCOMPILING)
  include_directories("/home/nack/work/libhv/arm-build/install/include")
  link_directories("../libhv/arm-build/install/lib")
else()
  include_directories("../libhv/build/include")
  link_directories("../libhv/build/lib")
  #include_directories("./yaml-cpp-install/include")
  #link_directories("./yaml-cpp-install/lib")
endif()

if(MSVC) 
	#runtime library
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /Zi")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Zi")

	set(CMAKE_GENERATOR_PLATFORM Win32)
  add_compile_options(/W3 /utf-8 /bigobj /MP)
	#pdb 
	#set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG /OPT:REF /OPT:ICF")
	#set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
else()
	 set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall -O3 -fPIC")
	 set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -fPIC")
endif()	

# default user option
option(ENABLE_TEST "Enable test" OFF)

if(ENABLE_TEST)
  add_definitions(-DENABLE_TEST -D_CRT_SECURE_NO_WARNINGS)
endif()

# set msvc info
macro(add_msvc_options target)
  if(MSVC)
    target_compile_options(${target} PRIVATE
    /W4                # 设置警告级别为 4
    /WX                # 将警告视为错误
    /MP                # 启用多处理器编译
    /permissive-       # 禁用不严格的语言 conformance
    /Zc:__cplusplus    # 启用正确的 __cplusplus 宏值
    /Zc:inline         # 移除未使用的函数
    /Gm-               # 禁用最小生成（minimal rebuild）
    #/EHsc              # 指定异常处理模型
    )
  endif()
endmacro()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")


if(MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "netWorkBroad")
endif()


file(GLOB_RECURSE DEV_SOURCES_EXE ./src/*.cpp)

add_executable(netWorkBroad ${DEV_SOURCES_EXE})
target_link_libraries(netWorkBroad hv pthread)
