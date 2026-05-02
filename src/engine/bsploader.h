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
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <glm/glm.hpp>

namespace BSP
{
    // Reference for these lines https://developer.valvesoftware.com/wiki/BSP_(Source)
    // no copyright infringment intended
    constexpr int32_t VBSP_IDENT = (('P' << 24) + ('S' << 16) + ('B' << 8) + 'V');
    constexpr int32_t SPRP_IDENT = (('s' << 24) + ('p' << 16) + ('r' << 8) + 'p');
    constexpr int32_t ZIP_IDENT = (0x04 << 24) | (0x03 << 16) | ('K' << 8) | 'P';
    constexpr int32_t VBSP_VERSION_MIN = 19;
    constexpr int32_t HEADER_LUMPS = 64;

    constexpr float MAPSCALE = 1.0f / 32.0f;

    constexpr int32_t SURF_SKY2D = 0x0002; // Skybox
    constexpr int32_t SURF_SKY = 0x0004; // Also skybox?
    constexpr int32_t SURF_WATER = 0x0008; // Seems to be water
    constexpr int32_t SURF_NODRAW = 0x0080; // Nodraw
    constexpr int32_t SURF_BUMPED = 0x0800; // Bumped lightmaps

    enum LumpType
    {
        LUMP_ENTITIES = 0,
        LUMP_PLANES = 1,
        LUMP_TEXDATA = 2,
        LUMP_VERTEXES = 3,
        LUMP_VISIBILITY = 4,
        LUMP_TEXINFO = 6,
        LUMP_FACES = 7,
        LUMP_LIGHTING = 8,
        LUMP_EDGES = 12,
        LUMP_SURFEDGES = 13,
        LUMP_MODELS = 14,
        LUMP_DISPINFO = 26,
        LUMP_TEXDATA_STRING_DATA = 43,
        LUMP_TEXDATA_STRING_TABLE = 44,
        LUMP_DISP_VERTS = 33,
        LUMP_GAMELUMP = 35,
        LUMP_PAKFILE = 40,
        LUMP_OVERLAYS = 45,
        LUMP_LIGHTING_HDR = 53
    };

#pragma pack(push, 1)
    struct Lump
    {
        int32_t offset, length, version, uncompressedSize;
    };

    struct Header
    {
        int32_t ident, version;
        Lump lumps[HEADER_LUMPS];
        int32_t mapRevision;
    };

    struct Face
    {
        uint16_t planenum;
        uint8_t side, onNode;
        int32_t firstedge;
        int16_t numedges, texinfo, dispinfo, surfaceFogVolumeID;
        uint8_t styles[4];
        int32_t lightofs;
        float area;
        int32_t lightmapTextureMinsInLuxels[2];
        int32_t lightmapTextureSizeInLuxels[2];
        int32_t origFace;
        uint16_t numPrims, firstPrimID;
        uint32_t smoothingGroups;
    };

    struct Edge
    {
        uint16_t v[2];
    };

    struct TexInfo
    {
        float textureVecs[2][4];
        float lightmapVecs[2][4];
        int32_t flags, texdata;
    };

    struct TexData
    {
        glm::vec3 reflectivity;
        int32_t nameStringTableID;
        int32_t width, height, view_width, view_height;
    };

    struct Color
    {
        uint8_t r, g, b;
        int8_t exponent;
    };

    struct Plane
    {
        glm::vec3 normal;
        float dist;
        int32_t type;
    };

    struct DispVert
    {
        glm::vec3 vec;
        float dist, alpha;
    };

    struct DispNeighbor
    {
        uint16_t neighborIndex;
        unsigned char neighborOrientation;
        unsigned char span;
        unsigned char neighborSpan;
        unsigned char pad;
    };

    struct DispCornerNeighbors
    {
        uint16_t neighborIndices[4];
        unsigned char nNeighbors;
        unsigned char pad;
    };

    struct DispInfo
    {
        glm::vec3 startPosition;
        int32_t dispVertStart;
        int32_t dispTriStart;
        int32_t power;
        int32_t minTess;
        float smoothingAngle;
        int32_t contents;
        uint16_t mapFace;
        uint16_t padding1;
        int32_t lightmapAlphaStart;
        int32_t lightmapSamplePositionStart;
        DispNeighbor edgeNeighbors[4];
        DispCornerNeighbors cornerNeighbors[4];
        uint32_t allowedVerts[10];
        char padding2[24];
    };

    struct GameLump
    {
        int32_t id;
        uint16_t flags;
        uint16_t version;
        int32_t fileofs;
        int32_t filelen;
    };

    struct GameLumpHeader
    {
        int32_t lumpCount;
    };

    struct Model
    {
        glm::vec3 mins, maxs;
        glm::vec3 origin;
        int32_t headnode;
        int32_t firstface, numfaces;
    };

    struct Overlay
    {
        int32_t id;
        int16_t texinfo;
        uint16_t faceCountAndRenderOrder;
        int32_t ofaces[64];
        float u[2];
        float v[2];
        glm::vec3 uvPoints[4];
        glm::vec3 origin;
        glm::vec3 basisNormal;
    };
#pragma pack(pop)

    // Engine specific
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        glm::vec2 uv;
        glm::vec2 lm_uv;
        glm::vec2 lm_uv2;
        glm::vec2 lm_uv3;
        glm::vec2 lm_uv4;
        glm::vec2 lm_uv5;
        glm::vec3 color;
    };

    struct DrawCall
    {
        std::string textureName;
        uint32_t start;
        uint32_t count;
        bool isBumped;
        glm::vec3 mins;
        glm::vec3 maxs;
    };

    struct StaticPropInstance
    {
        std::string modelPath;
        glm::vec3 position;
        glm::vec3 angles;
        float scale = 1.0f;
        std::vector<glm::vec4> vertexColors[3];
        bool hasBumpedLighting = false;
    };

    struct OverlayBatch
    {
        std::vector<Vertex> verts;
        bool isBumped = false;
    };

    struct CollisionData
    {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;
    };

    struct EntityData
    {
        std::string className;
        std::unordered_map<std::string, std::string> keyvalues;
        CollisionData brushCollision;
    };

    struct WaterSurface
    {
        uint32_t start, count;
        float height;
        std::string textureName;
    };

    struct MapData
    {
        std::vector<Vertex> renderVertices;
        std::vector<DrawCall> drawCalls;
        uint32_t opaqueVertexCount = 0;
        std::vector<WaterSurface> waterSurfaces;
        std::vector<StaticPropInstance> staticProps;
        CollisionData collision;
        std::vector<EntityData> entities;
        std::string skyName = "";
        bool loaded = false;

        std::vector<float> lightmapAtlas;
        int lightmapAtlasWidth = 4096;
        int lightmapAtlasHeight = 4096;
    };

    inline glm::vec3 ToEngineSpace(const glm::vec3& v)
    {
        return glm::vec3(v.x, v.z, -v.y) * MAPSCALE;
    }

    MapData Load(const std::string& path);
}