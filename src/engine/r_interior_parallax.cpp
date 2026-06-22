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

void R_InteriorParallax::Init()
{
    m_shader.Load("shaders/interior_parallax.vert", "shaders/interior_parallax.frag");
}

GLuint R_InteriorParallax::GetCubemap(const std::string& name)
{
    if (m_cubemapCache.count(name)) 
        return m_cubemapCache[name];

    GLuint tex;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex);
    glTextureStorage2D(tex, 10, GL_SRGB8_ALPHA8, 512, 512);

    std::vector<std::string> faces = { "right", "left", "top", "bottom", "front", "back" };
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        std::string path = "textures/interiors/" + name + "_" + faces[i] + ".dds";
        if (!DDS::LoadCubemapFace(tex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, path, true))
        {
            uint8_t black[4] = { 0, 0, 0, 255 };
            glTextureSubImage3D(tex, 0, 0, 0, i, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, black);
        }
    }

    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateTextureMipmap(tex);

    m_cubemapCache[name] = tex;
    return tex;
}

void R_InteriorParallax::Draw(const Camera& camera, R_BSP* bsp)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_shader.SetVec3("u_viewPos", camera.position);

    for (const auto& ent : EntityManager::GetEntities())
    {
        if (ent->GetClassName() == "func_interior_parallax" && ent->IsEnabled())
        {
            auto* p = static_cast<FuncInteriorParallax*>(ent.get());
            
            glBindTextureUnit(0, GetCubemap(p->GetCubemapName()));

            m_shader.SetVec3("u_roomSize", p->GetRoomSize());
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
            glm::vec3 ang = ent->GetAngles();
            model = glm::rotate(model, glm::radians(ang.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(ang.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(ang.z), glm::vec3(0, 0, 1));

            bsp->DrawBModel(ent->GetBModelIndex(), m_shader, model, true);
        }
    }
}

void R_InteriorParallax::Shutdown()
{
    for (auto& pair : m_cubemapCache) 
    {
        glDeleteTextures(1, &pair.second);
    }
    m_cubemapCache.clear();
}