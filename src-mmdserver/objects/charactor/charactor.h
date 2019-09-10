#pragma once

#include "vmmlib/vmmlib.hpp"

namespace mmd
{

class Charactor
{
public:
    explicit Charactor();
    virtual ~Charactor();

    const vmml::vector<3, float>& Position() const {
        return position_;
    }
    const vmml::vector<3, float>& Rotate() const {
        return rotate_;
    }
    const vmml::vector<3, float>& Scale() const {
        return scale_;
    }

    void SetPosition(float x, float y, float z) {
        position_.set(x, y, y);
    }
    void SetRoate(float x, float y, float z) {
        rotate_.set(x, y, z);
    }
    void SetScale(float x, float y, float z) {
        scale_.set(x, y, z);
    }

private:
    vmml::vector<3, float> position_ = {0, 0, 0}; 
    vmml::vector<3, float> rotate_ = {0, 0, 0};   // 绕x,y,z轴的角度
    vmml::vector<3, float> scale_ = {1, 1, 1};
};
} // namespace mmd
