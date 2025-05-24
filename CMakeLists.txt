set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE HPP_FILES  CONFIGURE_DEPENDS "*.hpp")
file(GLOB_RECURSE CPP_FILES  CONFIGURE_DEPENDS "*.cpp")

add_library(lsem_log ${HPP_FILES} ${CPP_FILES})
add_library(lsem::log ALIAS lsem_log)

target_include_directories(lsem_log PUBLIC include/)
target_link_libraries(lsem_log PUBLIC fmt::fmt)

