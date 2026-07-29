#pragma once
//-------------------------------------------------------------------------//
#include <string>
//-------------------------------------------------------------------------//
#include <curl/curl.h>
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  class request;
  class response;
//-------------------------------------------------------------------------//
  class session
  {
    //!< Keeps a CURL handle.
    CURL *handle = nullptr;

    //!< Keeps a list of headers.
    curl_slist *request_headers = nullptr;

    //!< Keeps a base url.
    std::string url;

  public:
    /**
     * Constructor.
     * @param base_url [in] - A base URL.
     */
    explicit session(std::string base_url = {});

    /**
     * Destructor.
     */
    ~session();

    /**
     * Performs a request.
     * @param req [in] - A request.
     * @param resp [out] - A response.
     */
    auto perform(const request &req, response &resp) -> void;

    /**
     * Sets a base URL.
     * @param value [in] - A base URL.
     * @return
     */
    auto base_url(std::string value) -> session &;

    [[nodiscard]]
    auto base_url() const noexcept -> std::string_view;

  public:
    session(const session&) = delete;
    auto operator=(const session&) -> session& = delete;

    session(session&&) = delete;
    auto operator=(session&&) -> session& = delete;

  private:
    auto configure() -> void;
    auto clear_headers() noexcept -> void;
    auto reset_request() -> void;
    auto prepare_headers(const request& req) -> void;
    auto prepare_method(const request& req) -> void;
    auto prepare_get(const request& req) -> void;
    auto prepare_post(const request& req) -> void;
    auto prepare_put(const request& req) -> void;
    auto prepare_delete(const request& req) -> void;
    auto prepare_head(const request& req) -> void;
    auto prepare_body(const request& req) -> void;

    [[nodiscard]]
    auto build_url(const request& req) const -> std::string;

  private:
    static auto onbody(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) -> std::size_t;
    static auto onheader(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) -> std::size_t;
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http