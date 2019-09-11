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

// todo: ���Է���ͬʱ������  ���ԵĻ�ptr_context_->pIOContext->IOOperation��������ôʶ�������
// Ϊÿһ�������½�һ��overlapped   ����д��һ��  �޸Ľṹ���
// GetQueuedCompletionStatusʱ�᷵��overlappedָ��  ���Ӧ���ǵ����첽����ʱ�����ָ��   ͨ��������ж����ĸ�io���� ��

// todo: ��������ʱΪʲô���з�һ��������   ��ʱӦ�ý�����������ȥ���ط� ���� �Ȱ�ʣ�µķ��ꣿ
void ClientProcessor::OnGetNewData(DWORD size)
{
	QMutexLocker locker(&recv_mutex_);

	if (ptr_context_ != nullptr)
	{
		ptr_context_->pIOContext->Buffer;

		// wsabuf ����  ptr_context_->pIOContext->Buffer  Ϊ�˷��ͽӿڵ��ö���װ��

		// ���Է�������
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

		
		if (size > 0)  // Ϊ0ʱ�����һ�ν����������  ��ʼ������
		{
			// todo:�յ����ݺ�ص���ҵ���
			qDebug() << QByteArray(ptr_context_->pIOContext->wsabuf.buf, size);
		}

		// ���Ŷ�
		//lpIOContext->IOOperation = ClientIoRead; // ��Ҫ��������־λ��
		DWORD dwRecvNumBytes = 0; // ûʲô��
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

		// �����ǰ���ڷ�������   �������ݵȴ��ѷ����ݷ������
		// restore buffer
	}
}

void ClientProcessor::OnSendDataComplete(DWORD size)
{
	QMutexLocker locker(&send_mutex_);

	if (ptr_context_ != nullptr)
	{
		// ptr_context_->pIOContext->nSentBytes += sentbytes  �ȸ������Щ������

		bool exist_data = true; //��������δ����  ���������  �� iocp�з���ʣ���
		if (exist_data)
		{
			// send and clear
		}
	}
}