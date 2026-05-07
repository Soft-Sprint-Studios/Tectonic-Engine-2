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
#include "sound_reverb.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <vector>
#include <unordered_map>

namespace Reverb
{
    static std::unordered_map<int, ReverbStyle> s_styles;

    struct Filter
    {
        std::vector<float> buffer;
        int index = 0;
        float store = 0.0f;
    };

    struct ReverbJob
    {
        const int16_t* input;
        int numSamples;
        int sampleRate;
        ReverbStyle style;
        int16_t** outputPtr;
        int* outputCount;
        SDL_Semaphore* doneSem;
    };

    static SDL_Thread* s_thread = nullptr;
    static SDL_Mutex* s_mutex = nullptr;
    static SDL_Semaphore* s_jobSem = nullptr;
    static std::vector<ReverbJob> s_queue;
    static bool s_running = false;

    void SetStyle(int id, float roomSize, float damping, float wet, float dry)
    {
        s_styles[id] = { roomSize, damping, wet, dry };
    }

    static float ProcessComb(Filter& f, float input, float feedback, float damping)
    {
        float output = f.buffer[f.index];
        f.store = (output * (1.0f - damping)) + (f.store * damping);
        f.buffer[f.index] = input + (f.store * feedback);

        if (++f.index >= f.buffer.size())
        {
            f.index = 0;
        }

        return output;
    }

    static float ProcessAllPass(Filter& f, float input)
    {
        float bufOut = f.buffer[f.index];
        float val = input + (bufOut * 0.5f);
        f.buffer[f.index] = val;

        if (++f.index >= f.buffer.size())
        {
            f.index = 0;
        }

        return -input + bufOut;
    }

    static void DoWork(ReverbJob& job)
    {
        int tail = job.sampleRate * 2;
        int total = job.numSamples + tail;
        *job.outputCount = total;

        int16_t* outData = new int16_t[total];
        *job.outputPtr = outData;

        Filter combs[8];
        Filter allpass[4];
        float c_tun[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        float a_tun[4] = { 556, 441, 341, 225 };

        float scale = job.sampleRate / 44100.0f;

        for (int i = 0; i < 8; i++)
        {
            combs[i].buffer.assign((int)(c_tun[i] * scale), 0.0f);
        }

        for (int i = 0; i < 4; i++)
        {
            allpass[i].buffer.assign((int)(a_tun[i] * scale), 0.0f);
        }

        for (int i = 0; i < total; i++)
        {
            float in = 0.0f;
            if (i < job.numSamples)
            {
                in = job.input[i] / 32768.0f;
            }

            float combined = 0;
            float monoIn = in * 0.1f;

            for (int j = 0; j < 8; j++)
            {
                combined += ProcessComb(combs[j], monoIn, job.style.roomSize, job.style.damping);
            }

            for (int j = 0; j < 4; j++)
            {
                combined = ProcessAllPass(allpass[j], combined);
            }

            float res = (combined * job.style.wetLevel) + (in * job.style.dryLevel);
            outData[i] = (int16_t)(std::clamp(res, -1.0f, 1.0f) * 32767.0f);
        }

        SDL_SignalSemaphore(job.doneSem);
    }

    static int ThreadFunc(void* data)
    {
        while (s_running)
        {
            SDL_WaitSemaphore(s_jobSem);
            if (!s_running)
            {
                break;
            }

            ReverbJob job;
            SDL_LockMutex(s_mutex);
            if (!s_queue.empty())
            {
                job = s_queue.front();
                s_queue.erase(s_queue.begin());
            }
            SDL_UnlockMutex(s_mutex);

            if (job.input)
            {
                DoWork(job);
            }
        }
        return 0;
    }

    void Init()
    {
        s_mutex = SDL_CreateMutex();
        s_jobSem = SDL_CreateSemaphore(0);
        s_running = true;
        s_thread = SDL_CreateThread(ThreadFunc, "ReverbThread", nullptr);

        // Preset styles
        SetStyle(1, 0.70f, 0.30f, 0.30f, 1.00f); // Generic
        SetStyle(2, 0.50f, 0.40f, 0.40f, 0.90f); // Metal
        SetStyle(3, 0.90f, 0.50f, 0.40f, 0.80f); // Tunnel
        SetStyle(4, 0.80f, 0.20f, 0.30f, 0.85f); // Chamber
        SetStyle(5, 0.98f, 0.10f, 0.50f, 0.70f); // Cave
        SetStyle(6, 0.30f, 0.10f, 0.20f, 1.00f); // Small Room
        SetStyle(7, 0.92f, 0.40f, 0.50f, 0.70f); // Large Hall
        SetStyle(8, 0.85f, 0.70f, 0.50f, 0.70f); // Sewer
        SetStyle(9, 0.96f, 0.20f, 0.60f, 0.60f); // Hangar
        SetStyle(10, 0.60f, 0.80f, 0.30f, 0.90f); // Basement
    }

    void Shutdown()
    {
        s_running = false;
        SDL_SignalSemaphore(s_jobSem);
        SDL_WaitThread(s_thread, nullptr);
        SDL_DestroyMutex(s_mutex);
        SDL_DestroySemaphore(s_jobSem);
        s_styles.clear();
    }

    int16_t* ProcessAsync(const int16_t* input, int numSamples, int sampleRate, int styleID, int& outCount)
    {
        if (s_styles.find(styleID) == s_styles.end())
        {
            return nullptr;
        }

        int16_t* result = nullptr;
        SDL_Semaphore* sem = SDL_CreateSemaphore(0);

        ReverbJob job = { input, numSamples, sampleRate, s_styles[styleID], &result, &outCount, sem };

        SDL_LockMutex(s_mutex);
        s_queue.push_back(job);
        SDL_UnlockMutex(s_mutex);
        SDL_SignalSemaphore(s_jobSem);

        SDL_WaitSemaphore(sem);
        SDL_DestroySemaphore(sem);

        return result;
    }
}