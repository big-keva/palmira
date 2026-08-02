#include "../thread-pool.h"

#include "network/elasticAPI/src/logger/logger.h"
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  thread_pool::thread_pool(std::size_t thread_count)
  {
    if (thread_count == 0)
    {
      thread_count = 1;
    }
    this->workers.reserve(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i)
    {
      this->workers.emplace_back([this]() {
        this->onrun();
      });
    }
  }

  thread_pool::~thread_pool()
  {
    this->stop();
  }

  bool thread_pool::enqueue(ontask_t ontask)
  {
    if (not ontask)
    {
      return false;
    }
    {
      std::lock_guard sync(this->mtx);
      if (this->stopping)
      {
        return false;
      }

      this->queue.emplace_back(std::move(ontask));
    }

    return this->cv.notify_one(), true;
  }

  void thread_pool::stop() noexcept
  {
    {
      std::lock_guard sync(this->mtx);
      this->stopping = true;
    }
    this->cv.notify_all();

    for (auto &worker : this->workers)
    {
      if (worker.joinable())
      {
        worker.join();
      }
    }
  }
//-------------------------------------------------------------------------//
  void thread_pool::onrun() noexcept
  {
    while (not this->stopping)
    {
      ontask_t task;
      {
        std::unique_lock sync(this->mtx);
        this->cv.wait(sync, [this]() {
          return this->stopping || !this->queue.empty();
        });

        if (this->stopping && this->queue.empty())
        {
          return;
        }

        task = std::move(this->queue.front());
        this->queue.pop_front();
      }

      if (not task)
      {
        continue;
      }

      try
      {
        // Executing a task.
        task();
      }
      catch (const std::exception &exc)
      { //<TODO> Adding logging.
        LOG_E_C("Proceeding request failed: %s", exc.what());
      }
      catch (...)
      { //<TODO> Adding logging.
        LOG_E_C("Proceeding request failed: unknown");
      }
    }
  }
//-------------------------------------------------------------------------//
} // namespace elastic
