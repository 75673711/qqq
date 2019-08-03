#ifndef MESSAGELOOP_H__
#define MESSAGELOOP_H__

#include <boost/interprocess/ipc/message_queue.hpp>  

#include <atomic>
#include <thread>
#include <mutex>
#include <functional>

#include <QWidget>



class MessageLoop
{
public:
	enum EventType {
		EventType_Normal,    // use NormalEvent
		EventType_Execute    // 
	};

public:
	static MessageLoop& GetInstance() {
		static MessageLoop instance;
		return instance;
	}

	void Run();
	// 消息不处理完就停止会导致内存泄漏
	void Stop(bool process_all); 

	bool PushOneTask(std::function<void(void)> func_);

protected:
	enum WaitMessageStatus {
		Success,
		TimeOut,
		Failed,
	};

	MessageLoop();
	~MessageLoop();

	void EventLoop();
	WaitMessageStatus WaitForMessage(int msec);
	void HandleMessage();

	template <typename T>
	void ReadArgument(char* ptr_buf, int& pos, T& argument);


private:
	std::atomic<bool> is_running_;
	std::thread* ptr_thread_;
	boost::interprocess::message_queue* ptr_redirect_queue_;
	std::string pipe_name_;

	char* buffer_;
	unsigned int read_size_;

	std::recursive_mutex mutex_;

	std::function<void(void)> func_;
};

#endif // MESSAGELOOP_H__