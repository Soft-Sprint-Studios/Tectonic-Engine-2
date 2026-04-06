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
#include "sound.h"
#include "filesystem.h"
#include "console.h"
#include <vector>

#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

namespace Sound
{
    static ALCdevice* s_device = nullptr;
    static ALCcontext* s_context = nullptr;
    static std::unordered_map<std::string, ALuint> s_bufferCache;

    static ALuint Internal_LoadWAV(const std::string& path)
    {
        std::vector<uint8_t> fileData = Filesystem::ReadBinary(path);
        if (fileData.empty()) 
        {
            return 0;
        }

        const uint8_t* ptr = fileData.data();
        const uint8_t* end = ptr + fileData.size();

        if (strncmp((const char*)ptr, "RIFF", 4) != 0) return 0;
        ptr += 8;
        if (strncmp((const char*)ptr, "WAVE", 4) != 0) return 0;
        ptr += 4;

        bool foundFmt = false;
        bool foundData = false;
        uint16_t audioFormat = 0, numChannels = 0, bitsPerSample = 0;
        uint32_t sampleRate = 0, dataSize = 0;
        const uint8_t* audioData = nullptr;

        while (ptr + 8 <= end)
        {
            char chunkId[5] = {0};
            memcpy(chunkId, ptr, 4);
            uint32_t chunkSize = *(uint32_t*)(ptr + 4);
            ptr += 8;

            if (strcmp(chunkId, "fmt ") == 0)
            {
                foundFmt = true;
                audioFormat = *(uint16_t*)ptr;
                numChannels = *(uint16_t*)(ptr + 2);
                sampleRate = *(uint32_t*)(ptr + 4);
                bitsPerSample = *(uint16_t*)(ptr + 14);
            }
            else if (strcmp(chunkId, "data") == 0)
            {
                foundData = true;
                dataSize = chunkSize;
                audioData = ptr;
            }
            
            ptr += chunkSize;
        }

        if (!foundFmt || !foundData || !audioData) 
            return 0;

        ALenum format;
        if (numChannels == 1) 
            format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
        else 
            format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;

        ALuint bufferID;
        alGenBuffers(1, &bufferID);
        alBufferData(bufferID, format, audioData, dataSize, sampleRate);
        return bufferID;
    }

    static ALuint Internal_LoadMP3(const std::string& path)
    {
        std::vector<uint8_t> fileData = Filesystem::ReadBinary(path);
        if (fileData.empty()) 
            return 0;

        mp3dec_ex_t mp3d;
        if (mp3dec_ex_open_buf(&mp3d, fileData.data(), fileData.size(), 0))
            return 0;

        std::vector<short> pcm(mp3d.samples);
        mp3dec_ex_read(&mp3d, pcm.data(), mp3d.samples);

        ALenum format = (mp3d.info.channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        ALuint bufferID;
        alGenBuffers(1, &bufferID);
        alBufferData(bufferID, format, pcm.data(), (ALsizei)(pcm.size() * 2), mp3d.info.hz);

        mp3dec_ex_close(&mp3d);
        return bufferID;
    }

    void Init()
    {
        s_device = alcOpenDevice(nullptr);
        if (!s_device)
        {
            Console::Error("Sound: Failed to open audio device.");
            return;
        }

        s_context = alcCreateContext(s_device, nullptr);
        alcMakeContextCurrent(s_context);

        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    }

    void Update(const glm::vec3& listenerPos, const glm::vec3& listenerForward)
    {
        alListenerfv(AL_POSITION, &listenerPos.x);
        float orientation[] = { listenerForward.x, listenerForward.y, listenerForward.z, 0.0f, 1.0f, 0.0f };
        alListenerfv(AL_ORIENTATION, orientation);
    }

    ALuint GetBuffer(const std::string& fileName)
    {
        // always in sounds folders
        std::string fullPath = "sounds/" + fileName;

        if (s_bufferCache.count(fullPath))
        {
            return s_bufferCache[fullPath];
        }

        ALuint buffer = 0;

        if (fullPath.find(".wav") != std::string::npos)
        {
            buffer = Internal_LoadWAV(fullPath);
        }
        else if (fullPath.find(".mp3") != std::string::npos)
        {
            buffer = Internal_LoadMP3(fullPath);
        }

        if (buffer != 0)
        {
            s_bufferCache[fullPath] = buffer;
            Console::Log("Sound: Loaded " + fullPath);
        }
        else
        {
            Console::Warn("Sound: Failed to find or load " + fullPath);
        }

        return buffer;
    }

    void Shutdown()
    {
        for (auto const& [path, id] : s_bufferCache) 
        {
            alDeleteBuffers(1, &id);
        }
        s_bufferCache.clear();

        alcMakeContextCurrent(nullptr);
        if (s_context) 
            alcDestroyContext(s_context);
        if (s_device) 
            alcCloseDevice(s_device);
    }

    AudioSource::AudioSource()
    {
        alGenSources(1, &m_sourceID);
        alSourcef(m_sourceID, AL_REFERENCE_DISTANCE, 1.0f);
        alSourcef(m_sourceID, AL_MAX_DISTANCE, 100.0f);
        alSourcef(m_sourceID, AL_ROLLOFF_FACTOR, 1.0f);
    }

    AudioSource::~AudioSource()
    {
        if (m_sourceID != 0) 
        {
            alDeleteSources(1, &m_sourceID);
        }
    }

    void AudioSource::Play(const std::string& path, bool loop)
    {
        ALuint buffer = GetBuffer(path);
        if (buffer == 0) 
            return;

        alSourcei(m_sourceID, AL_BUFFER, buffer);
        alSourcei(m_sourceID, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcePlay(m_sourceID);
    }

    void AudioSource::Stop()
    {
        alSourceStop(m_sourceID);
    }

    void AudioSource::SetVolume(float volume)
    {
        alSourcef(m_sourceID, AL_GAIN, volume);
    }

    void AudioSource::SetPitch(float pitch)
    {
        alSourcef(m_sourceID, AL_PITCH, pitch);
    }

    void AudioSource::SetPosition(const glm::vec3& pos)
    {
        alSourcefv(m_sourceID, AL_POSITION, &pos.x);
    }

    void AudioSource::SetRadius(float ref, float max)
    {
        alSourcef(m_sourceID, AL_REFERENCE_DISTANCE, ref);
        alSourcef(m_sourceID, AL_MAX_DISTANCE, max);
    }

    void AudioSource::SetLooping(bool loop)
    {
        alSourcei(m_sourceID, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    }

    bool AudioSource::IsLooping() const
    {
        ALint looping;
        alGetSourcei(m_sourceID, AL_LOOPING, &looping);
        return (looping == AL_TRUE);
    }

    bool AudioSource::IsPlaying() const
    {
        ALint state;
        alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
        return (state == AL_PLAYING);
    }
}