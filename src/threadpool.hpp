//
// Created by some yuan on 2019/8/1.
//

#ifndef CELL_THREADPOOL_HPP
#define CELL_THREADPOOL_HPP

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

namespace cell {
    class ThreadPool {
    public:
        explicit ThreadPool(size_t poolSize = std::thread::hardware_concurrency());

        template<class F, class... Args>
        auto execute(F &&f, Args &&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

        ~ThreadPool();

    private:
        // keep thread trace to join
        std::vector<std::thread> workers;
        // task queue
        std::queue<std::function<void()>> tasks;

        std::mutex mtx_;
        std::condition_variable cond_;

        bool isShutDown_;
    };

    inline
    ThreadPool::ThreadPool(size_t poolSize) : isShutDown_(false) {
        for (size_t i = 0; i < poolSize; ++i) {
            workers.emplace_back([this] {
                for(;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->mtx_);
                        this->cond_.wait(lock, [this] {
                            return this->isShutDown_ || !this->tasks.empty();
                        });
                        if (this->isShutDown_ && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto ThreadPool::execute(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using  returnType = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<returnType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );

        std::future<returnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(mtx_);
            if (isShutDown_)
                throw std::runtime_error("ThreadPool is shutdown!");
            tasks.emplace([task](){ (*task)(); });
        }
        cond_.notify_one();
        return res;
    }

    ThreadPool::~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            isShutDown_ = true;
        }
        cond_.notify_all();
        for (auto& worker : workers)
        {
            if (worker.joinable())
                worker.join();
        }
    }

}

#endif //CELL_THREADPOOL_HPP
