#include "SimpleThreadPooler.h"

SimpleThreadPooler::SimpleThreadPooler(uint32_t max_threads)
{
	thread_limit = max_threads;
	_wthreads.resize(thread_limit);
	_rem_tasks.resize(MAX_TASKS);
}

//template<typename F, typename ...Fargs>
//inline void SimpleThreadPooler::add_task(F&& f, Fargs && ...args)
//{
//	at_lock.lock();
//	_rem_tasks.push(std::function<void()>([f, args...]{ f(args...); }));
//	at_lock.unlock();
//}

void SimpleThreadPooler::do_work()
{
	while (!stop_work) {
		
		at_lock.lock();
		if (rti != rtj) {
			std::function<void()> t = _rem_tasks[rti];
			rti = (rti + 1) % MAX_TASKS;
			at_lock.unlock();

			t();
		}
		else {
			at_lock.unlock();
		}
	}
}

void SimpleThreadPooler::run()
{
	stop_work = false;
	for (int i = 0; i < thread_limit; i++) {
		_wthreads[i] = new std::thread(&SimpleThreadPooler::do_work, this);
	}
}

void SimpleThreadPooler::stop()
{
	while (true) {
		at_lock.lock();
		bool is_empty = rti == rtj;
		at_lock.unlock();

		if (is_empty) {
			stop_work = true;

			for (int i = 0; i < thread_limit; i++) {
				if (_wthreads[i]->joinable()) _wthreads[i]->join();
				delete _wthreads[i];
			}
			break;
		}
		
	}
}
