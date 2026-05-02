#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <AL/al.h>
#include <AL/alc.h>

namespace Sound
{
    void Init();
    void Update(const glm::vec3& listenerPos, const glm::vec3& listenerForward);
    void Shutdown();

    ALuint GetBuffer(const std::string& path);

    class AudioSource
    {
    public:
        AudioSource();
        ~AudioSource();

        void Play(const std::string& path, bool loop = false);
        void Stop();
        
        void SetVolume(float volume);
        void SetPitch(float pitch);
        void SetPosition(const glm::vec3& pos);
        void SetRadius(float referenceDist, float maxDist);
        
        void SetLooping(bool loop);
        bool IsLooping() const;
        bool IsPlaying() const;

    private:
        ALuint m_sourceID = 0;
    };
}