#include "../utils.h"
//-------------------------------------------------------------------------//
#include <chrono>
#include <random>
#include <stdexcept>
#include <thread>
//-------------------------------------------------------------------------//
#include <moonycode/codes.h>
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    template <class chartype>
    auto as_ulong(const chartype *str, size_t len, const std::initializer_list<std::pair<const char *, double>> &units) -> std::uint64_t
    {
      auto val = static_cast<std::uint64_t>(0);
      auto end = str;
      auto cmp = [&](const char *match) -> bool
      {
        auto src = str;
        while (src != end && *match != '\0' && (typename std::make_unsigned<chartype>::type) * src == static_cast<std::uint8_t>(*match))
        {
          ++src, ++match;
        }
        return src == end && *match == '\0';
      };

      if (len != UINT64_MAX)
      {
        end += len;
      }
      else
      {
        while (end != nullptr && *end != 0)
        {
          ++end;
        }
      }

      while (str != end && *str >= '0' && *str <= '9')
      {
        val = val * 10 + *str++ - '0';
      }

      for (auto &it : units)
      {
        if (cmp(it.first))
        {
          return size_t(val * it.second);
        }
      }
      return UINT64_MAX;
    }

    template <class chartype>
    auto as_ulong(const chartype *str, const std::initializer_list<std::pair<const char *, double>> &units) -> std::uint64_t
    {
      return as_ulong(str, UINT64_MAX, units);
    }

    template <class chartype>
    auto as_long(chartype *str, size_t len, const std::initializer_list<std::pair<const char *, double>> &units) -> long
    {
      auto end = str;
      if (len != static_cast<size_t>(-1))
      {
        end += len;
      }
      else
      {
        while (end != nullptr && *end != 0)
        {
          ++end;
        }
      }
      if (str != end)
      {
        auto neg = *str == '-';
        auto val = as_ulong(str + (neg ? 1 : 0), end, units);
        if (val != UINT64_MAX)
        {
          return (neg ? -1 : 1) * val;
        }
      }
      return -1;
    }

    template <class chartype>
    auto as_long(chartype *str, const std::initializer_list<std::pair<const char *, double>> &units) -> long
    {
      return as_long(str, UINT64_MAX, units);
    }

    // timespans parsers
    template <class _Rep, class _Period, class chartype>
    auto as_duration(const std::chrono::duration<_Rep, _Period> &, const chartype *str, size_t len = static_cast<size_t>(-1)) -> std::chrono::duration<_Rep, _Period>;

    template <class chartype>
    auto as_duration(const std::chrono::milliseconds &, const chartype *str, size_t len = static_cast<size_t>(-1)) -> std::chrono::milliseconds
    {
      return std::chrono::milliseconds(as_ulong(str, len, {
        {"H", 60 * 60 * 1000},
        {"h", 60 * 60 * 1000},
        {"M", 60 * 1000},
        {"m", 60 * 1000},
        {"S", 1000},
        {"s", 1000},
        {"MS", 1},
        {"Ms", 1},
        {"ms", 1},
        {"NS", 1e-6},
        {"Ns", 1e-6},
        {"ns", 1e-6}
      }));
    }

    template <class _Rep, class _Period, class chartype>
    auto as_duration(const std::chrono::duration<_Rep, _Period> &, const chartype *str, size_t len) -> std::chrono::duration<_Rep, _Period>
    {
      return std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(as_duration(std::chrono::milliseconds(), str, len));
    }

    template <class _Rep, class _Period, class chartype>
    auto as_duration(const std::chrono::duration<_Rep, _Period> &, const std::basic_string<chartype> &str) -> std::chrono::duration<_Rep, _Period>
    {
      return std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(as_duration(std::chrono::milliseconds(), str.c_str(), str.length()));
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  auto parse_size(const std::string &size) -> size_t
  {
    if (not size.empty())
    {
      auto val = as_ulong(size.c_str(), {
        {"", 1},
      {"B", 1},
      {"b", 1},
      {"K", 1024},
      {"k", 1024},
      {"Kb", 1024},
      {"KB", 1024},
      {"kb", 1024},
      {"M", 1024 * 1024},
      {"m", 1024 * 1024},
      {"MB", 1024 * 1024},
      {"Mb", 1024 * 1024},
      {"mb", 1024 * 1024},
      {"G", 1024 * 1024 * 1024},
      {"g", 1024 * 1024 * 1024},
      {"GB", 1024 * 1024 * 1024},
      {"Gb", 1024 * 1024 * 1024},
      {"gb", 1024 * 1024 * 1024}
      });

      if (val < UINT64_MAX)
      {
        return val;
      }
    }
    throw(std::invalid_argument("Invalid size '" + size + "'"));
  }

  auto parse_timeout(const std::string &timeout) -> size_t
  {
    return static_cast<size_t>(as_duration(std::chrono::milliseconds(), timeout.c_str()).count());
  }

  auto make_uid(std::uint8_t size /*= 16*/) -> std::string
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 35);

    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    std::string id;
    id.reserve(2 * size);

    auto time = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string time_str = std::to_string(time);
    for (char c : time_str)
    {
      id.push_back(charset[(c - '0') % 36]);
    }

    // 8 случайных символов
    for (auto i = 0; i < size; ++i)
    {
      id.push_back(charset[dis(gen)]);
    }

    return id;
  }

  auto to_utf8(const codepages::widestring &src) -> std::string
  {
    return codepages::widetombcs(codepages::codepage_utf8, src);
  }
//-------------------------------------------------------------------------//
} // namespace elastic
