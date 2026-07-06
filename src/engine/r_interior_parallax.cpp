/*
 * MIT License
 *
 * Copyright (c) 2025-2026 Soft Sprint Studios
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "r_interior_parallax.h"
#include "r_bsp.h"
#include "dds.h"
#include "entities.h"
#include "func_interior_parallax.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void R_InteriorParallax::Init()
{
    m_shader.Load("shaders/interior_parallax.vert", "shaders/interior_parallax.frag");
    m_sInteriorCube = bgfx::createUniform("s_interiorCube", bgfx::UniformType::Sampler);
    m_uRoomParams = bgfx::createUniform("u_roomParams", bgfx::UniformType::Vec4);
    m_uViewPos = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);
}

bgfx::TextureHandle R_InteriorParallax::GetCubemap(const std::string& name)
{
    if (m_cubemapCache.count(name)) 
        return m_cubemapCache[name];

    std::vector<std::string> faces = { "right", "left", "top", "bottom", "front", "back" };
    std::vector<uint8_t> allData;
    uint32_t width = 512, height = 512;
    bool first = true;

    for (unsigned int i = 0; i < 6; i++)
    {
        std::string path = "textures/interiors/" + name + "_" + faces[i] + ".dds";

        DDS::ImageInfo info;
        if (DDS::Load(path, true, info) && !info.mips.empty())
        {
            if (first) 
            { 
                width = info.width; 
                height = info.height; 
                first = false; 
            }
            allData.insert(allData.end(), info.data.begin() + info.mips[0].offset, info.data.begin() + info.mips[0].offset + info.mips[0].size);
        }
        else
        {
            std::vector<uint8_t> black(width * height * 4, 0);
            allData.insert(allData.end(), black.begin(), black.end());
        }
    }

    const bgfx::Memory* mem = bgfx::copy(allData.data(), (uint32_t)allData.size());
    bgfx::TextureHandle tex = bgfx::createTextureCube((uint16_t)width, false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP, mem);
    m_cubemapCache[name] = tex;
    return tex;
}

void R_InteriorParallax::Draw(bgfx::ViewId viewId, const Camera& camera, R_BSP* bsp)
{
    float vp[4] = { camera.position.x, camera.position.y, camera.position.z, 0.0f };

    for (const auto& ent : EntityManager::GetEntities())
    {
        if (ent->GetClassName() == "func_interior_parallax" && ent->IsEnabled())
        {
            auto* p = static_cast<FuncInteriorParallax*>(ent.get());
            
            bgfx::TextureHandle cubemap = GetCubemap(p->GetCubemapName());
            bgfx::setTexture(0, m_sInteriorCube, cubemap);

            bgfx::setUniform(m_uViewPos, vp);

            glm::vec3 roomSize = p->GetRoomSize();
            float params[4] = { roomSize.x, roomSize.y, roomSize.z, 0.0f };
            bgfx::setUniform(m_uRoomParams, params);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
            glm::vec3 ang = ent->GetAngles();
            model = glm::rotate(model, glm::radians(ang.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(ang.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(ang.z), glm::vec3(0, 0, 1));

            bsp->DrawBModel(ent->GetBModelIndex(), m_shader, viewId, model, camera.position, true);
        }
    }
}

void R_InteriorParallax::Shutdown()
{
    for (auto& pair : m_cubemapCache) 
    {
        if (bgfx::isValid(pair.second)) 
            bgfx::destroy(pair.second);
    }
    m_cubemapCache.clear();

    if (bgfx::isValid(m_sInteriorCube))
    {
        bgfx::destroy(m_sInteriorCube);
        bgfx::destroy(m_uRoomParams);
        bgfx::destroy(m_uViewPos);
        m_sInteriorCube = m_uRoomParams = m_uViewPos = BGFX_INVALID_HANDLE;
    }
}