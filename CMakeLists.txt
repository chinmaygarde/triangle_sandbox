cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake_toolbox")
include(CMakeToolboxInitialize)

CMakeToolboxInitialize()

SetCAndCXXStandard(11 20)

EnableCCache()

project(triangle_sandbox)

cmake_policy(SET CMP0135 NEW)
include(FetchContent)

enable_testing()
include(GoogleTest)
include(CTest)


# Setup Slang
set(SLANG_VERSION 2025.6.3)
FetchContent_Declare(
  slang_sdk
  URL https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-macos-aarch64.zip
)
FetchContent_MakeAvailable(slang_sdk)

# Google Test
add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)

# Google Benchmark
set(GOOGLETEST_PATH "third_party/googletest" EXCLUDE_FROM_ALL)
set(BENCHMARK_ENABLE_TESTING FALSE)
add_subdirectory(third_party/googlebenchmark EXCLUDE_FROM_ALL)
set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(third_party/sdl EXCLUDE_FROM_ALL)
add_subdirectory(third_party/jfml EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm EXCLUDE_FROM_ALL)

# Project
add_subdirectory(src)
