#include "cvar.h"
#include "texture.h"
#include "filesystem.h"
#include "console.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

CVar texAniso("r_textureAnisotropy", "16.0", CVAR_SAVE);

Texture::Texture()
{
    m_id = 0;
    m_width = 0;
    m_height = 0;
    m_channels = 0;
}

Texture::~Texture()
{
    Release();
}

bool Texture::Load(const std::string& path)
{
    std::string fullPath = Filesystem::GetFullPath(path);
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(fullPath.c_str(), &m_width, &m_height, &m_channels, 4);

    if (!data)
    {
        Console::Error("Failed to load texture: " + fullPath);
        return false;
    }

    Create(m_width, m_height, data);
    stbi_image_free(data);

    return true;
}

void Texture::Create(int width, int height, unsigned char* data)
{
    m_width = width;
    m_height = height;

    if (m_id != 0)
    {
        glDeleteTextures(1, &m_id);
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, texAniso.GetFloat());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::Release()
{
    if (m_id != 0)
    {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}