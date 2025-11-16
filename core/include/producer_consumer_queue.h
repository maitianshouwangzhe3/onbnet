#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <atomic>
#include <type_traits>

template <typename T>
class producer_consumer_queue
{
private:
    std::mutex _queueLock;
    std::queue<T> _queue;
    std::condition_variable _condition;
    std::atomic<bool> _shutdown;
    std::atomic<int> _count;

public:

    producer_consumer_queue<T>() : _shutdown(false), _count(0) { }

    void Push(const T& value)
    {
        std::lock_guard<std::mutex> lock(_queueLock);
        _queue.push(value);
        _count.fetch_add(1, std::memory_order_release);

        _condition.notify_one();
    }

    void PushPtr(T value)
    {
        std::lock_guard<std::mutex> lock(_queueLock);
        _queue.push(value);
        _count.fetch_add(1, std::memory_order_release);

        _condition.notify_one();
    }

    bool Empty()
    {
        std::lock_guard<std::mutex> lock(_queueLock);

        return _count.load(std::memory_order_release) == 0;
    }

    size_t Size() const
    {
        return _count.load(std::memory_order_release);
    }

    bool Pop(T& value)
    {
        std::lock_guard<std::mutex> lock(_queueLock);

        if (_queue.empty() || _shutdown)
            return false;

        value = _queue.front();

        _queue.pop();
        _count.fetch_sub(1, std::memory_order_release);

        return true;
    }

    T PopPtr() {
        std::lock_guard<std::mutex> lock(_queueLock);
        if (_queue.empty() || _shutdown)
            return nullptr;

        auto ptr = _queue.front();
        _queue.pop();
        _count.fetch_sub(1, std::memory_order_release);
        return ptr;
    }

    T WaitAndPop()
    {
        std::unique_lock<std::mutex> lock(_queueLock);
        // 使用值初始化替代 nullptr 初始化，支持所有类型 T
        T value{};
        
        // 等待直到队列非空或被关闭
        while (_queue.empty() && !_shutdown)
            _condition.wait(lock);

        // 再次检查队列是否为空（防止虚假唤醒或多个线程被同时唤醒）
        if (_queue.empty() || _shutdown)
            return value;

        value = _queue.front();
        _queue.pop();
        _count.fetch_sub(1, std::memory_order_release);

        return value;
    }

    void Cancel()
    {
        std::unique_lock<std::mutex> lock(_queueLock);

        while (!_queue.empty())
        {
            T& value = _queue.front();

            DeleteQueuedObject(value);

            _queue.pop();
        }

        _shutdown = true;
        _count.store(0, std::memory_order_release);

        _condition.notify_all();
    }

private:
    template<typename E = T>
    typename std::enable_if<std::is_pointer<E>::value>::type DeleteQueuedObject(E& obj) { delete obj; }

    template<typename E = T>
    typename std::enable_if<!std::is_pointer<E>::value>::type DeleteQueuedObject(E const& /*packet*/) { }
};