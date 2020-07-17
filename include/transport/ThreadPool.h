#pragma once 

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <queue>
#include <iostream>
#include "Swiften/EventLoop/EventLoop.h"
#include "Swiften/SwiftenCompat.h"

namespace Transport {

/*
 * Thread serves as a base class for any code that has to be excuted as a thread
 * by the ThreadPool class. The run method defines the code that has to be run
 * as a theard. For example, code in run could be sendinga request to a server
 * waiting for the response and storing the response. When the thread finishes
 * execution, the ThreadPool invokes finalize where one could have the code necessary
 * to collect all the responses and release any resources. 
 *
 * NOTE: The object of the Thread class must be valid (in scope) throughout the 
 * execution of the thread.
 */

class Thread
{
	int threadID;

	public:
	
	Thread() {}
	virtual ~Thread() {}
	virtual void run() = 0;
	virtual void finalize() {}
	int getThreadID() {return threadID;}
	void setThreadID(int tid) {threadID = tid;}
};

/*
 * ThreadPool provides the interface to manage a pool of threads. It schedules jobs
 * on free threads and when the thread completes it automatically deletes the object
 * corresponding to a Thread. If free threads are not available, the requests are 
 * added to a queue and scheduled later when threads become available.
 */

class ThreadPool
{
	const int MAX_THREADS;
	int activeThreads;
	std::queue<int> freeThreads;
	
	std::queue<Thread*> requestQueue;
	boost::thread **worker;

	boost::mutex count_lock;
	boost::mutex pool_lock;
	boost::mutex criticalregion;
	Swift::EventLoop *loop;

	SWIFTEN_SIGNAL_NAMESPACE::signal < void () > onWorkerAvailable;
	
	public:
	ThreadPool(Swift::EventLoop *loop, int maxthreads);
	~ThreadPool();
	void runAsThread(Thread *t);
	int getActiveThreadCount(); 
	void updateActiveThreadCount(int k);
	void cleandUp(Thread *, int);
	void scheduleFromQueue();
	int getFreeThread();
	void releaseThread(int i);
	void workerBody(Thread *t, int wid);
};

}
