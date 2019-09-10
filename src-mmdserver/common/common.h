#pragma once

#include "utils/mmd.pb.h"

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <string>
#include <iostream>

template<typename T>
inline T FourBytesToT(const char* ptr_data)
{
    return (T)((unsigned char)(*ptr_data) << 24 |
            (unsigned char)(*(ptr_data + 1)) << 16 |
            (unsigned char)(*(ptr_data + 2)) << 8 |
            (unsigned char)(*(ptr_data + 3)));
}

namespace mmd
{
typedef boost::shared_mutex WR_Mutex;
typedef boost::unique_lock< WR_Mutex > WRITE_LOCK;
typedef boost::shared_lock< WR_Mutex > READ_LOCK;

typedef uint64_t UserID;
typedef uint64_t RoomID;

// todo: 目前没有生成ID没有过滤非法值
const UserID kInvalidUserID = 0;
const UserID kInvalidRoomID = 0;

UserID GenerateUserID();
UserID GenerateRoomID();
uint32_t GenerateMessageID();

std::string PacketMessage(mmdtranfer::MessageHeader::ContentType content_type, const std::string& body_str);

class DebugInfo
{
public:
    DebugInfo();
    ~DebugInfo()
    {
        std::cout << temp.c_str() << std::endl;
    }

    void operator<<(const int& other)
    {
        temp += std::to_string(other);
    }
    void operator<<(const std::string& other)
    {
        temp += other;
    }

    std::string temp;
};
} // namespace mmd
