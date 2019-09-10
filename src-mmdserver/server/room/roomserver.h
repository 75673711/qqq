#pragma once

#include "../server.h"

#include <list>

namespace mmd
{
class RoomController;

class RoomServer : public WSServer
{
public:
    explicit RoomServer(boost::asio::io_context &ioc);
    ~RoomServer() override;

    std::shared_ptr<WSSession> OnNewConnection(boost::asio::ip::tcp::socket socket) override;
    void OnNewSession(const std::shared_ptr<WSSession> &ptr_session) override;

private:
    std::list<RoomController*> room_ctl_list_;

};
} // namespace mmd