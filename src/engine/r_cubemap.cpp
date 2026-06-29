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
#include "r_cubemap.h"
#include "cubemap.h"
#include "renderer.h"
#include "camera.h"
#include "entities.h"
#include "filesystem.h"
#include "dds.h"
#include "cvar.h"
#include <glad/glad.h>
#include <vector>
#include <fstream>

namespace R_Cubemap
{
    CVar r_cubemap_res("r_cubemap_resolution", "512", "Resolution of rendered cubemap faces.", CVAR_SAVE);

    uint32_t CreateFromFiles(const std::string& basePath)
    {
        uint32_t id;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, 10, GL_SRGB8_ALPHA8, 512, 512);

        const char* faces[] = { "_right", "_left", "_top", "_bottom", "_front", "_back" };
        for (int i = 0; i < 6; i++)
        {
            DDS::ImageInfo info;
            if (DDS::Load(basePath + faces[i] + ".dds", true, info) && !info.mips.empty())
            {
                glTextureSubImage3D(id, 0, 0, 0, i, info.width, info.height, 1, GL_RGBA, GL_UNSIGNED_BYTE, &info.data[info.mips[0].offset]);
            }
        }

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateTextureMipmap(id);
        return id;
    }

    void BuildProbes(const std::string& mapName, Renderer* renderer)
    {
        auto ents = EntityManager::GetEntities();
        std::vector<std::shared_ptr<Entity>> probes;
        for (auto& e : ents)
        {
            if (e->GetClassName() == "env_cubemap_box")
            {
                probes.push_back(e);
            }
        }

        if (probes.empty())
        {
            return;
        }

        int res = r_cubemap_res.GetInt();
        uint32_t fbo, rbo;
        glCreateFramebuffers(1, &fbo);
        glCreateRenderbuffers(1, &rbo);
        glNamedRenderbufferStorage(rbo, GL_DEPTH24_STENCIL8, res, res);
        glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        std::string dir = "cubemaps/" + mapName + "/";
        Filesystem::CreateDirectory(dir);

        Camera cam(90.0f, 1.0f, 0.1f, 2000.0f);
        const char* faceNames[] = { "right", "left", "top", "bottom", "front", "back" };

        for (size_t i = 0; i < probes.size(); i++)
        {
            uint32_t tex;
            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex);
            glTextureStorage2D(tex, 1, GL_RGB8, res, res);

            btVector3 bmin, bmax;
            probes[i]->GetPhysObject()->getCollisionShape()->getAabb(probes[i]->GetPhysObject()->getWorldTransform(), bmin, bmax);
            glm::vec3 origin = (glm::vec3(bmin.x(), bmin.y(), bmin.z()) + glm::vec3(bmax.x(), bmax.y(), bmax.z())) * 0.5f;
            cam.position = origin;

            for (int j = 0; j < 6; j++)
            {
                cam.yaw = (j == 0) ? 0.0f : (j == 1) ? 180.0f : (j == 4) ? 90.0f : (j == 5) ? -90.0f : -90.0f;
                cam.pitch = (j == 2) ? 90.0f : (j == 3) ? -90.0f : 0.0f;

                glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, tex, 0, j);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                glViewport(0, 0, res, res);
                renderer->RenderWorld(cam, 0);

                std::vector<uint8_t> buffer(res * res * 4);
                glGetTextureSubImage(tex, 0, 0, 0, j, res, res, 1, GL_RGBA, GL_UNSIGNED_BYTE, (GLsizei)buffer.size(), buffer.data());
                DDS::WriteUncompressedRGB(Filesystem::GetFullPath(dir + "cubemap_" + std::to_string(i) + "_" + faceNames[j] + ".dds"), res, res, buffer.data());
            }

            std::ofstream meta(Filesystem::GetFullPath(dir + "cubemap_" + std::to_string(i) + ".txt"));
            meta << origin.x << " " << origin.y << " " << origin.z << "\n";
            meta << bmin.x() << " " << bmin.y() << " " << bmin.z() << "\n";
            meta << bmax.x() << " " << bmax.y() << " " << bmax.z() << "\n";
            
            glDeleteTextures(1, &tex);
        }

        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &rbo);
        Cubemap::LoadForMap(mapName);
    }

    void Release(uint32_t id)
    {
        if (id != 0)
        {
            glDeleteTextures(1, &id);
        }
    }
}