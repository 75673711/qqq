#include "MessageLoop.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <assert.h>

#define REClOCK std::lock_guard<std::recursive_mutex> locker(mutex_);

static const unsigned int kMessageMaxSize = 40;
static const unsigned int kMessageMaxCount = 100;

namespace ipc = boost::interprocess;

MessageLoop::MessageLoop()
	:is_running_(false),
	ptr_thread_(nullptr),
	buffer_(nullptr),
	read_size_(0)
{

}

MessageLoop::~MessageLoop()
{
	Stop(false);
}

void MessageLoop::Run()
{
	REClOCK
	if (!is_running_)
	{
		buffer_ = new char[kMessageMaxSize];

		boost::uuids::random_generator gen;
		boost::uuids::uuid id = gen();
		std::string pipe_name_ = to_string(id);

		ipc::message_queue::remove(pipe_name_.c_str());
		ptr_redirect_queue_ = new ipc::message_queue
			(ipc::create_only               //only create   
				, pipe_name_.c_str()           //name   
				, kMessageMaxCount                       //max message number   
				, kMessageMaxSize               //max message size   
				);

		ptr_thread_ = new std::thread(&MessageLoop::EventLoop, this);

		is_running_ = true;
	}
}

void MessageLoop::EventLoop()
{
	while (is_running_)
	{
		if (WaitForMessage(1000) == Failed)
		{
			assert(false);
			break;
		}
	}
}

MessageLoop::WaitMessageStatus MessageLoop::WaitForMessage(int msec)
{
	try {
		// todo: receive操作有超时吗
		unsigned int priority;
		bool result = false;

		if (msec == 0)
		{
			result = ptr_redirect_queue_->try_receive(buffer_,
				kMessageMaxCount,
				read_size_,
				priority);
		}
		else
		{
			result = ptr_redirect_queue_->timed_receive(buffer_,
				kMessageMaxCount,
				read_size_,
				priority,
				boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(msec));
		}
		
		if (result)
		{
			return (read_size_ == 0) ? Failed : Success;
		}
		else
		{
			return TimeOut;
		}
	}
	catch (ipc::interprocess_exception &ex) {
		// todo: 错误码区分队列关闭还是溢出
		//assert(false);
		return Failed;
	}
}

void MessageLoop::Stop(bool process_all)
{
	REClOCK
	if (is_running_)
	{
		is_running_ = false;

		if (ptr_thread_->joinable())
		{
			ptr_thread_->join();
		}

		delete ptr_thread_;
		ptr_thread_ = nullptr;

		if (process_all)
		{
			while (WaitForMessage(0) != Success)
			{
				HandleMessage();
			}
		}

		ipc::message_queue::remove(pipe_name_.c_str());

		delete buffer_;
		buffer_ = nullptr;
	}
}

void MessageLoop::HandleMessage()
{
	if (read_size_ > 0)
	{
		int pos = 0;

		EventType e_type;
		ReadArgument(buffer_, pos, e_type);
	}

	int a;
	PushOneTask([=, &a]() { a = 0; });
}

template <typename T>
void MessageLoop::ReadArgument(char* ptr_buf, int& pos, T& argument)
{
	memcpy(&argument, ptr_buf + pos, sizeof(T));
	pos += sizeof(T);
}

bool MessageLoop::PushOneTask(std::function<void(void)> func_)
{
	if (!is_running_)
		return false;

	
	return true;
}