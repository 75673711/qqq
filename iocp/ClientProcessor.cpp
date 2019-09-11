#include "ClientProcessor.h"

#include <QDebug>

ClientProcessor::ClientProcessor() : QObject(nullptr)
{

}

ClientProcessor::~ClientProcessor()
{

}

bool ClientProcessor::Init(PER_SOCKET_CONTEXT* ptr_context)
{
	ptr_context_ = ptr_context;

	return true;
}

void ClientProcessor::Uninit()
{

}

// todo: 可以发的同时接收吗？  可以的话ptr_context_->pIOContext->IOOperation被重置怎么识别出来的
// 为每一个操作新建一个overlapped   读和写各一个  修改结构体吧
// GetQueuedCompletionStatus时会返回overlapped指针  相对应的是调用异步函数时传入的指针   通过相等来判断是哪个io操作 ？

// todo: 发送数据时为什么会有发一半的情况？   此时应该讲缓存里的填过去满载发 还是 先把剩下的发完？
void ClientProcessor::OnGetNewData(DWORD size)
{
	QMutexLocker locker(&recv_mutex_);

	if (ptr_context_ != nullptr)
	{
		ptr_context_->pIOContext->Buffer;

		// wsabuf 就是  ptr_context_->pIOContext->Buffer  为了发送接口调用而封装的

		// 测试发送数据
		//ptr_context_->pIOContext->IOOperation = ClientIoWrite;
		//ptr_context_->pIOContext->nTotalBytes = 5;
		//ptr_context_->pIOContext->nSentBytes = 0;
		//ptr_context_->pIOContext->wsabuf.len = 5;
		//memcpy(ptr_context_->pIOContext->wsabuf.buf, "hello", 5);
		//DWORD dwFlags = 0;
		//dwFlags = 0;
		//int nRet = WSASend(
		//	ptr_context_->Socket,
		//	&ptr_context_->pIOContext->wsabuf, 1, NULL,
		//	dwFlags,
		//	&(ptr_context_->pIOContext->Overlapped), NULL);
		//if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
		//	Uninit();
		//}

		// -----------------------------------

		
		if (size > 0)  // 为0时代表第一次建立连接完成  开始读数据
		{
			// todo:收到数据后回调给业务层
			qDebug() << QByteArray(ptr_context_->pIOContext->wsabuf.buf, size);
		}

		// 接着读
		//lpIOContext->IOOperation = ClientIoRead; // 不要和他抢标志位了
		DWORD dwRecvNumBytes = 0; // 没什么用
		DWORD dwFlags = 0;
		WSABUF buffRecv;
		buffRecv.buf = ptr_context_->pIOContext->Buffer,
			buffRecv.len = MAX_BUFF_SIZE;
		int nRet = WSARecv(
			ptr_context_->Socket,
			&buffRecv, 1, &dwRecvNumBytes,
			&dwFlags,
			&ptr_context_->pIOContext->recv_overlapped, NULL);
		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
			qDebug() << "WSARecv() failed: " << WSAGetLastError();
			//CloseClient(lpPerSocketContext, FALSE);
		}
	}
}

void ClientProcessor::SendData(int len, char* buffer)
{
	QMutexLocker locker(&send_mutex_);

	if (ptr_context_ != nullptr)
	{
		bool is_available = true;   // overlapped
		if (is_available)
		{
			// send
		}
		else
		{
			// restore
		}

		// 如果当前正在发送数据   保存数据等待已发数据发送完成
		// restore buffer
	}
}

void ClientProcessor::OnSendDataComplete(DWORD size)
{
	QMutexLocker locker(&send_mutex_);

	if (ptr_context_ != nullptr)
	{
		// ptr_context_->pIOContext->nSentBytes += sentbytes  先搞清楚哪些发送了

		bool exist_data = true; //还有数据未发送  包括缓存的  和 iocp中发送剩余的
		if (exist_data)
		{
			// send and clear
		}
	}
}