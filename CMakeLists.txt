cmake_minimum_required(VERSION 3.14)
project("lsem-log")

set(CMAKE_CXX_STANDARD 17)

add_library(lsem_log ${HPP_FILES} ${CPP_FILES})
target_sources(lsem_log PRIVATE src/log.hpp src/log.cpp include/lsem/log/log.hpp)
add_library(lsem::log ALIAS lsem_log)

target_include_directories(lsem_log PUBLIC include/)

# Note, fmt is expected to be provuded by hostx platform, you decide what version do you need we try to
# adjust.


option(LSEM_LOG_DEV_MODE "Development mode (tests, fmt, etc..)" OFF)

target_link_libraries(lsem_log PUBLIC fmt::fmt)

if (LSEM_LOG_DEV_MODE)
  include(FetchContent)
  set(FMT_TEST OFF CACHE INTERNAL "Disable fmt tests")
  set(FMT_INSTALL OFF CACHE INTERNAL "Disable fmt installation")
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        11.1.4
  )
  FetchContent_MakeAvailable(fmt)
  add_executable(lsem_log_test src/lsem_log_test.cpp)
  target_link_libraries(lsem_log_test PRIVATE lsem::log fmt::fmt)
endif()


