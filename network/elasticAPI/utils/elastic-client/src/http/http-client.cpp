#include "elastic/http/client.h"
//-------------------------------------------------------------------------//
#include <stdexcept>
#include <utility>
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  client::client(std::string base_url) : sess(std::move(base_url))
  {
    if (this->sess.base_url().empty())
    {
      throw std::invalid_argument("HTTP client base URL must not be empty");
    }
  }

  client::~client()
  {
    this->sess.shutdown();
  }

  auto client::execute(const request& req) -> response
  {
    response result;
    std::scoped_lock lock(this->mtx);

    this->sess.perform(req, result);
    return result;
  }

  auto client::base_url(std::string value) -> client&
  {
    if (value.empty())
    {
      throw std::invalid_argument("HTTP client base URL must not be empty");
    }

    std::scoped_lock lock(this->mtx);
    this->sess.base_url(std::move(value));

    return *this;
  }

  auto client::base_url() const -> std::string
  {
    std::scoped_lock lock(this->mtx);
    return std::string(this->sess.base_url());
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http
