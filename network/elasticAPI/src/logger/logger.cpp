#include "logger.h"
//-------------------------------------------------------------------------//
#include <vector>
#include <sstream>
#include <cstdio>
#include <cstdarg>
//-------------------------------------------------------------------------//
namespace elastic::logger
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    // Keeps a debug level.
    severities g_debug_level = severities::trace;
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  auto to_string_view(severities severity) -> std::string_view
  {
    switch (severity)
    {
      case severities::error: return "ERROR";
      case severities::warning: return "WARNING";
      case severities::info: return "INFO";
      case severities::debug: return "DEBUG";
      case severities::trace: return "TRACE";
      default: return "NONE";
    }
  }

  auto severity_from(std::string_view severity) -> severities
  {
    if (severity == "error" || severity == "ERROR")
    {
      return severities::error;
    }
    else if (severity == "warning" || severity == "WARNING")
    {
      return severities::warning;
    }
    else if (severity == "info" || severity == "INFO")
    {
      return severities::info;
    }
    else if (severity == "debug" || severity == "DEBUG")
    {
      return severities::debug;
    }
    else if (severity == "trace" || severity == "TRACE")
    {
      return severities::trace;
    }
    else if (severity == "none" || severity == "NONE")
    {
      return severities::none;
    }
    return severities::info;
  }
//-------------------------------------------------------------------------//
  auto init_logger(const std::string &fname) -> void
  {
  }

  auto destroy_logger() -> void
  {
  }

  auto set_debug_level(severities debug_level) -> void
  {
    g_debug_level = debug_level;
  }

  auto set_debug_level(const char *debug_level) -> void
  {
    g_debug_level = severity_from(debug_level);
  }

  auto get_debug_level() -> severities
  {
    return g_debug_level;
  }
//-------------------------------------------------------------------------//
  auto to_log(severities level, const char *file, int line, const char *func_name, const char *fmt, ...) -> void
  {
    if (level <= g_debug_level)
    {
      std::va_list arg, arg_copy;
      va_start(arg, fmt);

      // Making a copy.
      va_copy(arg_copy, arg);
      // Parsing a list of arguments.
      const auto size = std::vsnprintf(nullptr, 0, fmt, arg_copy);
      // Closing arg.
      va_end(arg_copy);

      // Allocating memory.
      std::vector<char> buffer(size + 1);
      // Building the string.
      std::vsnprintf(buffer.data(), buffer.size(), fmt, arg);

      // Closing arg.
      va_end(arg);

      std::fprintf(level <= severities::warning ? stderr : stdout,
             "%s:%d [%s] %s %s\n",
                   file, line, func_name, to_string_view(level).data(), buffer.data());
    }
  }
//-------------------------------------------------------------------------//
} // namespace elastic::logger
