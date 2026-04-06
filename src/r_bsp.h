#pragma once
#include "bsploader.h"
#include "shader.h"
#include "texture.h"
#include <memory>
#include <vector>

struct BSPDrawCall
{
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> specularMap;
    bool isBumped;

    uint32_t start;
    uint32_t count;
};

class R_BSP
{
public:
    R_BSP();
    ~R_BSP();

    bool Init(const BSP::MapData& mapData);
    void Draw(const Shader& shader);
    void Shutdown();

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_lightmapTexture = 0;
    std::vector<BSPDrawCall> m_drawCalls;
};