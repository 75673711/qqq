#ifndef BASICSCENE_H
#define BASICSCENE_H

#include <list>
#include <vector>

namespace whd3d {

class BasicMesh;

class BasicScene
{
public:
    BasicScene();
    virtual ~BasicScene();

    void AddMesh(BasicMesh* ptr_mesh);

    void UpdateWorld(uint32_t delta_time);

    const std::vector<std::list<BasicMesh*>>& GetMeshes() const {
        return mesh_vec_list_;
    }

private:
    std::vector<std::list<BasicMesh*>> mesh_vec_list_;
};
}

#endif // BASICSCENE_H
