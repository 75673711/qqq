
#ifndef IOCPSERVER_H
#define IOCPSERVER_H

#include <WinSock2.h>
#include <mswsock.h>

#include <QObject>

#define DEFAULT_PORT        "5001"
#define MAX_BUFF_SIZE       8192
#define MAX_WORKER_THREAD   16

class ClientProcessor;

typedef enum _IO_OPERATION {
	ClientIoAccept,
	ClientIoRead,
	ClientIoWrite
} IO_OPERATION, *PIO_OPERATION;

//
// data to be associated for every I/O operation on a socket
// 不可修改
typedef struct _PER_IO_CONTEXT {

	WSAOVERLAPPED               Overlapped;
	WSAOVERLAPPED               recv_overlapped;     // 接收标志     
	char                        Buffer[MAX_BUFF_SIZE];
	WSABUF                      wsabuf;          // 数据读取  待定
	int                         nTotalBytes;
	int                         nSentBytes;
	IO_OPERATION                IOOperation;
	SOCKET                      SocketAccept;

	struct _PER_IO_CONTEXT      *pIOContextForward;

	ClientProcessor* ptr_client_ = nullptr;


} PER_IO_CONTEXT, *PPER_IO_CONTEXT;

//
// For AcceptEx, the IOCP key is the PER_SOCKET_CONTEXT for the listening socket,
// so we need to another field SocketAccept in PER_IO_CONTEXT. When the outstanding
// AcceptEx completes, this field is our connection socket handle.
//

//
// data to be associated with every socket added to the IOCP
//
typedef struct _PER_SOCKET_CONTEXT {
	SOCKET                      Socket;

	LPFN_ACCEPTEX               fnAcceptEx;

	//
	//linked list for all outstanding i/o on the socket
	//
	PPER_IO_CONTEXT             pIOContext;
	struct _PER_SOCKET_CONTEXT  *pCtxtBack;
	struct _PER_SOCKET_CONTEXT  *pCtxtForward;
} PER_SOCKET_CONTEXT, *PPER_SOCKET_CONTEXT;

BOOL ValidOptions(int argc, char *argv[]);

BOOL WINAPI CtrlHandler(
	DWORD dwEvent
);

BOOL CreateListenSocket(void);

BOOL CreateAcceptSocket(
	BOOL fUpdateIOCP
);

DWORD WINAPI WorkerThread(
	LPVOID WorkContext
);

PPER_SOCKET_CONTEXT UpdateCompletionPort(
	SOCKET s,
	IO_OPERATION ClientIo,
	BOOL bAddToList
);
//
// bAddToList is FALSE for listening socket, and TRUE for connection sockets.
// As we maintain the context for listening socket in a global structure, we
// don't need to add it to the list.
//

VOID CloseClient(
	PPER_SOCKET_CONTEXT lpPerSocketContext,
	BOOL bGraceful
);

PPER_SOCKET_CONTEXT CtxtAllocate(
	SOCKET s,
	IO_OPERATION ClientIO
);

VOID CtxtListFree(
);

VOID CtxtListAddTo(
	PPER_SOCKET_CONTEXT lpPerSocketContext
);

VOID CtxtListDeleteFrom(
	PPER_SOCKET_CONTEXT lpPerSocketContext
);

class IOCPManager : public QObject 
{
	Q_OBJECT
public:
	IOCPManager();
	~IOCPManager();

	static IOCPManager* Instance();

	bool Init();
	void Uninit();
};

#endif