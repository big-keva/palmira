#include "elastic/http/session.h"
//-------------------------------------------------------------------------//
#include <stdexcept>
//-------------------------------------------------------------------------//
#include "elastic/http/request.h"
#include "elastic/http/response.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    struct curl_global_guard
    {
      curl_global_guard()
      {
        const CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (result != CURLE_OK)
        {
            throw std::runtime_error("curl_global_init failed");
        }
      }

      ~curl_global_guard()
      {
        curl_global_cleanup();
      }

      curl_global_guard(const curl_global_guard&) = delete;
      auto operator=(const curl_global_guard&) -> curl_global_guard& = delete;
    };

    auto ensure_curl_initialized() -> void
    {
      static curl_global_guard guard;
      (void)guard;
    }

    auto throw_on_curl_error(CURLcode code, std::string_view operation) -> void
    {
      if (code == CURLE_OK)
      {
        return;
      }

      std::string message;
      message.reserve(operation.size() + 2U + 64U);
      message.append(operation);
      message.append(": ");
      message.append(curl_easy_strerror(code));

      throw std::runtime_error(message);
    }

    template<typename value_t>
    auto set_option(CURL* handle, CURLoption option, value_t value, std::string_view operation) -> void
    {
      throw_on_curl_error(curl_easy_setopt(handle, option, value), operation);
    }
//-------------------------------------------------------------------------//
  } // namespace

  session::session(std::string base_url) : url(std::move(base_url))
  {
    ensure_curl_initialized();

    this->handle = curl_easy_init();
    if (this->handle == nullptr)
    {
      throw std::runtime_error("curl_easy_init failed");
    }

    try
    {
      this->configure();
    }
    catch (...)
    {
      curl_easy_cleanup(this->handle);
      this->handle = nullptr;
      throw;
    }
  }

  session::~session()
  {
    this->shutdown();
  }

  auto session::base_url(std::string value) -> session &
  {
    while (value.size() > 1U && value.back() == '/')
    {
      value.pop_back();
    }

    return this->url = std::move(value), *this;
  }

  auto session::base_url() const noexcept -> std::string_view
  {
    return this->url;
  }

  auto session::configure() -> void
  {
    set_option(this->handle, CURLOPT_WRITEFUNCTION, &session::onbody, "CURLOPT_WRITEFUNCTION");
    set_option(this->handle, CURLOPT_HEADERFUNCTION, &session::onheader, "CURLOPT_HEADERFUNCTION");
    set_option(this->handle, CURLOPT_NOSIGNAL, 1L, "CURLOPT_NOSIGNAL");
    set_option(this->handle, CURLOPT_TCP_KEEPALIVE, 1L, "CURLOPT_TCP_KEEPALIVE");
    set_option(this->handle, CURLOPT_FOLLOWLOCATION, 0L, "CURLOPT_FOLLOWLOCATION");
    set_option(this->handle, CURLOPT_ACCEPT_ENCODING, "", "CURLOPT_ACCEPT_ENCODING");
  }

  auto session::perform(const request& req, response& resp) -> void
  {
    resp.clear();
    this->reset_request();

    const std::string url = build_url(req);
    const auto timeout = req.timeout().count();
    if (timeout > std::numeric_limits<long>::max())
    {
        throw std::overflow_error("HTTP request timeout exceeds libcurl range");
    }

    set_option(this->handle, CURLOPT_URL, url.c_str(), "CURLOPT_URL");
    set_option(this->handle, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout), "CURLOPT_TIMEOUT_MS");
    set_option(this->handle, CURLOPT_WRITEDATA, &resp, "CURLOPT_WRITEDATA");
    set_option(this->handle, CURLOPT_HEADERDATA, &resp, "CURLOPT_HEADERDATA");

    prepare_headers(req);
    prepare_method(req);

    const CURLcode result = curl_easy_perform(this->handle);
    if (result != CURLE_OK)
    {
      throw_on_curl_error(result, "curl_easy_perform");
    }

    throw_on_curl_error(curl_easy_getinfo(this->handle, CURLINFO_RESPONSE_CODE, &resp.response_status), "CURLINFO_RESPONSE_CODE");

    resp.parse_headers();
    resp.headers_parsed = true;
  }

  auto session::shutdown() -> void
  {
    this->clear_headers();

    if (this->handle != nullptr)
    {
      curl_easy_cleanup(this->handle);
      this->handle = nullptr;
    }
  }

  auto session::clear_headers() noexcept -> void
  {
    if (this->request_headers != nullptr)
    {
      curl_slist_free_all(this->request_headers);
      this->request_headers = nullptr;
    }
  }

  auto session::reset_request() -> void
  {
    this->clear_headers();

    set_option(this->handle, CURLOPT_HTTPGET, 0L, "reset CURLOPT_HTTPGET");
    set_option(this->handle, CURLOPT_POST, 0L, "reset CURLOPT_POST");
    set_option(this->handle, CURLOPT_UPLOAD, 0L, "reset CURLOPT_UPLOAD");
    set_option(this->handle, CURLOPT_NOBODY, 0L, "reset CURLOPT_NOBODY");

    set_option(this->handle, CURLOPT_CUSTOMREQUEST, static_cast<const char *>(nullptr), "reset CURLOPT_CUSTOMREQUEST");
    set_option(this->handle, CURLOPT_POSTFIELDS, static_cast<const char *>(nullptr), "reset CURLOPT_POSTFIELDS");
    set_option(this->handle, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(0), "reset CURLOPT_POSTFIELDSIZE_LARGE");
    set_option(this->handle, CURLOPT_WRITEDATA, static_cast<void *>(nullptr), "reset CURLOPT_WRITEDATA");
    set_option(this->handle, CURLOPT_HEADERDATA, static_cast<void *>(nullptr), "reset CURLOPT_HEADERDATA");
    set_option(this->handle, CURLOPT_HTTPHEADER, static_cast<curl_slist *>(nullptr), "reset CURLOPT_HTTPHEADER");
  }

  auto session::prepare_headers(const request &req) -> void
  {
    for (const auto &header : req.headers())
    {
      std::string line;
      line.reserve(header.name.size() + header.value.size() + 2U);
      line.append(header.name);
      line.append(": ");
      line.append(header.value);

      curl_slist* appended = curl_slist_append(this->request_headers, line.c_str());
      if (appended == nullptr)
      {
        clear_headers();
        throw std::bad_alloc();
      }

      this->request_headers = appended;
    }

    if (this->request_headers != nullptr)
    {
      set_option(this->handle, CURLOPT_HTTPHEADER, this->request_headers, "CURLOPT_HTTPHEADER");
    }
  }

  auto session::prepare_method(const request& req) -> void
  {
      switch (req.method())
      {
        case method_types::get: return this->prepare_get(req);
        case method_types::post: return this->prepare_post(req);
        case method_types::put: return this->prepare_put(req);
        case method_types::del: return this->prepare_delete(req);
        case method_types::head: return this->prepare_head(req);
      }

      throw std::logic_error("unsupported HTTP method");
  }

  auto session::prepare_get(const request& req) -> void
  {
    if (req.has_body())
    {
      this->prepare_body(req);
      set_option(this->handle, CURLOPT_CUSTOMREQUEST, "GET", "GET CURLOPT_CUSTOMREQUEST");
      return;
    }

    set_option(this->handle, CURLOPT_HTTPGET, 1L, "CURLOPT_HTTPGET");
  }

  auto session::prepare_post(const request& req) -> void
  {
    set_option(this->handle, CURLOPT_POST, 1L, "CURLOPT_POST");
    this->prepare_body(req);
  }

  auto session::prepare_put(const request& req) -> void
  {
    this->prepare_body(req);
    set_option(this->handle, CURLOPT_CUSTOMREQUEST, "PUT", "PUT CURLOPT_CUSTOMREQUEST");
  }

  auto session::prepare_delete(const request& req) -> void
  {
    if (req.has_body())
    {
      this->prepare_body(req);
    }

    set_option(this->handle, CURLOPT_CUSTOMREQUEST, "DELETE", "DELETE CURLOPT_CUSTOMREQUEST");
  }

  auto session::prepare_head(const request&) -> void
  {
    set_option(this->handle, CURLOPT_NOBODY, 1L, "CURLOPT_NOBODY");
    set_option(this->handle, CURLOPT_CUSTOMREQUEST, "HEAD", "HEAD CURLOPT_CUSTOMREQUEST");
  }

  auto session::prepare_body(const request& req) -> void
  {
    const auto body_size = req.body().size();
    if (body_size > static_cast<std::size_t>(std::numeric_limits<curl_off_t>::max()))
    {
      throw std::overflow_error("HTTP request body exceeds libcurl range");
    }

    set_option(this->handle, CURLOPT_POSTFIELDS, req.body().data(), "CURLOPT_POSTFIELDS");
    set_option(this->handle, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body_size), "CURLOPT_POSTFIELDSIZE_LARGE");
  }

  auto session::build_url(const request& req) const -> std::string
  {
    std::string target = req.target();
    if (this->url.empty())
    {
      return target;
    }

    std::string result;
    result.reserve(this->url.size() + target.size() + 1U);
    result.append(this->url);

    if (not target.empty() && target.front() != '/')
    {
      result.push_back('/');
    }

    result.append(target);
    return result;
  }
//-------------------------------------------------------------------------//
  auto session::onbody(char *ptr, std::size_t size, std::size_t nmemb, void *userdata) -> std::size_t
  {
    if (userdata == nullptr)
    {
      return 0U;
    }

    if (size != 0U && nmemb > std::numeric_limits<std::size_t>::max() / size)
    {
      return 0U;
    }

    const std::size_t bytes = size * nmemb;
    auto *resp = static_cast<response *>(userdata);
    resp->raw_body.insert(resp->raw_body.end(), ptr, ptr + bytes);
    return bytes;
  }

  auto session::onheader(char *ptr, std::size_t size, std::size_t nmemb, void *userdata) -> std::size_t
  {
    if (userdata == nullptr)
    {
      return 0U;
    }

    if (size != 0U && nmemb > std::numeric_limits<std::size_t>::max() / size)
    {
      return 0U;
    }

    const std::size_t bytes = size * nmemb;
    auto *resp = static_cast<response *>(userdata);
    resp->raw_headers.insert(resp->raw_headers.end(), ptr, ptr + bytes);
    return bytes;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http
