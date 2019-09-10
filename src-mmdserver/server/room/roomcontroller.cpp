#include "roomcontroller.h"

#include "roomsession.h"

#include "objects/charactor/player.h"

#include <thread>
#include <iostream>

namespace mmd
{

RoomController::RoomController()
{
    CreateRoom();
}

RoomController::~RoomController()
{
    DestroyRoom();
}

bool RoomController::JoinRoom(const UserID &user_id, const std::shared_ptr<RoomSession> &ptr_session)
{
    std::cout << "JoinRoom " << user_id << std::endl;
    ptr_session->SetController(this);
    //todo:为什么这里要停一下WEB菜收的到消息
    mmdtranfer::UserTrend body;
    body.set_userid(user_id);
    body.set_trendtype(mmdtranfer::UserTrend::Join);

    size_t byte_size = body.ByteSizeLong(); 
    std::string body_str("", byte_size);      // 出参
    body.SerializeToArray((void *)(body_str.c_str()), byte_size);

    std::string data_str = PacketMessage(mmdtranfer::MessageHeader::UserTrend, body_str);

    // temp------指明是操作还是观看
    {
        READ_LOCK(map_mutex_);
        if (session_map_.begin() == session_map_.end())
        {
            control_userid_ = user_id;
            body.set_trendtype(mmdtranfer::UserTrend::Control);
        }
        else
        {
            body.set_trendtype(mmdtranfer::UserTrend::Watch);
        }
        byte_size = body.ByteSizeLong(); 
        std::string body2_str("", byte_size);      // 出参
        body.SerializeToArray((void *)(body2_str.c_str()), byte_size);
        std::string data2_str = PacketMessage(mmdtranfer::MessageHeader::UserTrend, body2_str);
        ptr_session->WriteData(data2_str);
    }
    
    // ----------------------------

    {
        WRITE_LOCK(map_mutex_);
        auto it = session_map_.begin();
        while (it != session_map_.end())
        {
            it->second->WriteData(data_str);
            ++it;
        }

        session_map_[user_id] = ptr_session; //todo: 改回去
    }

    BroasdcastPlayerState(control_userid_, true);

    return true;
}

void RoomController::LeaveRoom(const UserID &user_id)
{
    std::cout << "LeaveRoom " << user_id << std::endl;
    mmdtranfer::UserTrend body;
    body.set_userid(user_id);
    body.set_trendtype(mmdtranfer::UserTrend::Leave);

    size_t byte_size = body.ByteSizeLong();
    std::string body_str("", byte_size);
    body.SerializeToArray((void *)(body_str.c_str()), byte_size);

    std::string data_str = PacketMessage(mmdtranfer::MessageHeader::UserTrend, body_str);
    // todo: send leave message
    {
        READ_LOCK(map_mutex_);
        for (const auto &it : session_map_)
        {
            if (it.first == user_id)
            {
                continue;
            }

            it.second->WriteData(data_str);
        }
    }

    {
        WRITE_LOCK(map_mutex_);
        auto it = session_map_.find(user_id);
        if (it != session_map_.end())
        {
            it->second->SetController(nullptr);
            session_map_.erase(it);
        }
    }
}

void RoomController::CreateRoom()
{
}

void RoomController::DestroyRoom()
{
}

void RoomController::BroasdcastPlayerState(const UserID &user_id, bool exclude_self)
{
    if (user_id != control_userid_)
    {
        return;
    }

    READ_LOCK(map_mutex_);

    auto iter = session_map_.find(user_id);
    if (iter == session_map_.end())
    {
        return;
    }

    std::string state_str = iter->second->SessionPlayer()->PlayerStateToStr(user_id);
    std::string data_str = PacketMessage(mmdtranfer::MessageHeader::PlayerState, state_str);

    for (const auto &it : session_map_)
    {
        if (it.first == user_id && exclude_self)
        {
            continue;
        }

        it.second->WriteData(data_str);
        std::cout << "send player state to " << it.first << std::endl;
    }
}

void RoomController::OnSendMessageFailed(const UserID &user_id)
{
    std::cout << "send message error user id : " << user_id << std::endl;
}

} // namespace mmd