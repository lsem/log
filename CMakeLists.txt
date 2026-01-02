set(CMAKE_CXX_STANDARD 17)

add_library(lsem_log ${HPP_FILES} ${CPP_FILES})
target_sources(lsem_log PRIVATE src/log.hpp src/log.cpp include/lsem/log/log.hpp)
add_library(lsem::log ALIAS lsem_log)

target_include_directories(lsem_log PUBLIC include/)
target_link_libraries(lsem_log PUBLIC fmt::fmt)

