#pragma once

#include "charactor.h"

#include "common/common.h"

namespace mmd
{
class Player : public Charactor
{
public:
    enum MoveState {
        Stand,
        Walk,
        Run
    };

public:
    explicit Player();
    virtual ~Player();

    void SetSpeed(float speed) {
        speed_ = speed;
    }
    void SetMotion(const std::string& motion) {
        motion_ = motion;
    }

    std::string PlayerStateToStr(const UserID& user_id);
    void SetPlayerStateStr(const std::string& state_str);

private:
    float speed_ = 1;
    std::string motion_ = "basic";

    std::string temp_;
};
} // namespace mmd
