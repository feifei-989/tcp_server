#include "ThreadPool.h"
#include <iostream>

namespace tcp_server {

ThreadPool::ThreadPool(size_t threadCount)
    : stopped_(false) {
    
    if (threadCount == 0) {
        threadCount = 1;
    }

    std::cout << "Creating thread pool with " << threadCount << " threads" << std::endl;

    for (size_t i = 0; i < threadCount; ++i) {
        threads_.emplace_back([this]() { workerThread(); });
    }
}

ThreadPool::~ThreadPool() {
    stopped_ = true;
    condition_.notify_all();

    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::cout << "Thread pool destroyed" << std::endl;
}

void ThreadPool::submit(Task task) {
    if (stopped_) {
        std::cerr << "Cannot submit task to stopped thread pool" << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    
    condition_.notify_one();
}

size_t ThreadPool::getPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void ThreadPool::workerThread() {
    while (!stopped_) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this]() {
                return stopped_ || !tasks_.empty();
            });

            if (stopped_ && tasks_.empty()) {
                return;
            }

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in thread pool task" << std::endl;
            }
        }
    }
}

} // namespace tcp_server
