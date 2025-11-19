#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <memory>

namespace tcp_server {

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Submit a task to the thread pool
    void submit(Task task);

    // Get number of threads
    size_t getThreadCount() const { return threads_.size(); }

    // Get pending task count
    size_t getPendingTaskCount() const;

private:
    void workerThread();

    std::vector<std::thread> threads_;
    std::queue<Task> tasks_;
    
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stopped_;
};

using ThreadPoolPtr = std::shared_ptr<ThreadPool>;

} // namespace tcp_server
