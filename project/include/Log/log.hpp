// include/Log/log.hpp
#pragma once

#include <fmt/color.h>
#include <fmt/core.h>
#include <source_location>
#include <string>

namespace detail {

template <typename... Args>
static auto format_msg(const std::source_location &loc,
                       std::string_view fmt_str, Args &&...args)
    -> std::string {
  std::string user_msg =
      fmt::format(fmt::runtime(fmt_str), std::forward<Args>(args)...);

  return fmt::format(fmt::fg(fmt::color::alice_blue), "{} (at {}:{} in {})", user_msg, loc.file_name(),
                     loc.line(), loc.function_name());
}

class CompileError : public std::runtime_error {
public:
  explicit CompileError(const std::string &message)
      : std::runtime_error(message) {}
};

} // namespace detail

class Log {
public:
  template <typename... Args>
  static auto
  panic(std::string_view fmt_str, Args &&...args,
        const std::source_location &loc = std::source_location::current())
      -> void {
    fmt::print(stderr, fmt::emphasis::bold | fmt::fg(fmt::color::red),
               "[PANIC] ");
    std::string msg =
        fmt::format(fmt::runtime(fmt_str), std::forward<Args>(args)...);
    fmt::println(stderr, "{}", msg);
    fmt::print(stderr, fmt::fg(fmt::color::slate_gray), " --> {}:{}:{}\n",
               loc.file_name(), loc.line(), loc.function_name());

    throw detail::CompileError(
        detail::format_msg(loc, fmt_str, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static auto
  trace(std::string_view fmt_str, Args &&...args,
        const std::source_location &loc = std::source_location::current())
      -> void {
    fmt::print(stdout, fmt::fg(fmt::color::cyan), "[TRACE] ");
    fmt::print(stdout, "{} ",
               fmt::format(fmt::runtime(fmt_str), std::forward<Args>(args)...));
    fmt::print(stdout, fmt::fg(fmt::color::dark_violet), "[{}]\n",
               loc.function_name());
  }
};