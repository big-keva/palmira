#pragma once
//-------------------------------------------------------------------------//
#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "elastic/http/headers.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  namespace detail
  {
//-------------------------------------------------------------------------//
    struct response_test_access;
//-------------------------------------------------------------------------//
  }
//-------------------------------------------------------------------------//
  struct response_header
  {
    std::string_view name;
    std::string_view value;
  };

  using response_headers_type_t = http::headers<response_header>;
//-------------------------------------------------------------------------//
  class response
  {
    friend class session;
    friend struct detail::response_test_access;

    //!< Keeps a response status.
    long response_status = 0;

    //!< Keeps a response headers.
    mutable response_headers_type_t response_headers;

    std::vector<char> raw_headers;
    std::vector<char> raw_body;

    mutable bool headers_parsed = false;
    mutable simdjson::padded_string padded_body;

    inline thread_local static simdjson::ondemand::parser parser{};

  public:
    response(const response&) = delete;
    auto operator=(const response&) -> response& = delete;

  public:
    response() = default;
    response(response &&) noexcept = default;
    auto operator=(response &&) noexcept -> response& = default;

    /**
     * Destructor.
     */
    ~response() = default;

    //!< Clears a response.
    auto clear() noexcept -> void;

    [[nodiscard]] auto headers() const -> const response_headers_type_t &;
    [[nodiscard]] auto body() const noexcept -> std::string_view;
    [[nodiscard]] auto get_status() const noexcept -> std::uint16_t;

    [[nodiscard]] auto informational() const noexcept -> bool;
    [[nodiscard]] auto ok() const noexcept -> bool;
    [[nodiscard]] auto redirect() const noexcept -> bool;
    [[nodiscard]] auto client_error() const noexcept -> bool;
    [[nodiscard]] auto server_error() const noexcept -> bool;

    [[nodiscard]] auto has_header(std::string_view name) const -> bool;
    [[nodiscard]] auto header(std::string_view name) const -> std::string_view;

    [[nodiscard]] auto is_json() const -> bool;
    [[nodiscard]] auto is_chunked() const -> bool;
    [[nodiscard]] auto keep_alive() const -> bool;

    template<class onjson_t>
    auto parse_json(onjson_t &&onjson) const -> void;

  private:
    auto parse_headers() const -> void;
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http