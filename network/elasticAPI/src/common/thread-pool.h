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
namespace elastic
{
//-------------------------------------------------------------------------//
  class thread_pool final
  {
    using ontask_t = std::function<void()>;

    //!< Keeps a mutex.
    std::mutex mtx;

    //!< Keeps a condition.
    std::condition_variable cv;

    //!< Keeps a queue of tasks.
    std::deque<ontask_t> queue;

    //!< Keeps a list of workers.
    std::vector<std::thread> workers;

    //!< Keeps a flag of stopping.
    std::atomic_bool stopping{false};

  public:
    /**
     * Constructor.
     * @param thread_count
     */
    explicit thread_pool(std::size_t thread_count);

    thread_pool(const thread_pool &) = delete;
    thread_pool(thread_pool &&) = delete;
    thread_pool &operator=(const thread_pool &) = delete;
    thread_pool &&operator=(thread_pool &&) = delete;

    /**
     * Destructor.
     */
    ~thread_pool();

    /**
     * Adds a new task into queue.
     * @param ontask [in] - A callback function.
     * @return
     */
    bool enqueue(ontask_t ontask);

    //!< Stops proceeding queue.
    void stop() noexcept;

  private:
    void onrun() noexcept;
  };
//-------------------------------------------------------------------------//
} // namespace elastic
