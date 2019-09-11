
#ifndef IOCPCLIENT_H
#define IOCPCLIENT_H

#include <QObject>
#include <QMutex>

#include "IOCPManager.h"

class ClientProcessor : public QObject 
{
	Q_OBJECT
public:
	ClientProcessor();
	~ClientProcessor();

	bool Init(PER_SOCKET_CONTEXT* ptr_context);
	void Uninit();

	void SendData(int len, char* buffer);

	void OnGetNewData(DWORD size);
	void OnSendDataComplete(DWORD size);

private:
	PER_SOCKET_CONTEXT* ptr_context_ = nullptr;

	QMutex recv_mutex_;
	QMutex send_mutex_; // todo: ��ͬ�߳�ͬʱ����send�᲻�ᵼ��sendbuffer�����Ƕ�����  û��ϵͳsendbuffer�������

	char* parse_buffer_ = nullptr;
	int parse_buffer_len_ = 0;
};

#endif