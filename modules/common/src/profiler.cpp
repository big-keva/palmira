#include "../profiler.h"
//-------------------------------------------------------------------------//
namespace palmira::modules {
//-------------------------------------------------------------------------//
  auto profiler::event_default(duration_t elapsed) -> void {
    std::fprintf(stdout, "Profiler: %lu ms\n", static_cast<std::uint64_t>(elapsed.count()));
  }
//-------------------------------------------------------------------------//
  profiler::profiler(onevent_t fonevent /*= {}*/) : begin(std::chrono::steady_clock::now()),
                                                    onevent(std::move(fonevent)) {
  }

  profiler::~profiler() noexcept {
    try {
      if (this->onevent) {
        // Raising event.
        this->onevent(duration_t(std::chrono::steady_clock::now() - this->begin));
      } else {
        // Raising default event.
        profiler::event_default(duration_t(std::chrono::steady_clock::now() - this->begin));
      }
    } catch (...) {
    }
  }

  auto profiler::now() const noexcept -> duration_t {
    return {std::chrono::steady_clock::now() - this->begin};
  }
//-------------------------------------------------------------------------//
  auto get_duration_in_seconds_as_string(profiler::duration_t elapsed) -> std::string {
    return get_duration_in_seconds_as_string(float(elapsed.count()));
  }

  auto get_duration_in_seconds_as_string(float elapsed) -> std::string {
    char buffer[100] = {0};
    // Converting number to string.
    return std::sprintf(buffer, "%.3f", elapsed / 1000.), buffer;
  }
//-------------------------------------------------------------------------//
} // namespace palmira::modules
