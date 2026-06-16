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
#include "cvar.h"
#include "mp3.h"
#include "dsp.h"
#include "ambient_zone.h"
#include "timing.h"
#include <vector>
#include <algorithm>
#include <cstring>

#define AL_ALEXT_PROTOTYPES
#include <AL/alext.h>
#include <AL/efx.h>

CVar s_volume("s_volume", "1.0", "Master audio volume.", CVAR_SAVE);
CVar s_mute("s_mute", "0", "Mute all audio output.", CVAR_SAVE);
CVar s_hrtf("s_hrtf", "1", "Enable Head-Related Transfer Function (HRTF).", CVAR_SAVE);

namespace Sound
{
    static ALCdevice* s_device = nullptr;
    static ALCcontext* s_context = nullptr;
    static std::unordered_map<std::string, ALuint> s_bufferCache;
    static int s_currentStyle = 0;

    static ALuint s_efxAuxSlot = 0;
    static ALuint s_efxEffect = 0;
    static bool s_efxSupported = false;

    static LPALGENEFFECTS alGenEffects = nullptr;
    static LPALDELETEEFFECTS alDeleteEffects = nullptr;
    static LPALEFFECTI alEffecti = nullptr;
    static LPALEFFECTF alEffectf = nullptr;
    static LPALEFFECTFV alEffectfv = nullptr;
    static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
    static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
    static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;

    static void LoadEFXFunctions()
    {
        alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
        alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
        alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
        alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
        alEffectfv = (LPALEFFECTFV)alGetProcAddress("alEffectfv");
        alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
        alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
        alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");

        if (alGenEffects && alDeleteEffects && alEffecti && alEffectf && alEffectfv &&
            alGenAuxiliaryEffectSlots && alDeleteAuxiliaryEffectSlots && alAuxiliaryEffectSloti)
        {
            s_efxSupported = true;
        }
        else
        {
            s_efxSupported = false;
        }
    }

    static void LoadReverbPreset(const EFXEAXREVERBPROPERTIES& p)
    {
        if (!s_efxSupported)
        {
            return;
        }

        alEffecti(s_efxEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
        alEffectf(s_efxEffect, AL_EAXREVERB_DENSITY, p.flDensity);
        alEffectf(s_efxEffect, AL_EAXREVERB_DIFFUSION, p.flDiffusion);
        alEffectf(s_efxEffect, AL_EAXREVERB_GAIN, p.flGain);
        alEffectf(s_efxEffect, AL_EAXREVERB_GAINHF, p.flGainHF);
        alEffectf(s_efxEffect, AL_EAXREVERB_GAINLF, p.flGainLF);
        alEffectf(s_efxEffect, AL_EAXREVERB_DECAY_TIME, p.flDecayTime);
        alEffectf(s_efxEffect, AL_EAXREVERB_DECAY_HFRATIO, p.flDecayHFRatio);
        alEffectf(s_efxEffect, AL_EAXREVERB_DECAY_LFRATIO, p.flDecayLFRatio);
        alEffectf(s_efxEffect, AL_EAXREVERB_REFLECTIONS_GAIN, p.flReflectionsGain);
        alEffectf(s_efxEffect, AL_EAXREVERB_REFLECTIONS_DELAY, p.flReflectionsDelay);
        alEffectfv(s_efxEffect, AL_EAXREVERB_REFLECTIONS_PAN, p.flReflectionsPan);
        alEffectf(s_efxEffect, AL_EAXREVERB_LATE_REVERB_GAIN, p.flLateReverbGain);
        alEffectf(s_efxEffect, AL_EAXREVERB_LATE_REVERB_DELAY, p.flLateReverbDelay);
        alEffectfv(s_efxEffect, AL_EAXREVERB_LATE_REVERB_PAN, p.flLateReverbPan);
        alEffectf(s_efxEffect, AL_EAXREVERB_ECHO_TIME, p.flEchoTime);
        alEffectf(s_efxEffect, AL_EAXREVERB_ECHO_DEPTH, p.flEchoDepth);
        alEffectf(s_efxEffect, AL_EAXREVERB_MODULATION_TIME, p.flModulationTime);
        alEffectf(s_efxEffect, AL_EAXREVERB_MODULATION_DEPTH, p.flModulationDepth);
        alEffectf(s_efxEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, p.flAirAbsorptionGainHF);
        alEffectf(s_efxEffect, AL_EAXREVERB_HFREFERENCE, p.flHFReference);
        alEffectf(s_efxEffect, AL_EAXREVERB_LFREFERENCE, p.flLFReference);
        alEffectf(s_efxEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, p.flRoomRolloffFactor);
        alEffecti(s_efxEffect, AL_EAXREVERB_DECAY_HFLIMIT, p.iDecayHFLimit);

        alAuxiliaryEffectSloti(s_efxAuxSlot, AL_EFFECTSLOT_EFFECT, s_efxEffect);
    }

    static ALuint Internal_LoadMP3(const std::string& path)
    {
        MP3::AudioData data;
        if (!MP3::Load(path, data))
        {
            return 0;
        }

        ALenum format = (data.channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        ALuint bufferID;
        alGenBuffers(1, &bufferID);
        alBufferData(bufferID, format, data.pcm.data(), (ALsizei)(data.pcm.size() * 2), data.sampleRate);
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

        ALCint attrs[] = { ALC_HRTF_SOFT, s_hrtf.GetInt() > 0 ? ALC_TRUE : ALC_FALSE, 0 };

        s_context = alcCreateContext(s_device, alcIsExtensionPresent(s_device, "ALC_SOFT_HRTF") ? attrs : nullptr);

        alcMakeContextCurrent(s_context);

        alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

        if (alcIsExtensionPresent(s_device, "ALC_EXT_EFX"))
        {
            LoadEFXFunctions();

            if (s_efxSupported)
            {
                DSP::Init();
                alGenAuxiliaryEffectSlots(1, &s_efxAuxSlot);
                alGenEffects(1, &s_efxEffect);

                alEffecti(s_efxEffect, AL_EFFECT_TYPE, AL_EFFECT_NULL);
                alAuxiliaryEffectSloti(s_efxAuxSlot, AL_EFFECTSLOT_EFFECT, s_efxEffect);
            }
        }

        AmbientZone::Init();
    }

    void Update(const glm::vec3& listenerPos, const glm::vec3& listenerForward)
    {
        float vol = std::clamp(s_volume.GetFloat(), 0.0f, 5.0f);
        float gain = (s_mute.GetInt() > 0) ? 0.0f : vol;
        alListenerf(AL_GAIN, gain);

        alListenerfv(AL_POSITION, &listenerPos.x);
        float orientation[] = { listenerForward.x, listenerForward.y, listenerForward.z, 0.0f, 1.0f, 0.0f };
        alListenerfv(AL_ORIENTATION, orientation);

        AmbientZone::Update(Time::DeltaTime(), listenerPos);
    }

    void SetRoomStyle(int styleID)
    {
        s_currentStyle = styleID;
        if (!s_efxSupported)
        {
            return;
        }

        if (styleID == 0)
        {
            alEffecti(s_efxEffect, AL_EFFECT_TYPE, AL_EFFECT_NULL);
            alAuxiliaryEffectSloti(s_efxAuxSlot, AL_EFFECTSLOT_EFFECT, s_efxEffect);
            return;
        }

        LoadReverbPreset(DSP::GetPreset(styleID));
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

        if (fullPath.find(".mp3") != std::string::npos)
        {
            buffer = Internal_LoadMP3(fullPath);
        }

        if (buffer != 0)
        {
            s_bufferCache[fullPath] = buffer;
        }
        else
        {
            Console::Warn("Sound: Failed to find or load " + fullPath);
        }

        return buffer;
    }

    void Shutdown()
    {
        AmbientZone::Shutdown();

        if (s_efxSupported)
        {
            if (s_efxAuxSlot)
            {
                alDeleteAuxiliaryEffectSlots(1, &s_efxAuxSlot);
            }
            if (s_efxEffect)
            {
                alDeleteEffects(1, &s_efxEffect);
            }
        }

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

        if (s_efxSupported)
        {
            alSource3i(m_sourceID, AL_AUXILIARY_SEND_FILTER, s_efxAuxSlot, 0, AL_FILTER_NULL);
        }
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

    void AudioSource::SetSpatialized(bool spatial)
    {
        alSourcei(m_sourceID, AL_SOURCE_RELATIVE, spatial ? AL_FALSE : AL_TRUE);
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

    bool AudioSource::IsPlaying() const
    {
        ALint state;
        alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
        return (state == AL_PLAYING);
    }
}