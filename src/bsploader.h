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

    constexpr float MAPSCALE = 0.03125f;

    enum LumpType
    {
        LUMP_ENTITIES = 0,
        LUMP_PLANES = 1,
        LUMP_TEXDATA = 2,
        LUMP_VERTEXES = 3,
        LUMP_VISIBILITY = 4,
        LUMP_NODES = 5,
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

    struct ColorRGBExp32 
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
        uint16_t iNeighbor;
        uint8_t neighborOrientation, span, neighborSpan;
    };

    struct DispInfo 
    {
        glm::vec3 startPosition;
        int32_t dispVertStart, dispTriStart, power, minTess;
        float smoothingAngle;
        int32_t contents;
        uint16_t mapFace;
        int32_t lightmapAlphaStart, lightmapSamplePositionStart;
        DispNeighbor edgeNeighbors[4][2];
        uint16_t cornerNeighbors[4][4];
        uint8_t nCornerNeighbors[4];
        uint32_t allowedVerts[10];
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
#pragma pack(pop)

    // Engine specific
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec2 lm_uv;
        glm::vec2 lm_uv2;
        glm::vec2 lm_uv3;
        glm::vec2 lm_uv4;
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
        std::vector<glm::vec3> vertexColors[3];
        bool hasBumpedLighting = false;
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

    struct MapData 
    {
        std::vector<Vertex> renderVertices;
        std::vector<DrawCall> drawCalls;
        std::vector<StaticPropInstance> staticProps;
        CollisionData collision;
        std::vector<EntityData> entities;
        std::string skyName = "";
        bool loaded = false;

        std::vector<float> lightmapAtlas;
        int lightmapAtlasWidth = 4096;
        int lightmapAtlasHeight = 4096;
    };

    MapData Load(const std::string& path);
}