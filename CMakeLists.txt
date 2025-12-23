cmake_minimum_required(VERSION 3.22)

project(VanitySearch LANGUAGES CXX)

option(WITH_GPU "Build with CUDA GPU support" OFF)
option(NATIVE "Tune for the build machine CPU" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(VS_SOURCES
  Base58.cpp
  Bech32.cpp
  Int.cpp
  IntGroup.cpp
  IntMod.cpp
  Point.cpp
  Random.cpp
  SECP256K1.cpp
  Timer.cpp
  Vanity.cpp
  Wildcard.cpp
  GPU/GPUGenerate.cpp
  hash/ripemd160.cpp
  hash/ripemd160_sse.cpp
  hash/sha256.cpp
  hash/sha256_sse.cpp
  hash/sha512.cpp
  main.cpp
)

add_executable(VanitySearch ${VS_SOURCES})
target_include_directories(VanitySearch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(VanitySearch PRIVATE -Wno-write-strings)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mssse3" HAS_SSSE3)
if(HAS_SSSE3)
  target_compile_options(VanitySearch PRIVATE -mssse3)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(VanitySearch PRIVATE -O2 -fno-strict-aliasing)
endif()

if(NATIVE)
  check_cxx_compiler_flag("-march=native" HAS_MARCH_NATIVE)
  if(HAS_MARCH_NATIVE)
    target_compile_options(VanitySearch PRIVATE -march=native -mtune=native)
  endif()
endif()

find_package(Threads REQUIRED)
target_link_libraries(VanitySearch PRIVATE Threads::Threads)

if(WITH_GPU)
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)

  target_compile_definitions(VanitySearch PRIVATE WITHGPU)
  set_source_files_properties(GPU/GPUEngine.cu PROPERTIES LANGUAGE CUDA)
  target_sources(VanitySearch PRIVATE GPU/GPUEngine.cu)
  target_link_libraries(VanitySearch PRIVATE CUDA::cudart)

  # Default fatbin for recent GPUs. Override with:
  #   -DCMAKE_CUDA_ARCHITECTURES="86;90;100"
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 86 90 100)
  endif()

  set(CMAKE_CUDA_STANDARD 17)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  set_target_properties(VanitySearch PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif()
