#include "r_decals.h"
#include "materials.h"
#include "cvar.h"
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <unordered_map>

void R_Decals::Init()
{
    m_shader.Load("shaders/decal.vert", "shaders/decal.frag");

    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    DecalVertex vertices[] =
    {
        { -0.5f, -0.5f, 0.0f, 0.0f, 0.0f },
        {  0.5f, -0.5f, 0.0f, 1.0f, 0.0f },
        {  0.5f,  0.5f, 0.0f, 1.0f, 1.0f },
        { -0.5f,  0.5f, 0.0f, 0.0f, 1.0f }
    };

    uint16_t indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };

    m_vbo = bgfx::createVertexBuffer(bgfx::copy(vertices, sizeof(vertices)), m_layout);
    m_ebo = bgfx::createIndexBuffer(bgfx::copy(indices, sizeof(indices)));

    m_sDiffuse = bgfx::createUniform("s_diffuse", bgfx::UniformType::Sampler);
    m_sNormal  = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
    m_sMRAO    = bgfx::createUniform("s_mraohMap", bgfx::UniformType::Sampler);
    m_uParams  = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);
    m_uParallaxParams = bgfx::createUniform("u_parallaxParams", bgfx::UniformType::Vec4);
    m_uViewPos = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);
}

void R_Decals::Draw(bgfx::ViewId viewId, const Camera& camera, const Frustum& frustum, const std::vector<std::shared_ptr<Decal>>& decals)
{
    if (decals.empty())
    {
        return;
    }

    std::unordered_map<std::string, std::vector<glm::mat4>> groups;
    for (const auto& d : decals)
    {
        const auto& def = d->GetDef();
        float halfSize = def.size * 0.5f;
        glm::vec3 mins = def.position - glm::vec3(halfSize);
        glm::vec3 maxs = def.position + glm::vec3(halfSize);

        if (frustum.valid && !frustum.IsBoxVisible(mins, maxs))
        {
            continue;
        }

        groups[d->GetDef().textureName].push_back(d->GetModelMatrix());
    }

    float vp[4] = { camera.position.x, camera.position.y, camera.position.z, 0.0f };
    bgfx::setUniform(m_uViewPos, vp);

    float par[4] = 
    {
        (float)CVar::GetFloat("mat_parallax_min_steps", 8.0f),
        (float)CVar::GetFloat("mat_parallax_max_steps", 32.0f),
        (float)CVar::GetInt("mat_parallax_refine", 8),
        (float)CVar::GetInt("mat_parallax", 1)
    };
    bgfx::setUniform(m_uParallaxParams, par);

    uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LESS;

    for (auto& [texName, matrices] : groups)
    {
        uint16_t numInstances = (uint16_t)matrices.size();
        uint16_t instanceStride = 64; 

        if (numInstances == 0)
        {
            continue;
        }

        uint32_t avail = bgfx::getAvailInstanceDataBuffer(numInstances, instanceStride);
        if (avail > 0)
        {
            uint16_t instancesToDraw = std::min(numInstances, (uint16_t)avail);
            bgfx::InstanceDataBuffer idb;
            bgfx::allocInstanceDataBuffer(&idb, instancesToDraw, instanceStride);

            uint8_t* data = idb.data;
            for (size_t i = 0; i < instancesToDraw; ++i)
            {
                std::memcpy(data, glm::value_ptr(matrices[i]), instanceStride);
                data += instanceStride;
            }

            bgfx::setInstanceDataBuffer(&idb, 0, instancesToDraw);

            auto diff = Materials::GetTexture(texName);
            auto norm = Materials::GetNormalMap(texName);
            auto mraoh = Materials::GetMRAOMap(texName);

            bgfx::setTexture(0, m_sDiffuse, diff->GetHandle());
            bgfx::setTexture(1, m_sNormal, norm->GetHandle());
            bgfx::setTexture(2, m_sMRAO, mraoh->GetHandle());

            float params[4] = { Materials::GetHeightScale(texName), 0.0f, 0.0f, 0.0f };
            bgfx::setUniform(m_uParams, params);

            bgfx::setVertexBuffer(0, m_vbo);
            bgfx::setIndexBuffer(m_ebo);
            bgfx::setState(state);
            bgfx::submit(viewId, m_shader.GetProgram());
        }
    }
}

void R_Decals::Shutdown()
{
    if (bgfx::isValid(m_vbo))
    {
        bgfx::destroy(m_vbo);
        m_vbo = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_ebo))
    {
        bgfx::destroy(m_ebo);
        m_ebo = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_sDiffuse))
    {
        bgfx::destroy(m_sDiffuse);
        bgfx::destroy(m_sNormal);
        bgfx::destroy(m_sMRAO);
        bgfx::destroy(m_uParams);
        bgfx::destroy(m_uParallaxParams);
        bgfx::destroy(m_uViewPos);
        m_sDiffuse = BGFX_INVALID_HANDLE;
    }
}