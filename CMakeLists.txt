﻿# CMakeList.txt : CMake project for parallel-computing, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("parallel-computing")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden) 

if(WIN32)
  add_compile_options(/utf-8 /WX /bigobj)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# Add source to this project's executable.
add_executable (parallel-computing "parallel-computing.cpp" )

# thread
if(WIN32)
else()
	find_package(TBB CONFIG REQUIRED)
    target_link_libraries(parallel-computing PRIVATE TBB::tbb)
	find_package(Threads REQUIRED)
	target_link_libraries(parallel-computing PRIVATE Threads::Threads)
endif()


# TODO: Add tests and install targets if needed.
