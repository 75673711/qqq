#include "roomsession.h"

#include "roomcontroller.h"
#include "common/eventloop.h"
#include "objects/charactor/player.h"

#include <arpa/inet.h>
#include <iostream>

namespace mmd
{
RoomSession::RoomSession(boost::asio::ip::tcp::socket &&socket, REMOVESESSIONHANDLE &&remove_handler) : WSSession(std::move(socket), std::move(remove_handler)),
                                                                                                        user_id_(kInvalidUserID),
                                                                                                        ptr_ctl_(nullptr),
                                                                                                        ptr_player_(new Player)
{
}

RoomSession::~RoomSession()
{
    if (ptr_player_ != nullptr)
    {
        delete ptr_player_;
        ptr_player_ = nullptr;
    }
}

void RoomSession::SetController(RoomController *ptr_ctl)
{
    ptr_ctl_ = ptr_ctl;
}

void RoomSession::SetUserID(UserID user_id)
{
    user_id_ = user_id;
}

void RoomSession::OnCanWrite()
{
    if (ptr_ctl_ == nullptr)
    {
        sleep(1);
    }

    if (ptr_ctl_->JoinRoom(user_id_, std::dynamic_pointer_cast<RoomSession>(shared_from_this())))
    {
        //std::cout << "join room success: " << user_id << std::endl;
    }
    else
    {
        //DebugInfo() << "join room failed: " << GenerateUserID();
    }
}

void RoomSession::OnCloseComplete()
{
    ptr_ctl_->LeaveRoom(user_id_);
    WSSession::OnCloseComplete();
}

void RoomSession::OnReadData(const char *ptr_data, std::size_t len)
{
    std::cout << "OnReadData  " << len << std::endl;
    if (len < sizeof(uint32_t))
    {
        // unknow message
        return;
    }

    message_buffer_ += std::string(ptr_data, len);

    while (message_buffer_.length() > 4)
    {
        // uint32_t  get header size
        uint32_t header_len = FourBytesToT<uint32_t>(message_buffer_.substr(0, 4).c_str());
        std::cout << "x33get header len  " << header_len << std::endl;
        if (header_len > message_buffer_.length())
        {
            // 包头没到
            return;
        }

        std::string str_header = message_buffer_.substr(4, header_len);

        mmdtranfer::MessageHeader header;
        if (!header.ParsePartialFromArray(str_header.c_str(), header_len))
        {
            // 解析失败  清空
            message_buffer_ = std::string();
            return;
        }

        if (header.bodysize() + 4 + header_len > message_buffer_.length())
        {
            // 包体没到
            return;
        }

        // 解析body
        // mmdtranfer::PlayerState one_state;
        // if (!one_state.ParseFromString(message_buffer_.substr(4 + header_len, header.bodysize())))
        // {
        //     // 解析失败  清空
        //     message_buffer_ = std::string();
        //     return;
        // }

        std::string body_str = message_buffer_.substr(4 + header_len, header.bodysize());
        message_buffer_ = message_buffer_.substr(4 + header_len + header.bodysize());

        switch (header.contenttype())
        {
        case mmdtranfer::MessageHeader::PlayerState:
            ptr_player_->SetPlayerStateStr(body_str);
            ptr_ctl_->BroasdcastPlayerState(user_id_);
            break;
        default:
            std::cout << "cant handle it" << std::endl;
            break;
        }
    }
}

} // namespace mmd

// uint32_t temp = 0;
// for (int i = 3; i >= 0; --i)
// {
//     uint8_t one = *(ptr_data + (3 - i));

//     std::cout << static_cast<int>(one & 0x000000ff) << " ";

//     temp |= (one << i * 8);
// }