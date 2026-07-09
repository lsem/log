#include "log.hpp"

/*extern*/ log_level_t g_current_level = log_level_t::info;
/*extern*/ std::chrono::steady_clock::time_point g_local_epooch =
    std::chrono::steady_clock::now();
/*extern*/ std::mutex g_lock;
/*extern*/ bool g_log_level_read{false};

void set_log_level(log_level_t lvl) {
  std::unique_lock<std::mutex> ul{g_lock};
  g_current_level = lvl;
  g_log_level_read = true;
}
