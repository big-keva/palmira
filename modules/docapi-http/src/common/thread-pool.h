/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          thread-pool.h
* - Created:       06/23/2026
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
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
//-------------------------------------------------------------------------//
namespace docapi::common {
//-------------------------------------------------------------------------//
  class thread_pool final {
    std::mutex mtx;
    std::condition_variable cv;
    std::deque<std::function<void()>> queue;
    std::vector<std::thread> workers;
    bool stopping = false;

  public:
    /**
     * Constructor.
     * @param thread_count
     */
    explicit thread_pool(std::size_t thread_count);

    /**
     * Destructor.
     */
    ~thread_pool();

    bool enqueue(std::function<void()> task);
    void stop() noexcept;

  public:
    thread_pool(const thread_pool&) = delete;
    thread_pool &operator=(const thread_pool&) = delete;

  private:
      void onrun() noexcept;
};
//-------------------------------------------------------------------------//
} // namespace docapi::common

