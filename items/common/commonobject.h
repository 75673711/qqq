#ifndef COMMONOBJECT_H
#define COMMONOBJECT_H

#include <QDebug>

class QOpenGLFunctions_4_4_Compatibility;

enum MeshType {
    MeshLine = 0,
    MeshRectangle,
    MeshGraduation,
    MeshText,
    MeshFastText,
    MeshTypeCount
};

class CommonObject
{
public:
    static CommonObject& GetInstance() {
        static CommonObject obj;
        return obj;
    }

    QOpenGLFunctions_4_4_Compatibility* GetFunc() const {
        return ptr_f_;
    }

    void SetFunc(QOpenGLFunctions_4_4_Compatibility* ptr_f) {
        ptr_f_ = ptr_f;
    }

protected:
    CommonObject() {}
    ~CommonObject() {}

    QOpenGLFunctions_4_4_Compatibility* ptr_f_ = nullptr;
};

#endif // COMMONOBJECT_H
