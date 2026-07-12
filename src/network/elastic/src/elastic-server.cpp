#include "network/elastic-server.hpp"
//-------------------------------------------------------------------------//
#include <thread>
#include <filesystem>
//-------------------------------------------------------------------------//
#include <mtc/interfaces.h>
#include <mtc/sharedLibrary.hpp>
//-------------------------------------------------------------------------//
#include "server/http-server.h"
//-------------------------------------------------------------------------//
#include "plugins.hpp"
//-------------------------------------------------------------------------//
namespace elasticapi
{
//-------------------------------------------------------------------------//
  auto CreateServer(mtc::api<palmira::IService> service, const mtc::config &config) -> mtc::api<palmira::IServer>
  {
    const auto &plugin_path = config.get_path("path");
    if (plugin_path.empty())
    {
      throw (std::invalid_argument("ElasticSearch plugin path is empty."));
    }

    if (not std::filesystem::exists(plugin_path))
    {
      throw (std::invalid_argument("The file not found: " + plugin_path));
    }
    std::fprintf(stdout, "INFO Loading ElasticSearch module: %s\n", plugin_path.c_str());

    // Loading a plugin.
    return palmira::LoadPlugin<mtc::api<palmira::IServer>(mtc::api<palmira::IService>, const mtc::config &), mtc::api<palmira::IService>, mtc::config>(
      plugin_path,
      "createServer",
      std::move(service),
      mtc::config(config));
  }
//-------------------------------------------------------------------------//
} // namespace elasticapi
