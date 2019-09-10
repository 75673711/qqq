#include "roomserver.h"

#include "roomcontroller.h"

#include "common/common.h"

#include <boost/bimap.hpp>

namespace mmd
{
RoomServer::RoomServer(boost::asio::io_context &ioc) : WSServer(ioc)
{
    room_ctl_list_.push_back(new RoomController());
}

RoomServer::~RoomServer()
{
}

std::shared_ptr<WSSession> RoomServer::OnNewConnection(boost::asio::ip::tcp::socket socket)
{
    return std::make_shared<RoomSession>(std::move(socket), boost::bind(&WSServer::RemoveSession, shared_from_this(), _1));
}

void RoomServer::OnNewSession(const std::shared_ptr<WSSession> &ptr_session)
{
    WSServer::OnNewSession(ptr_session);

    std::shared_ptr<RoomSession> ptr_room_session = std::dynamic_pointer_cast<RoomSession>(ptr_session);
    if (ptr_room_session)
    {
        UserID user_id = GenerateUserID();
        ptr_room_session->SetUserID(user_id);
        ptr_room_session->SetController(room_ctl_list_.front());
    }
}
} // namespace mmd