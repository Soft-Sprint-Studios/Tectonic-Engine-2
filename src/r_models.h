#pragma once
#include "bsploader.h"
#include "shader.h"
#include "texture.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

struct ModelMesh
{
    uint32_t vao;
    std::vector<uint32_t> vbos;
    uint32_t ebo;
    uint32_t indexCount;
    uint32_t indexType;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> specularMap;
    uint32_t vertexOffset;
};

struct PropInstanceData
{
    glm::mat4 transform;
    uint32_t colorVbo[3] = { 0, 0, 0 };
    bool isBumped = false;
};

class R_Models
{
public:
    R_Models();
    ~R_Models();

    bool Init(const BSP::MapData& mapData);
    void Draw(const Shader& shader);
    void Shutdown();

private:
    struct PropGroup
    {
        std::vector<ModelMesh> meshes;
        std::vector<PropInstanceData> instances;
        btCollisionShape* physicsShape = nullptr;
    };

    std::unordered_map<std::string, PropGroup> m_propGroups;

    void LoadModel(const std::string& path);
};