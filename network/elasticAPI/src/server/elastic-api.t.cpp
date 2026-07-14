/*!
 * ============================================================================
 * \file
 * - Program:       elasticAPI
 * - File:          elastic-api.t.cpp
 * - Created:       07/14/2026
 * - Description:   Интеграционные тесты HTTP Document API.
 *
 * ----------------------------------------------------------------------------
 *
 * - History:
 *
 * ============================================================================
 */
//-------------------------------------------------------------------------//
#include "../server.h"
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
//-------------------------------------------------------------------------//
#include "service/structo-search.hpp"
#include "src/http/http-req.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  using namespace std::chrono_literals;
//-------------------------------------------------------------------------//
  //!< Результат выполнения HTTP-запроса.
  struct http_response
  {
    elastic::http::status_codes status_code = elastic::http::status_codes::OK;

    std::string status_line;
    std::string headers;
    std::string body;
  };

  /**
   * RAII-обёртка над POSIX socket descriptor.
   */
  class socket_handle final
  {
    int descriptor = -1;

  public:
    explicit socket_handle(int descriptor = -1) noexcept : descriptor(descriptor)
    {
    }

    ~socket_handle()
    {
        reset();
    }

    socket_handle(const socket_handle&) = delete;
    socket_handle& operator=(const socket_handle&) = delete;

    socket_handle(socket_handle&& other) noexcept
      : descriptor(std::exchange(other.descriptor, -1))
    {
    }

    auto operator=(socket_handle&& other) noexcept -> socket_handle&
    {
      if (this != &other)
      {
        reset();

        this->descriptor = std::exchange(other.descriptor, -1);
      }

      return *this;
    }

    [[nodiscard]]
    auto get() const noexcept -> int
    {
      return this->descriptor;
    }

    [[nodiscard]]
    auto valid() const noexcept -> bool
    {
      return this->descriptor >= 0;
    }

    void reset(int descriptor = -1) noexcept
    {
      if (this->descriptor >= 0)
      {
        ::close(this->descriptor);
      }

      this->descriptor = descriptor;
    }
  };
//-------------------------------------------------------------------------//
  /**
   * Возвращает временно свободный TCP-порт.
   *
   * Важно: между освобождением порта и запуском сервера теоретически остаётся
   * небольшое race window. Для локальных unit/integration-тестов это приемлемо.
   * Production-решение — передавать серверу port=0 и получать реально выбранный
   * порт из listen socket.
   */
  auto find_free_tcp_port() -> std::uint16_t
  {
    socket_handle socket(::socket(AF_INET, SOCK_STREAM, 0));
    if (not socket.valid())
    {
      throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(0);

    if (::bind(socket.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0)
    {
      throw std::runtime_error("bind() failed: " + std::string(std::strerror(errno)));
    }

    socklen_t address_size = sizeof(address);

    if (::getsockname(socket.get(), reinterpret_cast<sockaddr*>(&address), &address_size) != 0)
    {
      throw std::runtime_error("getsockname() failed: " + std::string(std::strerror(errno)));
    }

    return ntohs(address.sin_port);
  }

  //!< Connects to server.
  auto connect_to_server(std::uint16_t port) -> socket_handle
  {
    socket_handle socket(::socket(AF_INET, SOCK_STREAM, 0));
    if (not socket.valid())
    {
      throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (::inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
    {
      throw std::runtime_error("inet_pton() failed");
    }

    if (::connect(socket.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0)
    {
      throw std::runtime_error("connect() failed: " + std::string(std::strerror(errno)));
    }

    return socket;
  }

  //!< Отправляет весь буфер с обработкой частичных send().
  void send_all(int descriptor, std::string_view data)
  {
    std::size_t offset = 0;
    while (offset < data.size())
    {
      const auto result = ::send(descriptor, data.data() + offset, data.size() - offset, MSG_NOSIGNAL);
      if (result < 0)
      {
        if (errno == EINTR)
        {
          continue;
        }

        throw std::runtime_error("send() failed: " + std::string(std::strerror(errno)));
      }

      if (result == 0)
      {
        throw std::runtime_error("send() returned zero");
      }

      offset += static_cast<std::size_t>(result);
    }
  }

  /**
   * Читает HTTP-ответ до закрытия соединения.
   *
   * В запросе передаётся Connection: close, поэтому сервер должен закрыть
   * соединение после отправки ответа.
   */
  auto receive_all(int descriptor) -> std::string
  {
    std::string result;
    result.reserve(4096);

    char buffer[8192];

    while (true)
    {
      const auto received = ::recv(descriptor, buffer, sizeof(buffer), 0);
      if (received > 0)
      {
        result.append(buffer, static_cast<std::size_t>(received));
        continue;
      }

      if (received == 0)
      {
        break;
      }

      if (errno == EINTR)
      {
        continue;
      }

      throw std::runtime_error("recv() failed: " + std::string(std::strerror(errno)));
    }

    return result;
  }

  /**
   * Разбирает минимально необходимую часть HTTP/1.1 ответа.
   */
  auto parse_http_response(std::string raw_response) -> http_response
  {
    const auto status_end = raw_response.find("\r\n");
    if (status_end == std::string::npos)
    {
      throw std::runtime_error("HTTP response has no status line");
    }

    const auto headers_end = raw_response.find("\r\n\r\n");
    if (headers_end == std::string::npos)
    {
      throw std::runtime_error("HTTP response has no header terminator");
    }

    http_response response;
    response.status_line = raw_response.substr(0, status_end);
    response.headers = raw_response.substr(status_end + 2, headers_end - status_end - 2);
    response.body = raw_response.substr(headers_end + 4);

    const auto first_space = response.status_line.find(' ');
    if (first_space == std::string::npos)
    {
      throw std::runtime_error("Invalid HTTP status line");
    }

    const auto second_space = response.status_line.find(' ', first_space + 1);
    const std::string status_code = response.status_line.substr(first_space + 1,
                                                              second_space == std::string::npos? std::string::npos : second_space - first_space - 1);

    response.status_code = static_cast<elastic::http::status_codes>(std::stoi(status_code));

    return response;
  }

  /**
   * Выполняет один HTTP-запрос.
   */
  auto execute_http_request(std::uint16_t port, std::string_view method, std::string_view target, std::string_view body = {}) -> http_response
  {
    auto socket = connect_to_server(port);

    std::string request;
    request.reserve(body.size() + 512);

    request += method;
    request += " ";
    request += target;
    request += " HTTP/1.1\r\n";

    request += "Host: 127.0.0.1:";
    request += std::to_string(port);
    request += "\r\n";

    request += "Accept: application/json\r\n";
    request += "Connection: close\r\n";

    if (not body.empty())
    {
      request += "Content-Type: application/json\r\n";
    }

    request += "Content-Length: ";
    request += std::to_string(body.size());
    request += "\r\n\r\n";

    request.append(body.data(), body.size());

    send_all(socket.get(), request);
    return parse_http_response(receive_all(socket.get()));
  }

  /**
   * Проверяет, что сервер начал принимать TCP-соединения.
   */
  auto wait_until_server_ready(std::uint16_t port, std::chrono::milliseconds timeout) -> bool
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
      try
      {
        auto socket = connect_to_server(port);
        return true;
      }
      catch (const std::exception&)
      {
        std::this_thread::sleep_for(20ms);
      }
    }

    return false;
  }
//-------------------------------------------------------------------------//
  class DocumentApiHttpTest : public ::testing::Test
  {
  protected:
    DocumentApiHttpTest()
      : config{
        {"service", mtc::zmap{
          {"index", mtc::zmap{{"generic_name", "libelasticAPI.so"}}},
          {"contents", "Mini"}}
        },
        {"api", mtc::zmap{
            {"type", "elastic"},
            {"name", "Elastic"},
            {"port", 9200},
            {"module", "libelasticAPI.so"},
            {"workers", 2},
            {"max_body_size", "5M"},
            {"request_timeout", "2s"},
        }}
      },
      port(config.get_section("api").get_int32("port", 9200)),
      service(palmira::CreateStructo(config.get_section("service")))
    {
      mtc::zmap{
            { "limit", mtc::zmap{
              { "context", 5 },
              { "query", mtc::zmap{
                { "fuzzy", mtc::array_zval{"a", "b", "c"}}}}}}
      };
    }

  protected:
    void SetUp() override
    {
      palmira::IServer *srv = nullptr;
      ASSERT_EQ(CreateServer(&srv, this->service, this->config), 0);
      this->server = srv;

      ASSERT_TRUE(this->server != nullptr) << "createServer() returned an empty server";
      ASSERT_NO_THROW(this->server->Start()) << "server Start() failed";

      /*
       * Wait() предполагается блокирующим методом. Запускаем его отдельно,
       * чтобы тестовый поток мог отправлять HTTP-запросы.
       */
      this->wait_thread = std::thread([this]() {
        this->server->Wait();
      });

      ASSERT_TRUE(wait_until_server_ready(this->port, 5s)) << "server did not open port " << this->port;
    }

    void TearDown() override
    {
      if (this->server != nullptr)
      {
        this->server->Stop();
        this->server->Detach();
      }

      if (this->wait_thread.joinable())
      {
        this->wait_thread.join();
      }
    }

    const mtc::config config;
    const uint16_t port = 9200;
    mtc::api<palmira::IService> service;
    mtc::api<palmira::IServer> server;
    std::thread wait_thread;
  };
//-------------------------------------------------------------------------//
  TEST_F(DocumentApiHttpTest, PutGetAndPostDocument)
  {
    // PUT с заданным идентификатором.
    const auto put_response = execute_http_request(this->port, "PUT", "/test-document-api/_doc/1", R"json({"name":"brave","age":42})json");

    EXPECT_TRUE(put_response.status_code == elastic::http::status_codes::OK || put_response.status_code == elastic::http::status_codes::CREATED)
      << put_response.status_line << std::endl
      << put_response.body;
    EXPECT_NE(put_response.body.find("\"_index\""), std::string::npos);
    EXPECT_NE(put_response.body.find("\"_id\""), std::string::npos);

    // GET ранее добавленного документа.
    const auto get_response = execute_http_request(this->port, "GET", "/test-document-api/_doc/1");

    EXPECT_EQ(get_response.status_code, elastic::http::status_codes::OK)
      << get_response.status_line
      << std::endl
      << get_response.body;
    EXPECT_NE(get_response.body.find("\"found\": true"), std::string::npos);
    // EXPECT_NE(get_response.body.find("\"name\":\"brave\""), std::string::npos);

    // POST без идентификатора. Сервер должен сгенерировать _id.
    const auto post_response = execute_http_request(this->port, "POST", "/test-document-api/_doc", R"json({"name":"generated-id-document"})json");

    EXPECT_TRUE(post_response.status_code == elastic::http::status_codes::OK || post_response.status_code == elastic::http::status_codes::CREATED)
      << post_response.status_line
      << std::endl
      << post_response.body;
    EXPECT_NE(post_response.body.find("\"_id\""), std::string::npos);
    EXPECT_NE(post_response.body.find("\"result\""), std::string::npos);
  }

  TEST_F(DocumentApiHttpTest, RejectInvalidJsonDocument)
  {
      const auto response = execute_http_request(this->port, "PUT", "/test-document-api/_doc/invalid-json", R"json({"name":)json");

      EXPECT_EQ(response.status_code, elastic::http::status_codes::BAD_REQUEST)
        << response.status_line
        << std::endl
        << response.body;
      EXPECT_NE(response.body.find("\"error\""), std::string::npos);
  }
//-------------------------------------------------------------------------//
} // namespace