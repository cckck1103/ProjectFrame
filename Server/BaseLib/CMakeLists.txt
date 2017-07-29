CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 
project(LibBase)
 
include_directories(
        ./include
        ./src)
 
 aux_source_directory(./src SOURCE_FILES)
 
 set(DEBUG 0)
 
 if(DEBUG EQUAL 1)
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -00 -static-libgcc -static-libstdc++")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${projectname_SOURCE_DIR}/lib/linux)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${projectname_SOURCE_DIR}/bin/linux)
 else()
    set(CMAKE_BUILD_TYPE "Release")
   set(CMAKE_CXX_FLAGS_DEBUG "-03 -static-libgcc -static-libstdc++ -finline-limit=1000 -DNDEBUG")
   set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${projectname_SOURCE_DIR}/lib/linux)
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${projectname_SOURCE_DIR}/bin/linux)
 endif()
 
 add_library(baselib STATIC ${SOURCE_FILES} ${HEADER})
 
 add_definitions(-Wall -Wno-format -Wno-invalid-offsetof -Wno-unknown-pragmas -fPIC -std=c++11)
