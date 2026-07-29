#pragma once
//-------------------------------------------------------------------------//
#include <mutex>
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include "elastic/http/request.h"
#include "elastic/http/response.h"
#include "elastic/http/session.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  class client
  {
    //!< Keeps a session.
    session sess;

    //!< Keeps a mutex.
    mutable std::mutex mtx;

  public:
    client(const client&) = delete;
    auto operator=(const client&) -> client& = delete;

    client(client&&) = delete;
    auto operator=(client&&) -> client& = delete;

  public:
    /**
     * Constructor.
     * @param base_url [in] - A base URL.
     */
    explicit client(std::string base_url);
    ~client() = default;

    [[nodiscard]]
    auto execute(const request& req) -> response;

    auto base_url(std::string value) -> client&;

    [[nodiscard]]
    auto base_url() const -> std::string;
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http
