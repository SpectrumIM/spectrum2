#include "transport/ThreadPool.h"
#include "transport/Logging.h"

#include "Swiften/SwiftenCompat.h"

namespace Transport {

DEFINE_LOGGER(threadPoolLogger, "ThreadPool")

ThreadPool::ThreadPool(Swift::EventLoop *loop, int maxthreads) : MAX_THREADS(maxthreads)
{
	this->loop = loop;
	activeThreads = 0;
	worker = (boost::thread **) malloc(sizeof(boost::thread *) * MAX_THREADS);
	for (int i=0 ; i<MAX_THREADS ; i++) {
		worker[i] = NULL;
		freeThreads.push(i);
	}
	onWorkerAvailable.connect(boost::bind(&ThreadPool::scheduleFromQueue, this));
}

ThreadPool::~ThreadPool()
{
	for (int i=0; i<MAX_THREADS ; i++) {
		if (worker[i]) {
			delete worker[i];
		}
	}
	free(worker);

	while (!requestQueue.empty()) {
		Thread *t = requestQueue.front(); requestQueue.pop();
		delete t;
	}
}

int ThreadPool::getActiveThreadCount()
{
	int res;
	count_lock.lock();
	res = activeThreads;
	count_lock.unlock();
	return res;
}

void ThreadPool::updateActiveThreadCount(int k)
{
	count_lock.lock();
	activeThreads += k;
	count_lock.unlock();
}

int ThreadPool::getFreeThread()
{
	int res = -1;
	pool_lock.lock();
	if (!freeThreads.empty()){
		res = freeThreads.front();
		freeThreads.pop();
		updateActiveThreadCount(-1);
	}
	pool_lock.unlock();
	return res;
}

void ThreadPool::releaseThread(int i)
{
	if (i < 0 || i > MAX_THREADS) return;
	pool_lock.lock();

	delete worker[i];
	worker[i] = NULL;
	freeThreads.push(i);

	updateActiveThreadCount(1);

	pool_lock.unlock();
}

void ThreadPool::cleandUp(Thread *t, int wid)
{
	LOG4CXX_INFO(threadPoolLogger, "Cleaning up thread #" << t->getThreadID());
	t->finalize();
	delete t;
	releaseThread(wid);
	onWorkerAvailable();
}

void ThreadPool::scheduleFromQueue()
{
	criticalregion.lock();
	while (!requestQueue.empty()) {
		int  w = getFreeThread();
		if (w == -1) break;

		LOG4CXX_INFO(threadPoolLogger, "Worker Available. Creating thread #" << w);
		Thread *t = requestQueue.front(); requestQueue.pop();
		t->setThreadID(w);
		worker[w] = new boost::thread(boost::bind(&ThreadPool::workerBody, this, _1, _2), t, w, loop);
		updateActiveThreadCount(-1);
	}
	criticalregion.unlock();
}


void ThreadPool::runAsThread(Thread *t)
{
	int w;
	if ((w = getFreeThread()) != -1) {
		LOG4CXX_INFO(threadPoolLogger, "Creating thread #" << w);
		t->setThreadID(w);
		worker[w] = new boost::thread(boost::bind(&ThreadPool::workerBody, this, _1, _2), t, w, loop);
		updateActiveThreadCount(-1);
	}
	else {
		LOG4CXX_INFO(threadPoolLogger, "No workers available! adding to queue.");
		requestQueue.push(t);
	}
}

void ThreadPool::workerBody(Thread *t, int wid) {
	LOG4CXX_INFO(threadPoolLogger, "Starting thread " << wid);
	t->run();
	loop->postEvent(boost::bind(&ThreadPool::cleandUp, this, t, wid), SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::EventOwner>());
}

}
