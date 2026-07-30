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
#include <cerrno>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <filesystem>
#include <thread>
#include <utility>
//-------------------------------------------------------------------------//
#include <service/structo-search.hpp>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "elastic/api/client.h"
#include "elastic/api/docapi/index_document_request.h"
#include "elastic/api/docapi/get_document_request.h"
#include "elastic/api/docapi/delete_document_request.h"
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  using namespace std::chrono_literals;
//-------------------------------------------------------------------------//
  constexpr std::uint16_t g_server_port = 9292;
  constexpr std::string_view g_document_index = "test-document-api";
  constexpr std::string_view g_document_body{R"({"title":"Integration test document","revision":1,"active":true})"};
//-------------------------------------------------------------------------//
  class DocumentApiHttpTest final
  {
  protected:
    const mtc::config config;
    const uint16_t port = g_server_port;
    mtc::api<palmira::IService> service;
    mtc::api<palmira::IServer> server;
    std::thread wait_thread;

  public:
    DocumentApiHttpTest()
      : config{
        {"service", mtc::zmap{
          {"index", mtc::zmap{{"generic_name", "test-elastic-api"}}},
          {"contents", "Mini"}}
          },
        {"config", mtc::zmap{
          {"type", "elastic"},
          {"name", "Elastic"},
          {"port", g_server_port},
          {"workers", 2},
          {"max_body_size", "5M"},
          {"request_timeout", "2s"}
        }},
      },
      port(config.get_section("config").get_int32("port", g_server_port)),
      service(palmira::CreateStructo(config.get_section("service")))
    {
    }

    ~DocumentApiHttpTest()
    {
      this->stop();
    }

    auto start() -> void
    {
      palmira::IServer *srv = nullptr;
      CreateServer(&srv, this->service, this->config.get_section("config"));
      this->server = srv;

      this->server->Start();

      this->wait_thread = std::thread([this]() {
        this->server->Wait();
      });

      std::this_thread::sleep_for(std::chrono::milliseconds{1000});
    }

    auto stop() -> void
    {
      if (this->server != nullptr)
      {
        this->server->Stop();
        this->server->Wait();
      }

      if (this->wait_thread.joinable())
      {
        this->wait_thread.join();
      }

      std::filesystem::remove(this->config.get_section("config").get_section("index").get_path("generic_name"));
    }
  };
//-------------------------------------------------------------------------//
  class DocumentClientTest : public ::testing::Test
  {
  protected:
    DocumentApiHttpTest server;
    std::unique_ptr<elastic::api::client> client;
    const std::string index;

  protected:
    DocumentClientTest() : index(g_document_index)
    {
    }

    auto SetUp() -> void override
    {
      // Starting a server.
      this->server.start();

      const std::string url("http://127.0.0.1:" + std::to_string(g_server_port));
      this->client = std::make_unique<elastic::api::client>(std::string("http://127.0.0.1:" + std::to_string(g_server_port)));
    }

    auto TearDown() -> void override
    {
      // Closing a client.
      this->client.reset();

      // Stopping the server.
      this->server.stop();
    }

    static auto get_rand_document_id() -> std::string
    {
      static std::atomic_uint64_t sequence{0};

      const auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      return "elastic-client-it-" + std::to_string(timestamp) + "-" + std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
    }
  };
//-------------------------------------------------------------------------//
  TEST_F(DocumentClientTest, PutAndGetDocument)
  {
    {
      const auto document_id = get_rand_document_id();
      auto index_resp = this->client->index(this->index, document_id)
                                                            .body(R"json({"name":"brave","age":42})json")
                                                            .query("refresh", "wait_for")
                                                            .timeout(10000)
                                                            .execute();
      ASSERT_TRUE(index_resp.ok());
      ASSERT_TRUE(index_resp.is_json());
      index_resp.parse_json([&](simdjson::ondemand::document &doc) {
        EXPECT_EQ(doc["_index"].get_string().value(), this->index);
        EXPECT_EQ(doc["_id"].get_string().value(), document_id);
        EXPECT_EQ(doc["_version"].get_int64().value(), 1);
        EXPECT_EQ(doc["result"].get_string().value(), "created");
      });

      auto get_resp = this->client->get_document(this->index, document_id)
                                                        .timeout(10000)
                                                        .execute();
      ASSERT_TRUE(get_resp.ok());
      ASSERT_TRUE(get_resp.is_json());
      std::fprintf(stdout, "Response: %s\n", std::string(get_resp.body().data(), get_resp.body().length()).c_str());
      get_resp.parse_json([&](simdjson::ondemand::document &doc) {
        EXPECT_EQ(doc["_index"].get_string().value(), this->index);
        EXPECT_EQ(doc["_id"].get_string().value(), document_id);
        EXPECT_EQ(doc["_version"].get_int64().value(), -1);
        EXPECT_TRUE(doc["found"].get_bool().value());
        EXPECT_EQ(doc["_source"]["name"].get_string().value(), "brave");
        EXPECT_EQ(doc["_source"]["age"].get_string().value(), "42");
      });
    }
    {// Simple array JSON document
      const auto document_id = get_rand_document_id();
      auto index_resp = this->client->index(this->index, document_id)
                                                            .body(R"json({"name":"brave","age":42,"array":["item 1","item 2","item 3"]})json")
                                                            .query("refresh", "wait_for")
                                                            .timeout(10000)
                                                            .execute();
      ASSERT_TRUE(index_resp.ok());
      ASSERT_TRUE(index_resp.is_json());
      index_resp.parse_json([&](simdjson::ondemand::document &doc) {
        EXPECT_EQ(doc["_index"].get_string().value(), this->index);
        EXPECT_EQ(doc["_id"].get_string().value(), document_id);
        EXPECT_EQ(doc["_version"].get_int64().value(), 1);
        EXPECT_EQ(doc["result"].get_string().value(), "created");
      });

      auto get_resp = this->client->get_document(this->index, document_id)
                                                        .timeout(10000)
                                                        .execute();
      ASSERT_TRUE(get_resp.ok());
      ASSERT_TRUE(get_resp.is_json());
      get_resp.parse_json([&](simdjson::ondemand::document &doc) {
        EXPECT_EQ(doc["_index"].get_string().value(), this->index);
        EXPECT_EQ(doc["_id"].get_string().value(), document_id);
        EXPECT_EQ(doc["_version"].get_int64().value(), -1);
        EXPECT_TRUE(doc["found"].get_bool().value());
        EXPECT_EQ(doc["_source"]["name"].get_string().value(), "brave");
        EXPECT_EQ(doc["_source"]["age"].get_string().value(), "42");
        //<TODO> EXPECT_EQ(doc["_source"]["age"].get_int64().value(), 42);
      });
    }
  }

  TEST_F(DocumentClientTest, DeleteDocument)
  {
    {// Simple array JSON document
      const auto document_id = get_rand_document_id();
      auto index_resp = this->client->index(this->index, document_id)
                                                            .body(R"json({"name":"brave","age":42,"array":["item 1","item 2","item 3"]})json")
                                                            .query("refresh", "wait_for")
                                                            .timeout(10000)
                                                            .execute();
      ASSERT_TRUE(index_resp.ok());
      ASSERT_TRUE(index_resp.is_json());
      index_resp.parse_json([&](simdjson::ondemand::document &doc) {
        EXPECT_EQ(doc["_index"].get_string().value(), this->index);
        EXPECT_EQ(doc["_id"].get_string().value(), document_id);
        EXPECT_EQ(doc["_version"].get_int64().value(), 1);
        EXPECT_EQ(doc["result"].get_string().value(), "created");
      });

      auto del_resp = this->client->delete_document(this->index, document_id)
                                                          .timeout(10000)
                                                          .execute();
      ASSERT_TRUE(del_resp.ok());
      ASSERT_TRUE(del_resp.is_json());
      del_resp.parse_json([&](simdjson::ondemand::document &doc) {
        EXPECT_EQ(doc["_index"].get_string().value(), this->index);
        EXPECT_EQ(doc["_id"].get_string().value(), document_id);
        EXPECT_EQ(doc["_version"].get_int64().value(), 1);
        EXPECT_TRUE(doc["found"].get_bool().value());
        EXPECT_EQ(doc["_source"]["name"].get_string().value(), "brave");
        EXPECT_EQ(doc["_source"]["age"].get_string().value(), "42");
      });

      auto get_resp = this->client->get_document(this->index, document_id)
                                                        .timeout(10000)
                                                        .execute();
      ASSERT_TRUE(get_resp.ok());
      ASSERT_TRUE(get_resp.is_json());
      get_resp.parse_json([&](simdjson::ondemand::document &doc) {
        EXPECT_EQ(doc["_index"].get_string().value(), this->index);
        EXPECT_EQ(doc["_id"].get_string().value(), document_id);
        EXPECT_EQ(doc["_version"].get_int64().value(), 1);
        EXPECT_FALSE(doc["found"].get_bool().value());
      });
    }
  }
//-------------------------------------------------------------------------//
} // namespace