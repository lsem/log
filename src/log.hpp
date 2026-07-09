#pragma once

#include <fmt/color.h>
#include <fmt/format.h>

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif //

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#elif defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>

#elif defined(__APPLE__) && defined(__MACH__)
#include <pthread.h>
#else
#error "current_native_thread_id() is not implemented for this platform"
#endif
inline std::uint64_t current_native_thread_id() noexcept {
#if defined(_WIN32)
  return static_cast<std::uint64_t>(::GetCurrentThreadId());
#elif defined(__linux__)
  return static_cast<std::uint64_t>(::syscall(SYS_gettid));
#elif defined(__APPLE__) && defined(__MACH__)
  std::uint64_t tid = 0;
  pthread_threadid_np(nullptr, &tid);
  return tid;
#endif
}

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <mutex>
#include <string>
#include <string_view>

enum class log_level_t {
  error,
  warning,
  info,
  debug,
};

extern log_level_t g_current_level;
extern std::chrono::steady_clock::time_point g_local_epooch;
extern std::mutex g_lock;
extern bool g_log_level_read;

void set_log_level(log_level_t lvl);

constexpr std::string_view strip_fpath(std::string_view fpath) {
  size_t last_slash_pos = std::string_view::npos;
  for (size_t i = 0; i < fpath.size(); ++i) {
    if (fpath[i] == '/' || fpath[i] == '\\') {
      last_slash_pos = i;
    }
  }

  if (last_slash_pos != std::string_view::npos) {
    fpath.remove_prefix(last_slash_pos + 1);
  }
  return fpath;
}

static_assert(strip_fpath("a/b/c") == "c");
static_assert(strip_fpath("main.cpp") == "main.cpp");
static_assert(strip_fpath("a\\b\\c.cpp") == "c.cpp");

namespace {
struct rgb_color {
  rgb_color() = default;
  rgb_color(int r, int g, int b) : r(r), g(g), b(b) {}
  int r = 0;
  int g = 0;
  int b = 0;
};

rgb_color to_rgb(fmt::color c) {
  const uint32_t color_value = static_cast<uint32_t>(c);
  auto b = static_cast<uint8_t>(color_value & 0x000000FF);
  auto g = static_cast<uint8_t>((color_value & 0x0000FF00) >> 8);
  auto r = static_cast<uint8_t>((color_value & 0x00FF0000) >> 16);
  return rgb_color(r, g, b);
}

fmt::color from_rgb(rgb_color c) {
  return static_cast<fmt::color>(c.r << 16 | c.g << 8 | c.b);
}

fmt::color adjust_brightness(fmt::color c, double percents) {
  auto [r, g, b] = to_rgb(c);
  r = std::clamp(r + static_cast<int>(r * percents), 0, 255);
  g = std::clamp(g + static_cast<int>(g * percents), 0, 255);
  b = std::clamp(b + static_cast<int>(b * percents), 0, 255);
  return from_rgb(rgb_color(r, g, b));
}
} // namespace

template <class... Args>
void log_impl(log_level_t level, int line, std::string_view file_name,
              std::string_view module_name, fmt::format_string<Args...> fmt,
              Args &&...args) {

  std::unique_lock<std::mutex> ul{g_lock};
  if (!g_log_level_read) {
    std::string level_val;
    if (std::getenv("LOG")) {
      level_val = std::getenv("LOG");
      if (level_val == "debug") {
        g_current_level = log_level_t::debug;
      } else if (level_val == "info") {
        g_current_level = log_level_t::info;
      } else if (level_val == "warning") {
        g_current_level = log_level_t::warning;
      } else if (level_val == "error") {
        g_current_level = log_level_t::error;
      }
    } else if (std::getenv("DEBUG")) {
      g_current_level = log_level_t::debug;
    }
    g_log_level_read = true;
  }

  if (static_cast<int>(level) > static_cast<int>(g_current_level)) {
    return;
  }

  ul.unlock();

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
  const auto at_tty = isatty(STDERR_FILENO);
#else
  const auto at_tty = false;
#endif
  const auto style = ([level, at_tty] {
    if (!at_tty) {
      return fmt::text_style{};
    }
    switch (level) {
    case log_level_t::debug:
      return fmt::fg(fmt::color::gray);
    case log_level_t::info:
      return fmt::fg(fmt::color::light_gray);
    case log_level_t::warning:
      return fmt::bg(fmt::color::yellow) | fmt::fg(fmt::color::black);
    case log_level_t::error:
      return fmt::bg(fmt::color::indian_red) | fmt::fg(fmt::color::white);
    }
    return fmt::text_style{};
  })();
  const auto darker_style = ([level, at_tty] {
    if (!at_tty) {
      return fmt::text_style{};
    }
    switch (level) {
    case log_level_t::debug:
      return fmt::fg(adjust_brightness(fmt::color::gray, -0.5));
    case log_level_t::info:
      return fmt::fg(adjust_brightness(fmt::color::light_gray, -0.5));
    case log_level_t::warning:
      return fmt::bg(fmt::color::yellow) | fmt::fg(fmt::color::black);
    case log_level_t::error:
      return fmt::bg(fmt::color::indian_red) | fmt::fg(fmt::color::white);
    }
    return fmt::text_style{};
  })();

  const auto lvl_s = [level]() -> std::string_view {
    switch (level) {
    case log_level_t::debug:
      return "DBG";
    case log_level_t::info:
      return "INF";
    case log_level_t::warning:
      return "WRN";
    case log_level_t::error:
      return "ERR";
    default:
      return "UNK";
    }
  }();

  auto curr_ms = (std::chrono::steady_clock::now() - g_local_epooch) /
                 std::chrono::milliseconds(1);

  auto tid = current_native_thread_id();

  auto out = fmt::memory_buffer();
  fmt::format_to(std::back_inserter(out), style, "{}: {} {} T{}  ", curr_ms,
                 lvl_s, module_name, tid);
  fmt::format_to(std::back_inserter(out), style, "{}",
                 fmt::vformat(fmt, fmt::make_format_args(args...)));
  fmt::format_to(std::back_inserter(out), darker_style, " ({}:{}) ",
                 strip_fpath(file_name), line);
  fmt::format_to(std::back_inserter(out), "\n");

  ul.lock();
  std::fwrite(out.data(), 1, out.size(), stderr);
}

inline void log_empty_line() {
  auto out = fmt::memory_buffer();
  fmt::format_to(std::back_inserter(out), "\n");
  std::lock_guard<std::mutex> lock{g_lock};
  std::fwrite(out.data(), 1, out.size(), stderr);
}

#ifdef _MSC_VER
#define log_error(Fmt, ...)                                                    \
  log_impl(log_level_t::error, __LINE__, __FILE__,                             \
           lsem::log::details::module_name(), FMT_STRING(Fmt), __VA_ARGS__)
#define log_warning(Fmt, ...)                                                  \
  log_impl(log_level_t::warning, __LINE__, __FILE__,                           \
           lsem::log::details::module_name(), FMT_STRING(Fmt), __VA_ARGS__)
#define log_info(Fmt, ...)                                                     \
  log_impl(log_level_t::info, __LINE__, __FILE__,                              \
           lsem::log::details::module_name(), FMT_STRING(Fmt), __VA_ARGS__)
#define log_debug(Fmt, ...)                                                    \
  log_impl(log_level_t::debug, __LINE__, __FILE__,                             \
           lsem::log::details::module_name(), FMT_STRING(Fmt), __VA_ARGS__)
#else
#define log_error(Fmt, ...)                                                    \
  log_impl(log_level_t::error, __LINE__, __FILE__,                             \
           lsem::log::details::module_name(),                                  \
           FMT_STRING(Fmt) __VA_OPT__(, ) __VA_ARGS__)
#define log_warning(Fmt, ...)                                                  \
  log_impl(log_level_t::warning, __LINE__, __FILE__,                           \
           lsem::log::details::module_name(),                                  \
           FMT_STRING(Fmt) __VA_OPT__(, ) __VA_ARGS__)
#define log_info(Fmt, ...)                                                     \
  log_impl(log_level_t::info, __LINE__, __FILE__,                              \
           lsem::log::details::module_name(),                                  \
           FMT_STRING(Fmt) __VA_OPT__(, ) __VA_ARGS__)
#define log_debug(Fmt, ...)                                                    \
  log_impl(log_level_t::debug, __LINE__, __FILE__,                             \
           lsem::log::details::module_name(),                                  \
           FMT_STRING(Fmt) __VA_OPT__(, ) __VA_ARGS__)
#endif

#define LOG_MODULE_NAME(Name)                                                  \
  namespace lsem::log::details {                                               \
  namespace {                                                                  \
  const char *module_name() { return Name; }                                   \
  }                                                                            \
  }
