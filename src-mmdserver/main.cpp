#include "server/room/roomserver.h"
#include "common/eventloop.h"

int main()
{
    EventLoop::GetInstance().Run();

    boost::asio::io_context ioc{4};
    std::shared_ptr<mmd::RoomServer> ptr_server =  std::make_shared<mmd::RoomServer>(ioc);
    ptr_server->Listen("0.0.0.0", 8080);

    getchar();

    ptr_server->StopListen();
    ptr_server->Clear();
    EventLoop::GetInstance().Stop();

    return 0;
}
