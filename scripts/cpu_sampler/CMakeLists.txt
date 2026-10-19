cmake_minimum_required(VERSION 3.20)
project(DogGameCpuSampler LANGUAGES CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The external context sampler requires 64-bit Windows.")
endif()

add_executable(doggame-cpu-sampler ../cpu_sampler.cpp)
target_compile_features(doggame-cpu-sampler PRIVATE cxx_std_17)
target_link_libraries(doggame-cpu-sampler PRIVATE dbghelp)
