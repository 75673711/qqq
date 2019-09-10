#pragma once

#include "roomsession.h"
#include "../../common/common.h"

#include <map>
#include <memory>
#include <mutex>

namespace mmd
{
class RoomSession;

class RoomController
{
public:
    explicit RoomController();
    ~RoomController();

    bool JoinRoom(const UserID &user_id, const std::shared_ptr<RoomSession> &ptr_session);
    void LeaveRoom(const UserID &user_id);

    void CreateRoom();
    void DestroyRoom();

    void BroasdcastPlayerState(const UserID &user_id, bool exclude_self = true);

protected:
    void OnSendMessageFailed(const UserID &user_id);

private:
    WR_Mutex map_mutex_;
    std::map<UserID, std::shared_ptr<RoomSession>> session_map_;

    UserID control_userid_;
};
} // namespace mmd