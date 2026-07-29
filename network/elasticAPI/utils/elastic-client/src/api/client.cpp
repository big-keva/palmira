#include "elastic/api/client.h"
//-------------------------------------------------------------------------//
namespace elastic::api
{
//-------------------------------------------------------------------------//
  client::client(std::string base_url) : http_client(std::move(base_url))
  {
  }

  auto client::execute(const http::request &request) -> http::response
  {
    return this->http_client.execute(request);
  }

  auto client::base_url(std::string value) -> client&
  {
    return this->http_client.base_url(std::move(value)), *this;
  }

  auto client::base_url() const -> std::string
  {
    return this->http_client.base_url();
  }
//-------------------------------------------------------------------------//
} // namespace elastic::api
