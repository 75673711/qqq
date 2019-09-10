#include "common.h"

#include <atomic>
#include <arpa/inet.h>

namespace mmd
{
UserID GenerateUserID()
{
    static std::atomic<UserID> g_user_id;
    return ++g_user_id;
}
UserID GenerateRoomID()
{
    static std::atomic<RoomID> g_room_id;
    return ++g_room_id;
}

uint32_t GenerateMessageID()
{
    static std::atomic<uint32_t> i;
    return ++i;
}

std::string PacketMessage(mmdtranfer::MessageHeader::ContentType content_type, const std::string& body_str)
{
    mmdtranfer::MessageHeader header;
    header.set_version(mmdtranfer::MessageHeader::VERSION1X);
    header.set_messageid(GenerateMessageID());
    header.set_contenttype(content_type);
    header.set_bodysize(body_str.size());
    
    size_t byte_size = header.ByteSizeLong();
    std::string header_str("", byte_size);
    header.SerializeToArray((void*)(header_str.c_str()), byte_size);

    //uint32_t len = htonl(header_str.size() + body_str.size());
    uint32_t len = htonl(header_str.size());
    return std::string((char*)(&len), sizeof(uint32_t)) + header_str + body_str;
}

}
