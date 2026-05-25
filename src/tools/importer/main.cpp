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
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include <VTFLib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace AssetTool
{
    struct ToolConfig
    {
        std::string sdkBinPath;
        std::string sdkGamePath;
        std::string enginePath;
        std::string nvcompressPath;
    } g_config;

    struct MaterialTexturePaths
    {
        std::string diffuse;
        std::string normal;
    };

    void ShowHelp()
    {
        std::cout << "====================================================\n";
        std::cout << "      Tectonic Engine 2 - Asset Importer Tool       \n";
        std::cout << "====================================================\n\n";
        std::cout << "Usage:\n";
        std::cout << "  TectonicImport.exe [mode] [path_to_glb]\n\n";
        std::cout << "Modes:\n";
        std::cout << "  -model      Full model import (SMD, QC, VMT, VTF)\n";
        std::cout << "  -texture    Texture-only import (Extracts textures for brushes)\n\n";
        std::cout << "  -sound      Sound import (MP3)\n\n";
        std::cout << "Batch Support:\n";
        std::cout << "  [path] can be a file or a folder.\n";
        std::cout << "  Folders will batch process all files matching the mode's extension\n";
        std::cout << "  (.glb for models/textures, .mp3 for sounds).\n\n";
    }

    bool LoadConfig()
    {
        std::ifstream file("config.txt");
        if (!file.is_open())
        {
            std::ofstream outfile("config.txt");
            outfile << "sdk_bin: \"C:/Steam/steamapps/common/Source SDK Base 2013/bin\"\n";
            outfile << "sdk_game: \"C:/Steam/steamapps/common/Source SDK Base 2013/hl2\"\n";
            outfile << "engine_path: \"D:/VS2022PROJECTS/Tectonic-Engine-2/buildx64/RelWithDebInfo\"\n";
            outfile << "nvcompress_path: \"C:/Program Files/NVIDIA Corporation/NVIDIA Texture Tools/nvcompress.exe\"\n";
            return false;
        }

        std::string line;
        while (std::getline(file, line))
        {
            size_t first = line.find('\"');
            size_t last = line.find_last_of('\"');
            if (first != std::string::npos && last != std::string::npos)
            {
                std::string val = line.substr(first + 1, last - first - 1);
                if (line.find("sdk_bin") != std::string::npos)
                {
                    g_config.sdkBinPath = val;
                }
                if (line.find("sdk_game") != std::string::npos)
                {
                    g_config.sdkGamePath = val;
                }
                if (line.find("engine_path") != std::string::npos)
                {
                    g_config.enginePath = val;
                }
                if (line.find("nvcompress_path") != std::string::npos)
                {
                    g_config.nvcompressPath = val;
                }
            }
        }
        return true;
    }

    static float GetAccessorElement(const cgltf_accessor* accessor, size_t index, size_t element)
    {
        if (!accessor)
        {
            return 0.0f;
        }
        float out[16];
        cgltf_accessor_read_float(accessor, index, out, 16);
        return out[element];
    }

    void GenerateSMD(const std::string& modelName, const std::filesystem::path& outPath, cgltf_data* data)
    {
        std::string smdPath = (outPath / (modelName + ".smd")).string();
        std::ofstream file(smdPath);
        if (!file.is_open())
        {
            return;
        }

        file << "version 1\n";
        file << "nodes\n0 \"root\" -1\nend\n";
        file << "skeleton\ntime 0\n0 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000\nend\n";
        file << "triangles\n";

        for (cgltf_size i = 0; i < data->meshes_count; ++i)
        {
            for (cgltf_size j = 0; j < data->meshes[i].primitives_count; ++j)
            {
                cgltf_primitive& prim = data->meshes[i].primitives[j];
                cgltf_accessor* pos_acc = nullptr;
                cgltf_accessor* norm_acc = nullptr;
                cgltf_accessor* uv_acc = nullptr;

                for (cgltf_size k = 0; k < prim.attributes_count; ++k)
                {
                    if (prim.attributes[k].type == cgltf_attribute_type_position) pos_acc = prim.attributes[k].data;
                    if (prim.attributes[k].type == cgltf_attribute_type_normal) norm_acc = prim.attributes[k].data;
                    if (prim.attributes[k].type == cgltf_attribute_type_texcoord) uv_acc = prim.attributes[k].data;
                }

                if (!pos_acc)
                {
                    continue;
                }

                size_t vertex_count = prim.indices ? prim.indices->count : pos_acc->count;
                for (size_t v = 0; v < vertex_count; v += 3)
                {
                    file << modelName << "\n";
                    for (int tri = 0; tri < 3; ++tri)
                    {
                        size_t index = (prim.indices) ? cgltf_accessor_read_index(prim.indices, v + tri) : (v + tri);

                        float px = GetAccessorElement(pos_acc, index, 0);
                        float py = GetAccessorElement(pos_acc, index, 1);
                        float pz = GetAccessorElement(pos_acc, index, 2);

                        float nx = GetAccessorElement(norm_acc, index, 0);
                        float ny = GetAccessorElement(norm_acc, index, 1);
                        float nz = GetAccessorElement(norm_acc, index, 2);

                        float u = GetAccessorElement(uv_acc, index, 0);
                        float vc = GetAccessorElement(uv_acc, index, 1);

                        float sx = px * 32.0f;
                        float sy = -pz * 32.0f;
                        float sz = py * 32.0f;

                        float snx = nx;
                        float sny = -nz;
                        float snz = ny;

                        file << "0 " << std::fixed << std::setprecision(6)
                            << sx << " " << sy << " " << sz << " "
                            << snx << " " << sny << " " << snz << " "
                            << u << " " << (1.0f - vc) << " 0\n";
                    }
                }
            }
        }
        file << "end\n";
        file.close();
    }

    void ConvertToVTF(const std::string& name, unsigned char* rgbaData, int w, int h, const std::filesystem::path& outPath, bool isNormal)
    {
        vlUInt imageID;
        vlCreateImage(&imageID);
        vlBindImage(imageID);
        SVTFCreateOptions options;
        vlImageCreateDefaultCreateStructure(&options);
        options.uiVersion[0] = 7;
        options.uiVersion[1] = 2;
        options.ImageFormat = isNormal ? IMAGE_FORMAT_BGR888 : IMAGE_FORMAT_DXT1;
        options.uiFlags = isNormal ? (vlUInt)TEXTUREFLAGS_NORMAL : (vlUInt)TEXTUREFLAGS_SRGB;
        options.bMipmaps = vlTrue;

        if (vlImageCreateSingle(w, h, rgbaData, &options))
        {
            vlImageSave((outPath / (name + ".vtf")).string().c_str());
        }
        vlDeleteImage(imageID);
    }

    unsigned char* LoadBufferImage(cgltf_texture* tex, int* w, int* h)
    {
        if (!tex || !tex->image || !tex->image->buffer_view)
        {
            return nullptr;
        }
        unsigned char* raw = (unsigned char*)tex->image->buffer_view->buffer->data + tex->image->buffer_view->offset;
        return stbi_load_from_memory(raw, (int)tex->image->buffer_view->size, w, h, nullptr, 4);
    }

    void UpdateGlobalMaterialDef(const std::string& matName, const MaterialTexturePaths& paths, const std::string& subFolder)
    {
        std::filesystem::path defPath = std::filesystem::path(g_config.enginePath) / "materials.def";
        std::string content;
        std::ifstream inFile(defPath);
        if (inFile.is_open())
        {
            content.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
        }

        std::string fullKey = subFolder + "/" + matName;
        if (content.find("\"" + fullKey + "\"") != std::string::npos)
        {
            return;
        }

        std::ofstream outFile(defPath, std::ios::app);
        if (outFile.is_open())
        {
            outFile << "\n\"" << fullKey << "\"\n{\n";
            if (!paths.diffuse.empty()) outFile << "\tdiffuse = \"" << subFolder << "/" << paths.diffuse << "\"\n";
            if (!paths.normal.empty()) outFile << "\tnormal = \"" << subFolder << "/" << paths.normal << "\"\n";
            outFile << "}\n";
        }
    }

    void CopyToEngine(const std::string& modelName, const std::string& glbPath, const std::filesystem::path& workDir, const std::string& subFolder)
    {
        if (!glbPath.empty())
        {
            std::filesystem::path engineModelDir = std::filesystem::path(g_config.enginePath) / "models/tectonic";
            std::filesystem::create_directories(engineModelDir);
            std::filesystem::copy_file(glbPath, engineModelDir / (modelName + ".glb"), std::filesystem::copy_options::overwrite_existing);
        }

        for (const auto& entry : std::filesystem::directory_iterator(workDir))
        {
            if (entry.path().extension() == ".dds")
            {
                std::string stem = entry.path().stem().string();
                std::filesystem::path engineTexDir = std::filesystem::path(g_config.enginePath) / "textures" / subFolder;

                std::filesystem::create_directories(engineTexDir);
                std::filesystem::copy_file(entry.path(), engineTexDir / entry.path().filename(), std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    void ProcessModel(const std::string& glbPath)
    {
        std::filesystem::path p(glbPath);
        if (!std::filesystem::exists(p))
        {
            return;
        }

        std::string modelName = p.stem().string();
        std::filesystem::path workDir = std::filesystem::current_path() / "import_work" / modelName;
        std::filesystem::create_directories(workDir);

        cgltf_options options = {};
        cgltf_data* data = nullptr;
        if (cgltf_parse_file(&options, glbPath.c_str(), &data) != cgltf_result_success)
        {
            return;
        }
        cgltf_load_buffers(&options, data, glbPath.c_str());

        std::string qcPath = (workDir / (modelName + ".qc")).string();
        std::ofstream qcFile(qcPath);
        qcFile << "$modelname \"tectonic/" << modelName << ".mdl\"\n$staticprop\n$body \"body\" \"" << modelName << ".smd\"\n";
        qcFile << "$cdmaterials \"models/tectonic/" << modelName << "/\"\n$sequence \"idle\" \"" << modelName << ".smd\" fps 1\n$contents \"solid\"\n";
        qcFile.close();

        GenerateSMD(modelName, workDir, data);

        stbi_flip_vertically_on_write(1);
        for (cgltf_size i = 0; i < data->materials_count; ++i)
        {
            cgltf_material& mat = data->materials[i];
            std::string matName = (mat.name) ? mat.name : modelName;
            std::replace(matName.begin(), matName.end(), ' ', '_');

            int w, h;
            MaterialTexturePaths paths;

            if (unsigned char* diff = LoadBufferImage(mat.pbr_metallic_roughness.base_color_texture.texture, &w, &h))
            {
                paths.diffuse = matName + "_diffuse.dds";
                std::string tempPng = (workDir / (matName + "_diffuse.png")).string();
                stbi_write_png(tempPng.c_str(), w, h, 4, diff, w * 4);
                ConvertToVTF(matName + "_diffuse", diff, w, h, workDir, false);
                stbi_image_free(diff);

                std::string outDds = (workDir / paths.diffuse).string();
                std::string cmd = "\"\"" + g_config.nvcompressPath + "\" -bc3 \"" + tempPng + "\" \"" + outDds + "\"\"";
                system(cmd.c_str());
                std::filesystem::remove(tempPng);
            }

            if (unsigned char* norm = LoadBufferImage(mat.normal_texture.texture, &w, &h))
            {
                paths.normal = matName + "_normal.dds";
                std::string tempPng = (workDir / (matName + "_normal.png")).string();
                int pbrW, pbrH;
                if (unsigned char* pbr = LoadBufferImage(mat.pbr_metallic_roughness.metallic_roughness_texture.texture, &pbrW, &pbrH))
                {
                    if (pbrW == w && pbrH == h)
                    {
                        for (int j = 0; j < w * h * 4; j += 4)
                        {
                            norm[j + 3] = 255 - pbr[j + 1];
                        }
                    }
                    stbi_image_free(pbr);
                }
                else
                {
                    for (int j = 0; j < w * h * 4; j += 4) 
                        norm[j + 3] = 255;
                }
                stbi_write_png(tempPng.c_str(), w, h, 4, norm, w * 4);
                ConvertToVTF(matName + "_normal", norm, w, h, workDir, true);
                stbi_image_free(norm);

                std::string outDds = (workDir / paths.normal).string();
                std::string cmd = "\"\"" + g_config.nvcompressPath + "\" -bc3 \"" + tempPng + "\" \"" + outDds + "\"\"";
                system(cmd.c_str());
                std::filesystem::remove(tempPng);
            }

            std::string vmtPath = (workDir / (matName + ".vmt")).string();
            std::ofstream vmtFile(vmtPath);
            vmtFile << "\"VertexLitGeneric\"\n{\n\t\"$basetexture\" \"models/tectonic/" << modelName << "/" << matName << "_diffuse\"\n";
            vmtFile << "\t\"$bumpmap\" \"models/tectonic/" << modelName << "/" << matName << "_normal\"\n}\n";
            vmtFile.close();

            std::filesystem::path sdkMaterialDir = std::filesystem::path(g_config.sdkGamePath) / "materials/models/tectonic" / modelName;
            std::filesystem::create_directories(sdkMaterialDir);

            for (const auto& entry : std::filesystem::directory_iterator(workDir))
            {
                std::string ext = entry.path().extension().string();
                if (ext == ".vmt" || ext == ".vtf")
                {
                    std::filesystem::copy_file(entry.path(), sdkMaterialDir / entry.path().filename(), std::filesystem::copy_options::overwrite_existing);
                }
            }

            UpdateGlobalMaterialDef(matName, paths, modelName);
        }

        std::string cmd = "\"\"" + g_config.sdkBinPath + "/studiomdl.exe\" -game \"" + g_config.sdkGamePath + "\" \"" + qcPath + "\"\"";
        system(cmd.c_str());
        CopyToEngine(modelName, glbPath, workDir, modelName);
        cgltf_free(data);
        std::cout << "--- Model Import Complete ---" << std::endl;
    }

    void ProcessTextures(const std::string& glbPath)
    {
        std::filesystem::path p(glbPath);
        if (!std::filesystem::exists(p))
        {
            return;
        }

        std::string modelName = p.stem().string();
        std::filesystem::path workDir = std::filesystem::current_path() / "import_work" / (modelName + "_tex");
        std::filesystem::create_directories(workDir);

        cgltf_options options = {};
        cgltf_data* data = nullptr;
        if (cgltf_parse_file(&options, glbPath.c_str(), &data) != cgltf_result_success)
        {
            return;
        }
        cgltf_load_buffers(&options, data, glbPath.c_str());

        stbi_flip_vertically_on_write(1);
        for (cgltf_size i = 0; i < data->materials_count; ++i)
        {
            cgltf_material& mat = data->materials[i];
            if (!mat.name)
            {
                continue;
            }

            std::string matName = mat.name;
            std::replace(matName.begin(), matName.end(), ' ', '_');

            int w, h;
            MaterialTexturePaths paths;

            if (unsigned char* diff = LoadBufferImage(mat.pbr_metallic_roughness.base_color_texture.texture, &w, &h))
            {
                paths.diffuse = matName + "_diffuse.dds";
                std::string tempPng = (workDir / (matName + "_diffuse.png")).string();
                stbi_write_png(tempPng.c_str(), w, h, 4, diff, w * 4);
                ConvertToVTF(matName + "_diffuse", diff, w, h, workDir, false);
                stbi_image_free(diff);

                std::string outDds = (workDir / paths.diffuse).string();
                std::string cmd = "\"\"" + g_config.nvcompressPath + "\" -bc3 \"" + tempPng + "\" \"" + outDds + "\"\"";
                system(cmd.c_str());
                std::filesystem::remove(tempPng);
            }

            if (unsigned char* norm = LoadBufferImage(mat.normal_texture.texture, &w, &h))
            {
                paths.normal = matName + "_normal.dds";
                std::string tempPng = (workDir / (matName + "_normal.png")).string();
                int pbrW, pbrH;
                if (unsigned char* pbr = LoadBufferImage(mat.pbr_metallic_roughness.metallic_roughness_texture.texture, &pbrW, &pbrH))
                {
                    if (pbrW == w && pbrH == h)
                    {
                        for (int j = 0; j < w * h * 4; j += 4)
                        {
                            norm[j + 3] = 255 - pbr[j + 1];
                        }
                    }
                    stbi_image_free(pbr);
                }
                else
                {
                    for (int j = 0; j < w * h * 4; j += 4)
                        norm[j + 3] = 255;
                }
                stbi_write_png(tempPng.c_str(), w, h, 4, norm, w * 4);
                ConvertToVTF(matName + "_normal", norm, w, h, workDir, true);
                stbi_image_free(norm);

                std::string outDds = (workDir / paths.normal).string();
                std::string cmd = "\"\"" + g_config.nvcompressPath + "\" -bc3 \"" + tempPng + "\" \"" + outDds + "\"\"";
                system(cmd.c_str());
                std::filesystem::remove(tempPng);
            }

            std::string vmtPath = (workDir / (matName + ".vmt")).string();
            std::ofstream vmtFile(vmtPath);
            if (vmtFile.is_open())
            {
                vmtFile << "\"LightmappedGeneric\"\n{\n";
                vmtFile << "\t\"$basetexture\" \"tectonic_world/" << matName << "_diffuse\"\n";
                if (!paths.normal.empty())
                {
                    vmtFile << "\t\"$bumpmap\" \"tectonic_world/" << matName << "_normal\"\n";
                }
                vmtFile << "\t\"$surfaceprop\" \"concrete\"\n";
                vmtFile << "}\n";
                vmtFile.close();
            }

            std::filesystem::path sdkMaterialDir = std::filesystem::path(g_config.sdkGamePath) / "materials/tectonic_world";
            std::filesystem::create_directories(sdkMaterialDir);

            for (const auto& entry : std::filesystem::directory_iterator(workDir))
            {
                std::string ext = entry.path().extension().string();
                if (ext == ".vmt" || ext == ".vtf")
                {
                    std::filesystem::copy_file(entry.path(), sdkMaterialDir / entry.path().filename(), std::filesystem::copy_options::overwrite_existing);
                }
            }

            UpdateGlobalMaterialDef(matName, paths, "tectonic_world");
        }

        CopyToEngine(modelName, "", workDir, "tectonic_world");
        cgltf_free(data);
        std::cout << "--- Texture Import Complete ---" << std::endl;
    }

    void ProcessSound(const std::string& mp3Path)
    {
        std::filesystem::path p(mp3Path);
        if (!std::filesystem::exists(p) || p.extension() != ".mp3")
        {
            return;
        }

        std::filesystem::path engineSoundDir = std::filesystem::path(g_config.enginePath) / "sounds";
        std::filesystem::path sdkSoundDir = std::filesystem::path(g_config.sdkGamePath) / "sound";

        std::filesystem::create_directories(engineSoundDir);
        std::filesystem::create_directories(sdkSoundDir);

        std::filesystem::copy_file(p, engineSoundDir / p.filename(), std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(p, sdkSoundDir / p.filename(), std::filesystem::copy_options::overwrite_existing);

        std::cout << "--- Sound Import Complete: " << p.filename() << " ---" << std::endl;
    }
}

std::vector<std::string> CollectFiles(const std::filesystem::path& inputPath, const std::string& mode)
{
    std::vector<std::string> files;
    std::string targetExt;
    if (mode == "-sound") 
        targetExt = ".mp3";
    else if (mode == "-model" || mode == "-texture") 
        targetExt = ".glb";
    else return 
        files;

    if (std::filesystem::is_directory(inputPath))
    {
        std::cout << "Batch processing directory: " << inputPath.string() << " for extension " << targetExt << "..." << std::endl;
        for (const auto& entry : std::filesystem::directory_iterator(inputPath))
        {
            if (entry.path().extension() == targetExt)
            {
                files.push_back(entry.path().string());
            }
        }
    }
    else
    {
        if (inputPath.extension() == targetExt)
        {
            files.push_back(inputPath.string());
        }
        else
        {
            std::cerr << "Error: File " << inputPath.filename() << " does not match required extension " << targetExt << " for mode " << mode << std::endl;
        }
    }

    return files;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        AssetTool::ShowHelp();
        return 1;
    }

    if (!AssetTool::LoadConfig())
    {
        std::cerr << "Error: config.txt is missing or invalid. Check paths for sdk_bin, sdk_game, and engine_path." << std::endl;
        return 1;
    }

    if (!vlInitialize())
    {
        std::cerr << "Error: Could not initialize VTFLib." << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    std::filesystem::path inputPath(argv[2]);

    if (!std::filesystem::exists(inputPath))
    {
        std::cerr << "Error: Path does not exist: " << inputPath.string() << std::endl;
        vlShutdown();
        return 1;
    }

    std::filesystem::path importRoot =
        std::filesystem::current_path() / "import_work";

    if (std::filesystem::exists(importRoot))
    {
        std::filesystem::remove_all(importRoot);
    }

    std::filesystem::create_directories(importRoot);

    std::vector<std::string> processQueue = CollectFiles(inputPath, mode);

    if (processQueue.empty())
    {
        std::cout << "No valid files found to process." << std::endl;
    }
    else
    {
        for (const std::string& file : processQueue)
        {
            if (mode == "-model")
            {
                AssetTool::ProcessModel(file);
            }
            else if (mode == "-texture")
            {
                AssetTool::ProcessTextures(file);
            }
            else if (mode == "-sound")
            {
                AssetTool::ProcessSound(file);
            }
            else
            {
                AssetTool::ShowHelp();
                break;
            }
        }
    }

    vlShutdown();
    return 0;
}