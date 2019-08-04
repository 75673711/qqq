#include "EventLoop.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <assert.h>

#define REClOCK std::lock_guard<std::recursive_mutex> locker(mutex_);

EventLoop::EventLoop()
	:is_running_(false),
	ptr_thread_(nullptr),
	ptr_work_(nullptr)
{

}

EventLoop::~EventLoop()
{
	Stop();
}

void EventLoop::Run()
{
	REClOCK
	if (!is_running_)
	{
		// 使用work  构造时为ioc_任务+1  使其run在任务完成时不退出  继续运行    析构时将任务-1并保证所有任务完成
		ptr_work_ = new boost::asio::io_service::work(ioc_);
		ptr_thread_ = new std::thread([this]() {
			ioc_.run();
		});

		is_running_ = true;
	}
}

void EventLoop::Stop()
{
	REClOCK
	if (is_running_)
	{
		is_running_ = false;

		delete ptr_work_;
		ptr_work_ = nullptr;

		if (ptr_thread_->joinable())
		{
			ptr_thread_->join();
		}

		delete ptr_thread_;
		ptr_thread_ = nullptr;
	}
}

static void callback(const boost::system::error_code& e, std::function<void(void)> func_, boost::asio::deadline_timer *timer)
{
	func_();
	delete timer;
}

bool EventLoop::PushOneTask(const std::function<void(void)>& func_, int msec)
{
	if (!is_running_)
		return false;

	boost::asio::deadline_timer* t = new boost::asio::deadline_timer(ioc_, boost::posix_time::millisec(msec));
	//t->async_wait([func_, t](const boost::system::error_code&) {
	//	func_();
	//	delete t;
	//});

	t->async_wait(boost::bind(&callback, _1, func_, t));
	
	return true;
}