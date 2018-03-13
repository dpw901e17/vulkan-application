#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

using task_t = std::function<void()>;

class ThreadPool
{
	std::vector<std::thread> m_Threads;
	std::queue<task_t> m_Tasks;

	std::mutex m_QueueMutex;
	std::condition_variable m_ConditionVariable;
	bool stopped = false;
	std::atomic_int active_threads;
public:
	ThreadPool(size_t thread_count);
	ThreadPool(const ThreadPool&) = delete; // No copying
	~ThreadPool() noexcept;

	template<class TFunc, class... TArgs>
	auto enqueue(TFunc&&, TArgs&&...)->std::future<typename std::result_of<TFunc(TArgs...)>::type>;

	ThreadPool operator=(const ThreadPool&) = delete; // No assigning

private:
	template<class result_t>
	void inner_enqueue(std::shared_ptr<std::packaged_task<result_t()>> task);
	task_t get_next_task();
	void thread_function();
	bool task_ready();
	void stop();
};

inline bool ThreadPool::task_ready()
{
	return stopped || !m_Tasks.empty();
}

inline void ThreadPool::stop()
{
	std::unique_lock<std::mutex> lock(m_QueueMutex);
	stopped = true;
}

inline task_t ThreadPool::get_next_task()
{
	std::unique_lock<std::mutex> lock(m_QueueMutex);
	m_ConditionVariable.wait(lock, [this] {return this->task_ready();});
	if (stopped && m_Tasks.empty())
	{
		return [] {}; // Return empty task
	}
	auto result = std::move(m_Tasks.front());
	m_Tasks.pop();
	++active_threads;
	return std::move(result);
}

inline void ThreadPool::thread_function()
{
	while (!stopped)
	{
		auto task = get_next_task();
		task();
		--active_threads;
	}
}

inline ThreadPool::ThreadPool(size_t thread_count)
{
	for (int i = 0; i < thread_count; ++i)
	{
		m_Threads.emplace_back(
			[this]
		{
			this->thread_function();
		}
		);
	}
}

template<class result_t>
inline void ThreadPool::inner_enqueue(std::shared_ptr<std::packaged_task<result_t()>> task)
{
	std::unique_lock<std::mutex> lock(m_QueueMutex);

	// don't allow enqueueing after stopping the pool
	if (stopped)
	{
		throw std::runtime_error("Tried to enqueue task on stopped ThreadPool");
	}

	m_Tasks.emplace([task]() { (*task)(); });
}

template<class TFunc, class... TArgs>
inline auto ThreadPool::enqueue(TFunc&& func, TArgs&&... args)
-> std::future<typename std::result_of<TFunc(TArgs...)>::type>
{
	using result_t = typename std::result_of<TFunc(TArgs...)>::type;

	auto task = std::make_shared<std::packaged_task<result_t()>>(
		std::bind(std::forward<TFunc>(func), std::forward<TArgs>(args)...)
		);
	inner_enqueue(task);
	auto result = task->get_future();

	m_ConditionVariable.notify_one();
	return result;
}

inline ThreadPool::~ThreadPool() noexcept
{
	stop();
	m_ConditionVariable.notify_all();
	for (auto& thread : m_Threads)
	{
		thread.join();
	}
}
