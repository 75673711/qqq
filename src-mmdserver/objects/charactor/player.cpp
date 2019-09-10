#include "player.h"

#include "utils/mmd.pb.h"

namespace mmd
{
Player::Player() : Charactor()
{
}

Player::~Player()
{
}

std::string Player::PlayerStateToStr(const UserID& user_id)
{
    if (!temp_.empty())
    {
        return temp_;
    }

    mmdtranfer::PlayerState state;
    state.set_userid(user_id);
    state.set_motion(motion_);
    state.set_speed(speed_);
    state.set_positionx(Position().x());
    state.set_positiony(Position().y());
    state.set_positionz(Position().z());
    state.set_rotatex(Rotate().x());
    state.set_rotatey(Rotate().y());
    state.set_rotatez(Rotate().z());
    state.set_scalex(Scale().x());
    state.set_scaley(Scale().y());
    state.set_scalez(Scale().z());

    size_t byte_size = state.ByteSizeLong(); 
    std::string state_str("", byte_size);
    state.SerializeToArray((void*)(state_str.c_str()), byte_size);

    return state_str;
}

void Player::SetPlayerStateStr(const std::string &state_str)
{
    temp_ = state_str;

    mmdtranfer::PlayerState state;
    state.ParseFromArray(state_str.c_str(), state_str.size());

    SetSpeed(state.speed());
    SetMotion(state.motion());
    SetPosition(state.positionx(), state.positiony(), state.positionz());
    SetRoate(state.rotatex(), state.rotatey(), state.rotatez());
    SetScale(state.scalex(), state.scaley(), state.scalez());
}
} // namespace mmd
