#pragma once
#include "r_shader.h"
#include "camera.h"
#include "decals.h"
#include <bgfx/bgfx.h>
#include <memory>
#include <vector>

struct DecalVertex
{
    float x, y, z;
    float u, v;
};

class R_Decals
{
public:
    void Init();
    void Draw(bgfx::ViewId viewId, const Camera& camera, const Frustum& frustum, const std::vector<std::shared_ptr<Decal>>& decals);
    void Shutdown();

private:
    bgfx::VertexLayout m_layout;
    bgfx::VertexBufferHandle m_vbo = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_ebo = BGFX_INVALID_HANDLE;
    R_Shader m_shader;

    bgfx::UniformHandle m_sDiffuse = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sNormal = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sMRAO = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uParallaxParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uViewPos = BGFX_INVALID_HANDLE;
};