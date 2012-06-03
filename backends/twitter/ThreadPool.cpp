#include "ThreadPool.h"
DEFINE_LOGGER(logger, "ThreadPool")
boost::signals2::signal< void (Thread*, int) > onWorkCompleted;

static void Worker(Thread *t, int wid, Swift::EventLoop *loop)
{
	LOG4CXX_INFO(logger, "Starting thread " << wid)
	t->run();
	loop->postEvent(boost::bind(boost::ref(onWorkCompleted), t, wid), boost::shared_ptr<Swift::EventOwner>());
}


ThreadPool::ThreadPool(Swift::EventLoop *loop, int maxthreads) : MAX_THREADS(maxthreads)
{
	this->loop = loop;
	activeThreads = 0;
	worker = new boost::thread*[MAX_THREADS];
	for(int i=0 ; i<MAX_THREADS ; i++) {
		worker[i] = NULL;
		freeThreads.push(i);
	}
	onWorkCompleted.connect(boost::bind(&ThreadPool::cleandUp, this, _1, _2));
	onWorkerAvailable.connect(boost::bind(&ThreadPool::scheduleFromQueue, this));
}

ThreadPool::~ThreadPool()
{
	for(int i=0; i<MAX_THREADS ; i++) {
		if(worker[i]) {
			delete worker[i];
		}
	}
	delete worker;

	while(!requestQueue.empty()) {
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
	if(!freeThreads.empty()){
		res = freeThreads.front();
		freeThreads.pop();
		updateActiveThreadCount(-1);
	}
	pool_lock.unlock();
	return res;
}

void ThreadPool::releaseThread(int i)
{
	if(i < 0 || i > MAX_THREADS) return;
	pool_lock.lock();

	delete worker[i];
	worker[i] = NULL;
	freeThreads.push(i);
	
	updateActiveThreadCount(1);
	
	pool_lock.unlock();
}

void ThreadPool::cleandUp(Thread *t, int wid)
{
	LOG4CXX_INFO(logger, "Cleaning up thread #" << t->getThreadID())
	t->finalize();
	delete t;
	releaseThread(wid);
	onWorkerAvailable();	
}

void ThreadPool::scheduleFromQueue()
{
	criticalregion.lock();	
	while(!requestQueue.empty()) {
		int  w = getFreeThread();
		if(w == -1) break;

		LOG4CXX_INFO(logger, "Worker Available. Creating thread #" << w)
		Thread *t = requestQueue.front(); requestQueue.pop();
		t->setThreadID(w);
		worker[w] = new boost::thread(Worker, t, w, loop);
		updateActiveThreadCount(-1);
	}
	criticalregion.unlock();
}


void ThreadPool::runAsThread(Thread *t)
{
	int w;
	if((w = getFreeThread()) != -1) {
		LOG4CXX_INFO(logger, "Creating thread #" << w)
		t->setThreadID(w);
		worker[w] = new boost::thread(Worker, t, w, loop);
		updateActiveThreadCount(-1);
	}
	else {
		LOG4CXX_INFO(logger, "No workers available! adding to queue.")
		requestQueue.push(t);
	}
}
