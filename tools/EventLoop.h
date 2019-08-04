#ifndef EVENTLOOP_H__
#define EVENTLOOP_H__

#include <boost/asio.hpp>

#include <atomic>
#include <thread>
#include <mutex>
#include <functional>

class EventLoop
{
public:
	static EventLoop& GetInstance() {
		static EventLoop instance;
		return instance;
	}

	void Run();
	// stop时所有剩余任务都将立即被处理
	void Stop(); 

	bool PushOneTask(const std::function<void(void)>& func_, int msec = 0);

protected:
	EventLoop();
	~EventLoop();

private:
	std::atomic<bool> is_running_;
	std::thread* ptr_thread_;

	std::recursive_mutex mutex_;
	boost::asio::io_context ioc_;
	boost::asio::io_service::work* ptr_work_;
};

#endif // EVENTLOOP_H__