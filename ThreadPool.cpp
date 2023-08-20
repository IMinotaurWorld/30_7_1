#include "ThreadPool.h"
OptimizeThreadPool::OptimizeThreadPool()
	: m_thread_count(thread::hardware_concurrency() != 0 ? thread::hardware_concurrency() : 4),
	m_thred_queues(m_thread_count)
{}

void OptimizeThreadPool::start()
{
	for (int i = 0; i < m_thread_count; i++)
	{
		m_threads.emplace_back(&OptimizeThreadPool::threadFunc, this, i);
	}
}

void OptimizeThreadPool::stop()
{
	for (int i = 0; i < m_thread_count; i++)
	{
		task_type empty_task;
		m_thred_queues[i].push(empty_task);
	}

	for (auto& t : m_threads)
		t.join();
}

res_type OptimizeThreadPool::push_task(FuncType f, vector<int>& vect, int arg1, int arg2, RequestHandler& rh)
{
	int queue_to_push = m_qindex++ % m_thread_count;
	task_type new_task = task_type([&vect, arg1, arg2, &rh, f] {f(vect, arg1, arg2, rh); });
	auto res = new_task.get_future();
	m_thred_queues[queue_to_push].push(new_task);

	return res;
}


void OptimizeThreadPool::threadFunc(int qindex)
{
	while (true)
	{
		task_type task_to_do;
		bool res;
		int i = 0;
		for (; i < m_thread_count; i++)
		{
			if (res = m_thred_queues[(qindex + i) % m_thread_count].fast_pop(task_to_do))
				break;
		}
		if (!res)
		{
			m_thred_queues[qindex].pop(task_to_do);
		}
		else if (!task_to_do.valid())
		{
			m_thred_queues[(qindex + i) % m_thread_count].push(task_to_do);
		}
		if (!task_to_do.valid())
		{
			return;
		}

		task_to_do();
	}
}


void OptimizeThreadPool::run_pending_task()
{
	task_type task_to_do;
	bool res;
	int i = 0;
	for (; i < m_thread_count; ++i)
	{
		res = m_thred_queues[i % m_thread_count].fast_pop(task_to_do);
		if (res) break;
	}

	if (!task_to_do.valid())
		std::this_thread::yield();
	else
		task_to_do();
}

RequestHandler::RequestHandler()
{
	this->m_tpool.start();
}

RequestHandler::~RequestHandler()
{
	this->m_tpool.stop();
}

res_type RequestHandler::push_task(FuncType f, vector<int>& vect, int arg1, int arg2, RequestHandler& rh)
{
	return this->m_tpool.push_task(f, vect, arg1, arg2, rh);
}

void RequestHandler::run_pending_task()
{
	this->m_tpool.run_pending_task();
}