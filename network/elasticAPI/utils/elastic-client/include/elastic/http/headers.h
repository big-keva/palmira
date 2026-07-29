#pragma once
//-------------------------------------------------------------------------//
#include <locale>
#include <string_view>
#include <vector>
#include <algorithm>
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  namespace detail
  {
    [[nodiscard]]
    inline bool iequals(std::string_view lhs, std::string_view rhs) noexcept
    {
      if (lhs.size() != rhs.size())
      {
        return false;
      }

      for (std::size_t index = 0; index < lhs.size(); ++index)
      {
        const auto left = static_cast<unsigned char>(lhs[index]);
        const auto right = static_cast<unsigned char>(rhs[index]);

        if (std::tolower(left) != std::tolower(right))
        {
          return false;
        }
      }

      return true;
    }
  } // namespace detail
//-------------------------------------------------------------------------//
  struct header
  {
    std::string_view name;
    std::string_view value;
  };
//-------------------------------------------------------------------------//
  template<typename header_t>
  class headers
  {
    using value_type = header_t;

    //!< Keeps a list of headers.
    std::vector<value_type> header_types;

  public:
    using iterator = std::vector<value_type>::iterator;
    using const_iterator = std::vector<value_type>::const_iterator;

  public:
    void clear() noexcept
    {
      this->header_types.clear();
    }

    void reserve(std::size_t count)
    {
      this->header_types.reserve(count);
    }

    void add(std::string_view name, std::string_view value)
    {
      this->header_types.emplace_back(value_type{name, value});
    }

    void set(std::string_view name, std::string_view value)
    {
      this->remove(name);
      this->add(name, value);
    }

    bool remove(std::string_view name)
    {
      const auto old_size = this->header_types.size();

      std::erase_if(this->header_types, [name](const value_type &item) -> bool {
        return detail::iequals(item.name, name);
      });

      return old_size != this->header_types.size();
    }

    [[nodiscard]]
    bool contains(std::string_view name) const noexcept
    {
      return std::any_of(this->header_types.begin(), this->header_types.end(), [name](const value_type &item) -> bool {
        return detail::iequals(item.name, name);
      });
    }

    [[nodiscard]]
    std::string_view get(std::string_view name) const noexcept
    {
      const auto iterator = std::find_if(this->header_types.begin(), this->header_types.end(), [name](const value_type &item)-> bool {
        return detail::iequals(item.name, name);
      });

      if (iterator == this->header_types.end())
      {
        return {};
      }

      return iterator->value;
    }

    [[nodiscard]]
    iterator begin() noexcept
    {
      return this->header_types.begin();
    }

    [[nodiscard]]
    iterator end() noexcept
    {
      return this->header_types.end();
    }

    [[nodiscard]]
    const_iterator begin() const noexcept
    {
      return this->header_types.begin();
    }

    [[nodiscard]]
    const_iterator end() const noexcept
    {
      return this->header_types.end();
    }

    [[nodiscard]]
    const_iterator cbegin() const noexcept
    {
      return this->header_types.cbegin();
    }

    [[nodiscard]]
    const_iterator cend() const noexcept
    {
      return this->header_types.cend();
    }

    [[nodiscard]]
    std::size_t size() const noexcept
    {
      return this->header_types.size();
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
      return this->header_types.empty();
    }
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http