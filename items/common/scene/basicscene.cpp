#include "basicscene.h"

#include "items/common/mesh/basicmesh.h"

namespace whd3d {

BasicScene::BasicScene()
{
    mesh_vec_list_.resize(MeshType::MeshTypeCount);
}

BasicScene::~BasicScene()
{

}

void BasicScene::AddMesh(BasicMesh* ptr_mesh)
{
    mesh_vec_list_[ptr_mesh->GetType()].push_back(ptr_mesh);
}

void BasicScene::UpdateWorld(uint32_t delta_time)
{

}


}
