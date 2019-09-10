#pragma once

#include "../session.h"

#include "common/common.h"

namespace mmd
{
class RoomController;
class Player;

class RoomSession : public WSSession
{
public:
    explicit RoomSession(boost::asio::ip::tcp::socket &&socket, REMOVESESSIONHANDLE&& remove_handler);
    ~RoomSession() override;

    void SetController(RoomController* ptr_ctl);
    void SetUserID(UserID user_id);

    void OnCanWrite() override;
    void OnCloseComplete() override;
    void OnReadData(const char *ptr_data, std::size_t len) override;

    Player* SessionPlayer() const {
        return ptr_player_;
    }

private:
    UserID user_id_;
    RoomController* ptr_ctl_;
    Player* ptr_player_;

    std::string message_buffer_;
};
} // namespace mmd