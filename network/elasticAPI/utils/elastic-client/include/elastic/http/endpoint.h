#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  struct parameter
  {
    std::string name;
    std::string value;
  };
//-------------------------------------------------------------------------//
  class endpoint
  {
    //!< Keeps a list of segments.
    std::vector<std::string> segments;

    //!< Keeps a list of query parameters.
    std::vector<parameter> query_params;

  public:
    endpoint() = default;

    auto clear() noexcept -> endpoint&;
    auto reserve(std::size_t segment_count, std::size_t query_count = 0) -> endpoint&;

    // Добавляет ровно один сегмент. Символ '/' внутри значения кодируется как %2F.
    auto append(std::string_view segment) -> endpoint&;

    // Добавляет путь из нескольких сегментов, разделённых '/'.
    auto path(std::string_view path) -> endpoint&;

    auto remove_last() noexcept -> endpoint&;
    auto replace_last(std::string_view segment) -> endpoint&;

    auto query(std::string_view name, std::string_view value) -> endpoint&;
    auto query_if(bool condition, std::string_view name, std::string_view value) -> endpoint&;

    [[nodiscard]]
    auto empty() const noexcept -> bool;

    [[nodiscard]]
    auto segment_count() const noexcept -> std::size_t;

    [[nodiscard]]
    auto query_count() const noexcept -> std::size_t;

    [[nodiscard]]
    auto build() const -> std::string;

  private:
    [[nodiscard]]
    static auto encode_component(std::string_view value) -> std::string;
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http
