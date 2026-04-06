#pragma once
#include <glad/glad.h>
#include <string>

class Texture
{
public:
    Texture();
    ~Texture();

    bool Load(const std::string& path);
    void Create(int width, int height, unsigned char* data);
    void Bind(unsigned int unit = 0) const;
    void Release();

    int GetWidth() const
    {
        return m_width;
    }

    int GetHeight() const
    {
        return m_height;
    }

    GLuint GetID() const 
    { 
        return m_id; 
    }

private:
    GLuint m_id;
    int m_width;
    int m_height;
    int m_channels;
};