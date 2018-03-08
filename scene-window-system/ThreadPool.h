#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

#define TEST_THREAD_JOB_WAIT_TIME 1

using Clock = std::chrono::high_resolution_clock;	//<-- for debugging

class ThreadHandler
{
private:
	std::thread* thread;
	int id;

public:
	//TODO: make diz private?
	volatile bool isWorking = false;
	Clock::time_point endTime;
	Clock::time_point startTime;
	
	ThreadHandler(std::thread* thread, int id) : thread(thread), id(id) {};
	ThreadHandler() = default;

	~ThreadHandler() {
		delete thread;
	}

	void Detach()
	{
		thread->detach();
	}

	void SetThread(std::thread* t) {
		thread = t;
	}
};

template<typename ArgT>
class ThreadJob
{
private:
	void (*job)(ArgT&);
	ArgT arg;	
public:
	ThreadJob<ArgT>(void (*job)(ArgT&), ArgT& arg) : job(job), arg(arg) {}

	void Execute() {
		job(arg);
	}
};

template<typename ArgT>
struct ThreadArg
{
	std::queue<ThreadJob<ArgT>>& jobs;
	std::mutex* joblock;
	bool& keepThreadsAlive = true;
	volatile bool& isWorking;
	volatile size_t& terminatedThreads = 0;
	int id;
	Clock::time_point* startTime;
	Clock::time_point* endTime;
};

template<typename ArgT>
class ThreadPool
{
private:
	std::vector<ThreadHandler*> threadHandlers;
	std::queue<ThreadJob<ArgT>> jobs;
	std::mutex joblock;
	bool keepThreadsAlive = true;
	volatile size_t terminatedThreads = 0;
public:

	ThreadPool<ArgT>(size_t threadCount) {

		threadHandlers.resize(threadCount);
		for (auto i = 0; i < threadCount; ++i) {

			//temp handler (we don't have the thread yet)
			auto threadHandler = new ThreadHandler();

			//create the thread (with a member from threadHandler as an argument)
			ThreadArg<ArgT> arg = { jobs, &joblock, keepThreadsAlive, threadHandler->isWorking, terminatedThreads, i, &threadHandler->startTime, &threadHandler->endTime };
			std::thread* thread = new std::thread(ThreadMain<ArgT>, arg);

			threadHandler->SetThread(thread);

			threadHandlers[i] = threadHandler;
		}


		for (auto& thread : threadHandlers) {
			thread->Detach();
		}
	}
	
	~ThreadPool() {
		Stop();

		//make sure all spawned threads have terminated before detructing fields (as spawned threads have references to them)
		while (terminatedThreads < threadHandlers.size()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(TEST_THREAD_JOB_WAIT_TIME));
		}

		for (auto& handler : threadHandlers) {
			delete handler;
		}
	}

	void AddThreadJob(ThreadJob<ArgT> job) {
		joblock.lock();
		jobs.push(job);
		joblock.unlock();
	}

	bool Idle()
	{
		joblock.lock();
		bool noJobs = jobs.empty();

		bool threadsDone = noJobs;
		for (auto& handler : threadHandlers) {
			threadsDone = threadsDone && !handler->isWorking;
		}
		joblock.unlock();
		return threadsDone;
	}

	void Stop()
	{
		keepThreadsAlive = false;
	}
};

template<typename ArgT>
void ThreadMain(ThreadArg<ArgT>& arg) {
	auto& jobs = arg.jobs;
	auto& joblock = *arg.joblock;
	auto& keepAlive = arg.keepThreadsAlive;
	auto& terminatedThreads = arg.terminatedThreads;
	auto& startTime = *arg.startTime;
	auto& endTime = *arg.endTime;

	while (keepAlive) {
		bool locked = joblock.try_lock();
		if (locked && jobs.size() > 0) {
			startTime = Clock::time_point();
			endTime = Clock::time_point();

			auto threadJob = jobs.front();
			jobs.pop();

			arg.isWorking = true;
			joblock.unlock();
			
			startTime = Clock::now();
			threadJob.Execute();
			endTime = Clock::now();
			arg.isWorking = false;
		}
		else {
			if (locked) {
				joblock.unlock();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(TEST_THREAD_JOB_WAIT_TIME));
		}
	}
	joblock.lock();
	++terminatedThreads;
	joblock.unlock();
}