#include "thread-pool.h"
//-------------------------------------------------------------------------//
#include <exception>

//-------------------------------------------------------------------------//
namespace docapi::common {
  //-------------------------------------------------------------------------//
  thread_pool::thread_pool(std::size_t thread_count) {
    if (thread_count == 0) {
      thread_count = 1;
    }
    this->workers.reserve(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i) {
      this->workers.emplace_back([this]() {
        onrun();
      });
    }
  }

  thread_pool::~thread_pool() {
    this->stop();
  }

  bool thread_pool::enqueue(std::function<void()> task) {
    if (!task) {
      return false;
    }
    {
      std::lock_guard sync(this->mtx);
      if (this->stopping) {
        return false;
      }

      this->queue.push_back(std::move(task));
    }

    return this->cv.notify_one(), true;
  }

  void thread_pool::stop() noexcept {
    {
      std::lock_guard sync(this->mtx);
      this->stopping = true;
    }

    this->cv.notify_all();

    for (auto &worker: this->workers) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  //-------------------------------------------------------------------------//
  void thread_pool::onrun() noexcept {
    for (;;) {
      std::function<void()> task;

      {
        std::unique_lock sync(this->mtx);
        this->cv.wait(sync, [this]() {
          return this->stopping || !this->queue.empty();
        });

        if (this->stopping && this->queue.empty()) {
          return;
        }

        task = std::move(this->queue.front());
        this->queue.pop_front();
      }

      try {
        task();
      } catch (...) {
        // В production здесь должен быть callback логирования.
        // Исключение из worker-потока не должно завершать процесс.
      }
    }
  }

  //-------------------------------------------------------------------------//
} // namespace osrest::detail
