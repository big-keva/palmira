/*!==========================================================================
* \file
* - Program:       modules-common
* - File:          modules-loader.h
* - Created:       06/26/2026
* - Author:        Vitaly Bulganin
* - Description:
* - Comments:
*
-----------------------------------------------------------------------------
*
* - History:
*
===========================================================================*/
#pragma once
//-------------------------------------------------------------------------//
#ifndef __MODULES_LOADER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __MODULES_LOADER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <dlfcn.h>
//-------------------------------------------------------------------------//
namespace palmira::modules {
//-------------------------------------------------------------------------//
  class modules_loader final {
    struct module_file {
      //!< Keeps module settings.
      const mtc::zmap settings;

      //!< Keeps a handle.
      void *handle = nullptr;
    };

    //!< Keeps a list of loaded plugins.
    mutable std::unordered_map<std::string, std::shared_ptr<module_file>> plugins;

  public:
    /**
     * Constructor.
     * @param config [in] - A path of keeping plugins.
     */
    explicit modules_loader(const mtc::config &config) {
      auto push_module = [&](const std::string &module_name, const mtc::zmap &settings) -> void {
        // Loading a module.
        auto handle = dlopen(module_name.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (handle != nullptr) {
          const auto error = dlerror();
          if (error == nullptr) {//<TODO> Adding error handle.
          }

          auto module = std::make_shared<module_file>(module_file{
            .settings = settings,
            .handle = handle
          });

          // Saving a new module.
          this->plugins.emplace(module_name, module);
        } else {
          const auto error = dlerror();
          if (error != nullptr) {//<TODO> Adding error handle.
            std::fprintf(stderr, "Module %s not loaded: %s\n", module_name.c_str(), error);
          } else {
            std::fprintf(stderr, "Module not loaded: %s\n", module_name.c_str());
          }
        }
      };

      if (config.has_key("path") && not config.get_charstr("path").empty()) {//<TODO> Add impl settings
        if (not std::filesystem::is_directory(config.get_charstr("path"))) {
          throw (std::invalid_argument("Module path is not directory: " + config.get_charstr("path")));
        }

        for (const auto &module : std::filesystem::directory_iterator(config.get_charstr("path"))) {
          if (std::filesystem::is_regular_file(module.status())) {
            // Adding a new module.
            push_module(module.path().string(), {});
          }
        }
      }

      if (config.has_key("files") && not config.to_zmap().get_array_zmap("files", {}).empty()) {
        for (const auto &module_settings : config.to_zmap().get_array_zmap("files", {})) {
          const auto file_name = module_settings.get_charstr("file", "");
          if (not file_name.empty() && not std::filesystem::is_regular_file(file_name)) {
            std::fprintf(stderr, "Module file %s is not a regular file\n", file_name.c_str());
            continue;
          }

          // Adding a new module.
          push_module(file_name, module_settings);
        }
      }

      if (this->plugins.empty()) {
        std::fprintf(stderr, "No one module loaded\n");
      }
    }

    /**
     * Destructor.
     */
    ~modules_loader() {
      for (auto &plugin : this->plugins) {
        if (plugin.second != nullptr && plugin.second->handle != nullptr) {
          // Closing loaded module.
          dlclose(plugin.second->handle);
        }
      }
    }

    modules_loader(const modules_loader &) = delete;
    modules_loader(const modules_loader &&) = delete;
    modules_loader &operator=(const modules_loader &) = delete;
    modules_loader &operator=(modules_loader &&) = delete;

    template<typename symbol_t>
    auto load(const char *symbol_name, std::function<void(symbol_t symbol, const mtc::zmap &module_settings, const char *error)> onload) const -> void {
      for (const auto &plugin : this->plugins) {
        // Getting a symbol by it name.
        auto symbol = reinterpret_cast<symbol_t>(dlsym(plugin.second->handle, symbol_name));

        if (onload != nullptr) {
          // Raising ONLOAD event.
          onload(reinterpret_cast<symbol_t>(symbol), plugin.second->settings, dlerror());
        }
      }
    }
  };
//-------------------------------------------------------------------------//
} // namespace palmira::modules
//-------------------------------------------------------------------------//
#endif // __MODULES_LOADER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
