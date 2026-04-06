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
#include "bsploader.h"
#include "filesystem.h"
#include "console.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <map>

namespace BSP
{
    // Reference for BSP loader was taken from https://developer.valvesoftware.com/wiki/BSP_(Source)
    // no copyright infringment intended
    class MapLoader
    {
    public:
        MapLoader(const std::vector<uint8_t>& buffer)
            : m_buffer(buffer)
        {
            m_header = (const Header*)m_buffer.data();
        }

        MapData Process()
        {
            if (!ValidateHeader())
            {
                return m_map;
            }

            SetupLumpPointers();
            SetupLightmapAtlas();
            ParseEntities();
            GenerateGeometry();
            ProcessGameLumps();
            ProcessPakFile();

            m_map.loaded = true;
            return m_map;
        }

    private:
        const std::vector<uint8_t>& m_buffer;
        const Header* m_header;
        MapData m_map;

        // Lump Pointers
        const glm::vec3* d_verts = nullptr;
        const Face* d_faces = nullptr;
        const Plane* d_planes = nullptr;
        const Edge* d_edges = nullptr;
        const int32_t* d_surfedges = nullptr;
        const TexInfo* d_texinfos = nullptr;
        const TexData* d_texdatas = nullptr;
        const int32_t* d_stringtable = nullptr;
        const char* d_stringdata = nullptr;
        const char* d_entities = nullptr;
        const DispInfo* d_dispinfos = nullptr;
        const DispVert* d_dispverts = nullptr;
        const uint8_t* d_lighting = nullptr;
        int m_lightingLength = 0;

        // Atlas Packing State
        int m_atlasX = 1;
        int m_atlasY = 0;
        int m_rowHeight = 1;

        bool ValidateHeader()
        {
            if (m_header->ident != VBSP_IDENT)
            {
                Console::Error("BSP: Invalid identifier");
                return false;
            }
            if (m_header->version < VBSP_VERSION_MIN)
            {
                Console::Error("BSP: Version too old");
                return false;
            }
            return true;
        }

        template<typename T>
        T* GetLump(LumpType type)
        {
            return (T*)(m_buffer.data() + m_header->lumps[type].offset);
        }

        void SetupLumpPointers()
        {
            d_verts = GetLump<glm::vec3>(LUMP_VERTEXES);
            d_faces = GetLump<Face>(LUMP_FACES);
            d_planes = GetLump<Plane>(LUMP_PLANES);
            d_edges = GetLump<Edge>(LUMP_EDGES);
            d_surfedges = GetLump<int32_t>(LUMP_SURFEDGES);
            d_texinfos = GetLump<TexInfo>(LUMP_TEXINFO);
            d_texdatas = GetLump<TexData>(LUMP_TEXDATA);
            d_stringtable = GetLump<int32_t>(LUMP_TEXDATA_STRING_TABLE);
            d_stringdata = GetLump<char>(LUMP_TEXDATA_STRING_DATA);
            d_entities = GetLump<char>(LUMP_ENTITIES);
            d_dispinfos = GetLump<DispInfo>(LUMP_DISPINFO);
            d_dispverts = GetLump<DispVert>(LUMP_DISP_VERTS);
        }

        void SetupLightmapAtlas()
        {
            LumpType lightingType = LUMP_LIGHTING;
            if (m_header->lumps[LUMP_LIGHTING_HDR].length > 0)
            {
                lightingType = LUMP_LIGHTING_HDR;
            }

            d_lighting = GetLump<uint8_t>(lightingType);
            m_lightingLength = m_header->lumps[lightingType].length;

            m_map.lightmapAtlasWidth = 4096;
            m_map.lightmapAtlasHeight = 4096;
            m_map.lightmapAtlas.assign(m_map.lightmapAtlasWidth * m_map.lightmapAtlasHeight * 3, 1.0f);
        }

        void ParseEntities()
        {
            std::stringstream ss(d_entities);
            std::string line;

            while (std::getline(ss, line))
            {
                if (line.find('{') != std::string::npos)
                {
                    EntityData ent;
                    while (std::getline(ss, line) && line.find('}') == std::string::npos)
                    {
                        size_t kStart = line.find('\"') + 1;
                        size_t kEnd = line.find('\"', kStart);
                        size_t vStart = line.find('\"', kEnd + 1) + 1;
                        size_t vEnd = line.find('\"', vStart);

                        if (kStart != std::string::npos && vStart != std::string::npos)
                        {
                            std::string key = line.substr(kStart, kEnd - kStart);
                            std::string val = line.substr(vStart, vEnd - vStart);
                            ent.keyvalues[key] = val;
                            if (key == "classname") ent.className = val;
                            if (key == "skyname") m_map.skyName = val;
                        }
                    }
                    if (!ent.className.empty()) m_map.entities.push_back(ent);
                }
            }
        }

        void GenerateGeometry()
        {
            // Initialize collision vertex pool
            int numVerts = m_header->lumps[LUMP_VERTEXES].length / sizeof(glm::vec3);
            m_map.collision.vertices.resize(numVerts);
            for (int i = 0; i < numVerts; ++i)
            {
                const auto& v = d_verts[i];
                // Z-up to Y-up and 1/32 scale conversion
                m_map.collision.vertices[i] = glm::vec3(v.x, v.z, -v.y) * 0.03125f;
            }

            // Batch faces by texture
            int numFaces = m_header->lumps[LUMP_FACES].length / sizeof(Face);
            std::unordered_map<std::string, std::vector<int>> textureBatches;

            for (int i = 0; i < numFaces; i++)
            {
                const Face& face = d_faces[i];
                const TexInfo& tex = d_texinfos[face.texinfo];
                if (tex.flags & 0x0080 || tex.flags & 0x0004 || tex.flags & 0x0002)
                    continue; // Skip nodraw/triggers

                const TexData& td = d_texdatas[tex.texdata];
                std::string name = d_stringdata + d_stringtable[td.nameStringTableID];
                textureBatches[name].push_back(i);
            }

            // Process each batch
            for (auto const& [texName, faceIndices] : textureBatches)
            {
                DrawCall dc;
                dc.textureName = texName;
                dc.start = (uint32_t)m_map.renderVertices.size();

                // Determine if this entire batch is bumped based on the first face
                const Face& firstFace = d_faces[faceIndices[0]];
                const TexInfo& firstTex = d_texinfos[firstFace.texinfo];
                dc.isBumped = (firstTex.flags & 0x0800) != 0;

                for (int faceIdx : faceIndices)
                {
                    ProcessFace(faceIdx);
                }

                dc.count = (uint32_t)m_map.renderVertices.size() - dc.start;
                if (dc.count > 0) m_map.drawCalls.push_back(dc);
            }
        }

        void ProcessGameLumps()
        {
            if (m_header->lumps[LUMP_GAMELUMP].length <= 0)
            {
                return;
            }

            const uint8_t* basePtr = m_buffer.data() + m_header->lumps[LUMP_GAMELUMP].offset;
            const GameLumpHeader* glHeader = (const GameLumpHeader*)basePtr;
            const GameLump* entries = (const GameLump*)(basePtr + sizeof(int32_t));

            for (int i = 0; i < glHeader->lumpCount; i++)
            {
                if (entries[i].id == SPRP_IDENT)
                {
                    const uint8_t* sprpPtr = m_buffer.data() + entries[i].fileofs;

                    int32_t dictCount = *(int32_t*)sprpPtr;
                    sprpPtr += 4;

                    std::vector<std::string> modelPaths;
                    for (int j = 0; j < dictCount; j++)
                    {
                        char name[128];
                        memcpy(name, sprpPtr, 128);
                        std::string path = name;

                        // want glb not mdl
                        size_t dot = path.find_last_of(".");
                        if (dot != std::string::npos)
                        {
                            path = path.substr(0, dot) + ".glb";
                        }

                        modelPaths.push_back(path);
                        sprpPtr += 128;
                    }

                    int32_t leafCount = *(int32_t*)sprpPtr;
                    sprpPtr += 4 + (leafCount * 2);

                    int32_t propCount = *(int32_t*)sprpPtr;
                    sprpPtr += 4;

                    // Stride depends on the sprp version
                    int stride = 0;
                    switch (entries[i].version)
                    {
                    case 4:  stride = 56; break;
                    case 5:  stride = 60; break;
                    case 6:  stride = 72; break;
                    case 10: stride = 76; break;
                    case 11: stride = 80; break;
                    default: stride = 56; break;
                    }

                    for (int j = 0; j < propCount; j++)
                    {
                        glm::vec3* origin = (glm::vec3*)sprpPtr;
                        glm::vec3* angles = (glm::vec3*)(sprpPtr + 12);
                        uint16_t propType = *(uint16_t*)(sprpPtr + 24);

                        if (propType < modelPaths.size())
                        {
                            StaticPropInstance inst;
                            inst.modelPath = modelPaths[propType];
                            inst.position = glm::vec3(origin->x, origin->z, -origin->y) * 0.03125f;
                            inst.angles = *angles;

                            m_map.staticProps.push_back(inst);
                        }
                        sprpPtr += stride;
                    }
                }
            }
        }

        void ProcessPakFile()
        {
            if (m_header->lumps[LUMP_PAKFILE].length <= 0)
            {
                return;
            }
            const uint8_t* pakData = m_buffer.data() + m_header->lumps[LUMP_PAKFILE].offset;
            int pakLength = m_header->lumps[LUMP_PAKFILE].length;

            // vertex lighting is stored in zip
            std::map<std::string, const uint8_t*> vhvFiles;
            int offset = 0;
            while (offset + 30 <= pakLength)
            {
                if (*(uint32_t*)(pakData + offset) != ZIP_IDENT)
                    break;

                uint16_t nameLen = *(uint16_t*)(pakData + offset + 26);
                uint16_t extraLen = *(uint16_t*)(pakData + offset + 28);
                uint32_t compSize = *(uint32_t*)(pakData + offset + 18);

                std::string path((const char*)(pakData + offset + 30), nameLen);
                std::string name = path.substr(path.find_last_of("/\\") + 1);

                // Collect both standard VHV and our custom VHD files
                if (*(uint16_t*)(pakData + offset + 8) == 0 &&
                    (name.find(".vhv") != std::string::npos || name.find(".vhd") != std::string::npos))
                {
                    vhvFiles[name] = pakData + offset + 30 + nameLen + extraLen;
                }
                offset += 30 + nameLen + extraLen + compSize;
            }

            constexpr int VHV_FILE_HEADER_SIZE = 40;
            constexpr int VHV_MESH_HEADER_SIZE = 28;

            for (size_t i = 0; i < m_map.staticProps.size(); i++)
            {
                std::string vhvName = "sp_hdr_" + std::to_string(i) + ".vhv";
                if (vhvFiles.find(vhvName) == vhvFiles.end()) vhvName = "sp_" + std::to_string(i) + ".vhv";

                if (vhvFiles.count(vhvName))
                {
                    const uint8_t* vhvData = vhvFiles[vhvName];
                    int32_t totalVerts = *(int32_t*)(vhvData + 16);
                    int32_t numMeshes = *(int32_t*)(vhvData + 20);

                    if (*(int32_t*)vhvData != 2)
                        continue;

                    m_map.staticProps[i].vertexColors[0].assign(totalVerts, glm::vec3(1.0f));
                    uint32_t colorIdx = 0;
                    for (int m = 0; m < numMeshes; m++)
                    {
                        const uint8_t* meshPtr = vhvData + VHV_FILE_HEADER_SIZE + (m * VHV_MESH_HEADER_SIZE);
                        uint32_t count = *(uint32_t*)(meshPtr + 4);
                        uint32_t rawOfs = *(uint32_t*)(meshPtr + 8);
                        if (count == 0 || rawOfs == 0)
                            continue;

                        const uint8_t* colors = (rawOfs < VHV_FILE_HEADER_SIZE) ?
                            (vhvData + VHV_FILE_HEADER_SIZE + (numMeshes * VHV_MESH_HEADER_SIZE) + rawOfs) :
                            (vhvData + rawOfs);

                        for (uint32_t v = 0; v < count && colorIdx < (uint32_t)totalVerts; v++)
                        {
                            // They are stored in BGRA for some reason..
                            float b = colors[v * 4 + 0] / 255.0f;
                            float g = colors[v * 4 + 1] / 255.0f;
                            float r = colors[v * 4 + 2] / 255.0f;
                            m_map.staticProps[i].vertexColors[0][colorIdx++] = glm::vec3(r, g, b);
                        }
                    }
                }

                std::string vhdName = "sp_hdr_" + std::to_string(i) + ".vhd";
                if (vhvFiles.find(vhdName) == vhvFiles.end()) vhdName = "sp_" + std::to_string(i) + ".vhd";

                if (vhvFiles.count(vhdName))
                {
                    const uint8_t* vhdData = vhvFiles[vhdName];
                    if (*(int32_t*)vhdData != 2)
                        continue;

                    int32_t totalVerts = *(int32_t*)(vhdData + 16);
                    int32_t numMeshes = *(int32_t*)(vhdData + 20);

                    for (int b = 0; b < 3; b++)
                    {
                        m_map.staticProps[i].vertexColors[b].assign(totalVerts, glm::vec3(0.0f));
                    }

                    uint32_t colorIdx = 0;
                    for (int m = 0; m < numMeshes; m++)
                    {
                        const uint8_t* meshPtr = vhdData + VHV_FILE_HEADER_SIZE + (m * VHV_MESH_HEADER_SIZE);
                        uint32_t count = *(uint32_t*)(meshPtr + 4);
                        uint32_t rawOfs = *(uint32_t*)(meshPtr + 8);
                        if (count == 0 || rawOfs == 0)
                            continue;

                        const uint8_t* colors = vhdData + rawOfs;

                        for (uint32_t v = 0; v < count && colorIdx < (uint32_t)totalVerts; v++)
                        {
                            for (int b = 0; b < 3; b++)
                            {
                                int baseIdx = (v * 3 + b) * 4;
                                float blue = colors[baseIdx + 0] / 255.0f;
                                float green = colors[baseIdx + 1] / 255.0f;
                                float red = colors[baseIdx + 2] / 255.0f;
                                m_map.staticProps[i].vertexColors[b][colorIdx] = glm::vec3(red, green, blue);
                            }
                            colorIdx++;
                        }
                    }
                    m_map.staticProps[i].hasBumpedLighting = true;
                }
            }
        }

        void ProcessFace(int faceIdx)
        {
            const Face& face = d_faces[faceIdx];
            const TexInfo& tex = d_texinfos[face.texinfo];
            const TexData& td = d_texdatas[tex.texdata];

            int lw = face.lightmapTextureSizeInLuxels[0] + 1;
            int lh = face.lightmapTextureSizeInLuxels[1] + 1;

            bool isBumped = (tex.flags & 0x0800) != 0;
            int numMaps = isBumped ? 4 : 1;
            bool hasLM = (face.lightofs >= 0 && (face.lightofs + (lw * lh * 4 * numMaps)) <= m_lightingLength);

            int axs[4] = { 0 }, ays[4] = { 0 };
            if (hasLM)
            {
                for (int m = 0; m < numMaps; m++)
                {
                    PackLightmap(face.lightofs + (m * lw * lh * 4), lw, lh, axs[m], ays[m]);
                }
            }

            if (face.dispinfo != -1) 
                LoadDisplacement(face, tex, td, axs, ays, hasLM, numMaps);
            else 
                LoadBrush(face, tex, td, axs, ays, hasLM, numMaps);
        }

        void PackLightmap(int offset, int w, int h, int& outX, int& outY)
        {
            if (m_atlasX + w > m_map.lightmapAtlasWidth)
            {
                m_atlasX = 0;
                m_atlasY += m_rowHeight;
                m_rowHeight = 0;
            }
            if (h > m_rowHeight) 
                m_rowHeight = h;

            outX = m_atlasX;
            outY = m_atlasY;

            const ColorRGBExp32* src = (const ColorRGBExp32*)(d_lighting + offset);
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    ColorRGBExp32 c = src[y * w + x];
                    float power = std::pow(2.0f, c.exponent);
                    int dest = ((outY + y) * m_map.lightmapAtlasWidth + (outX + x)) * 3;
                    m_map.lightmapAtlas[dest + 0] = (c.r / 255.0f) * power;
                    m_map.lightmapAtlas[dest + 1] = (c.g / 255.0f) * power;
                    m_map.lightmapAtlas[dest + 2] = (c.b / 255.0f) * power;
                }
            }
            m_atlasX += w;
        }

        void LoadBrush(const Face& face, const TexInfo& tex, const TexData& td, int* axs, int* ays, bool hasLM, int numMaps)
        {
            std::vector<Vertex> verts;
            std::vector<uint32_t> indices;

            for (int i = 0; i < face.numedges; i++)
            {
                int32_t se = d_surfedges[face.firstedge + i];
                uint32_t vIdx = (se >= 0) ? d_edges[se].v[0] : d_edges[-se].v[1];
                indices.push_back(vIdx);

                const auto& pos = d_verts[vIdx];
                Vertex v;
                v.position = glm::vec3(pos.x, pos.z, -pos.y) * 0.03125f;

                const Plane& plane = d_planes[face.planenum];
                v.normal = glm::vec3(plane.normal.x, plane.normal.z, -plane.normal.y);

                // Albedo UVs
                float u = glm::dot(pos, glm::vec3(tex.textureVecs[0][0], tex.textureVecs[0][1], tex.textureVecs[0][2])) + tex.textureVecs[0][3];
                float v_uv = glm::dot(pos, glm::vec3(tex.textureVecs[1][0], tex.textureVecs[1][1], tex.textureVecs[1][2])) + tex.textureVecs[1][3];
                v.uv = glm::vec2(u / td.width, v_uv / td.height);

                if (hasLM)
                {
                    float lu = glm::dot(pos, glm::vec3(tex.lightmapVecs[0][0], tex.lightmapVecs[0][1], tex.lightmapVecs[0][2])) + tex.lightmapVecs[0][3];
                    float lv = glm::dot(pos, glm::vec3(tex.lightmapVecs[1][0], tex.lightmapVecs[1][1], tex.lightmapVecs[1][2])) + tex.lightmapVecs[1][3];
                    lu -= face.lightmapTextureMinsInLuxels[0];
                    lv -= face.lightmapTextureMinsInLuxels[1];

                    int flatIdx = (numMaps == 4) ? 3 : 0;
                    v.lm_uv = glm::vec2((axs[flatIdx] + lu + 0.5f) / m_map.lightmapAtlasWidth, (ays[flatIdx] + lv + 0.5f) / m_map.lightmapAtlasHeight);

                    if (numMaps == 4)
                    {
                        v.lm_uv2 = glm::vec2((axs[0] + lu + 0.5f) / m_map.lightmapAtlasWidth, (ays[0] + lv + 0.5f) / m_map.lightmapAtlasHeight);
                        v.lm_uv3 = glm::vec2((axs[1] + lu + 0.5f) / m_map.lightmapAtlasWidth, (ays[1] + lv + 0.5f) / m_map.lightmapAtlasHeight);
                        v.lm_uv4 = glm::vec2((axs[2] + lu + 0.5f) / m_map.lightmapAtlasWidth, (ays[2] + lv + 0.5f) / m_map.lightmapAtlasHeight);
                    }
                }

                v.color = glm::vec3(1.0f);
                verts.push_back(v);
            }

            // Triangulate face fan
            for (size_t i = 1; i < verts.size() - 1; i++)
            {
                m_map.renderVertices.push_back(verts[0]);
                m_map.renderVertices.push_back(verts[i + 1]);
                m_map.renderVertices.push_back(verts[i]);

                m_map.collision.indices.push_back(indices[0]);
                m_map.collision.indices.push_back(indices[i + 1]);
                m_map.collision.indices.push_back(indices[i]);
            }
        }

        void LoadDisplacement(const Face& face, const TexInfo& tex, const TexData& td, int* axs, int* ays, bool hasLM, int numMaps)
        {
            const DispInfo& di = d_dispinfos[face.dispinfo];
            int size = (1 << di.power) + 1;

            // Get base face corners
            std::vector<glm::vec3> corners;
            for (int i = 0; i < face.numedges; i++)
            {
                int32_t se = d_surfedges[face.firstedge + i];
                corners.push_back(d_verts[(se >= 0) ? d_edges[se].v[0] : d_edges[-se].v[1]]);
            }

            // Find starting index closest to startPosition
            int start = 0;
            float minD = 1e10f;
            for (int i = 0; i < 4; i++)
            {
                float d = glm::distance(corners[i], di.startPosition);
                if (d < minD) 
                { 
                    minD = d; 
                    start = i; 
                }
            }

            std::vector<glm::vec3> q(4);
            for (int i = 0; i < 4; i++) 
                q[i] = corners[(start + i) % 4];

            std::vector<Vertex> grid(size * size);
            for (int y = 0; y < size; y++)
            {
                for (int x = 0; x < size; x++)
                {
                    float u = (float)x / (size - 1);
                    float v = (float)y / (size - 1);

                    glm::vec3 e1 = glm::mix(q[0], q[3], u);
                    glm::vec3 e2 = glm::mix(q[1], q[2], u);
                    glm::vec3 pos = glm::mix(e1, e2, v);

                    const DispVert& dv = d_dispverts[di.dispVertStart + (y * size + x)];
                    pos += dv.vec * dv.dist;

                    Vertex& vert = grid[y * size + x];
                    vert.position = glm::vec3(pos.x, pos.z, -pos.y) * 0.03125f;

                    const Plane& plane = d_planes[face.planenum];
                    vert.normal = glm::vec3(plane.normal.x, plane.normal.z, -plane.normal.y);

                    // Albedo UVs
                    float tu = glm::dot(pos, glm::vec3(tex.textureVecs[0][0], tex.textureVecs[0][1], tex.textureVecs[0][2])) + tex.textureVecs[0][3];
                    float tv = glm::dot(pos, glm::vec3(tex.textureVecs[1][0], tex.textureVecs[1][1], tex.textureVecs[1][2])) + tex.textureVecs[1][3];
                    vert.uv = glm::vec2(tu / (float)td.width, tv / (float)td.height);

                    if (hasLM)
                    {
                        float lu = glm::dot(pos, glm::vec3(tex.lightmapVecs[0][0], tex.lightmapVecs[0][1], tex.lightmapVecs[0][2])) + tex.lightmapVecs[0][3];
                        float lv = glm::dot(pos, glm::vec3(tex.lightmapVecs[1][0], tex.lightmapVecs[1][1], tex.lightmapVecs[1][2])) + tex.lightmapVecs[1][3];
                        lu -= (float)face.lightmapTextureMinsInLuxels[0];
                        lv -= (float)face.lightmapTextureMinsInLuxels[1];

                        int flatIdx = (numMaps == 4) ? 3 : 0;
                        vert.lm_uv = glm::vec2((axs[flatIdx] + lu + 0.5f) / m_map.lightmapAtlasWidth, (ays[flatIdx] + lv + 0.5f) / m_map.lightmapAtlasHeight);

                        if (numMaps == 4)
                        {
                            vert.lm_uv2 = glm::vec2((axs[0] + lu + 0.5f) / m_map.lightmapAtlasWidth, (ays[0] + lv + 0.5f) / m_map.lightmapAtlasHeight);
                            vert.lm_uv3 = glm::vec2((axs[1] + lu + 0.5f) / m_map.lightmapAtlasWidth, (ays[1] + lv + 0.5f) / m_map.lightmapAtlasHeight);
                            vert.lm_uv4 = glm::vec2((axs[2] + lu + 0.5f) / m_map.lightmapAtlasWidth, (ays[2] + lv + 0.5f) / m_map.lightmapAtlasHeight);
                        }
                    }
                    float alpha = dv.alpha / 255.0f;
                    vert.color = glm::vec3(alpha);
                }
            }

            uint32_t baseIdx = (uint32_t)m_map.collision.vertices.size();
            for (const auto& v : grid)
            {
                m_map.collision.vertices.push_back(v.position);
            }

            for (int y = 0; y < size - 1; y++)
            {
                for (int x = 0; x < size - 1; x++)
                {
                    int i0 = y * size + x;
                    int i1 = y * size + (x + 1);
                    int i2 = (y + 1) * size + (x + 1);
                    int i3 = (y + 1) * size + x;

                    m_map.renderVertices.push_back(grid[i0]);
                    m_map.renderVertices.push_back(grid[i1]);
                    m_map.renderVertices.push_back(grid[i2]);
                    m_map.renderVertices.push_back(grid[i0]);
                    m_map.renderVertices.push_back(grid[i2]);
                    m_map.renderVertices.push_back(grid[i3]);

                    m_map.collision.indices.push_back(baseIdx + i0);
                    m_map.collision.indices.push_back(baseIdx + i1);
                    m_map.collision.indices.push_back(baseIdx + i2);

                    m_map.collision.indices.push_back(baseIdx + i0);
                    m_map.collision.indices.push_back(baseIdx + i2);
                    m_map.collision.indices.push_back(baseIdx + i3);
                }
            }
        }
    };

    MapData Load(const std::string& path)
    {
        std::vector<uint8_t> buffer = Filesystem::ReadBinary(path);
        if (buffer.empty())
        {
            return MapData();
        }

        MapLoader loader(buffer);
        return loader.Process();
    }
}