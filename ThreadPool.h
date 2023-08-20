#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <future>

using namespace std;

class RequestHandler;

typedef std::packaged_task<void()> task_type; 
typedef std::future<void> res_type;
typedef void (*FuncType) (vector<int>&, int, int, RequestHandler&);

template<typename T>
class BlockedQueue
{
public:
	void push(T& item)
	{
		std::lock_guard<std::mutex> l(m_locker);
		m_task_queue.push(std::move(item));
		m_notifier.notify_one();
	}

	void pop(T& item)
	{
		std::unique_lock<std::mutex> l(m_locker);
		if (m_task_queue.empty())
		{
			m_notifier.wait(l, [this]() {return !m_task_queue.empty(); });
		}
		item = std::move(m_task_queue.front());
		m_task_queue.pop();
	}

	bool fast_pop(T& item)
	{
		std::lock_guard<std::mutex> l(m_locker);
		if (m_task_queue.empty())
		{
			return false;
		}
		item = std::move(m_task_queue.front());
		m_task_queue.pop();
		return true;
	}
private:
	std::queue<T> m_task_queue;
	std::mutex m_locker;
	std::condition_variable m_notifier;
};

class OptimizeThreadPool
{
public:
	OptimizeThreadPool();
	void start(); 
	void stop(); 
	res_type push_task(FuncType f, vector<int>& vect, int arg1, int arg2, RequestHandler&); 
	void threadFunc(int qindex);
	void run_pending_task();
private:
	int m_thread_count;
	vector<thread> m_threads;
	vector<BlockedQueue<task_type>> m_thred_queues;
	unsigned m_qindex;
};


class RequestHandler
{
public:
	RequestHandler();
	~RequestHandler();
	res_type push_task(FuncType f, vector<int>& vect, int arg1, int arg2, RequestHandler&);
	void run_pending_task();
private:
	OptimizeThreadPool m_tpool;
};