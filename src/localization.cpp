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
#include "localization.h"
#include "filesystem.h"
#include "console.h"
#include "cvar.h"
#include <unordered_map>
#include <sstream>
#include <SDL3/SDL.h>

namespace Localization
{
    static std::unordered_map<std::string, std::string> s_localMap;
    CVar cl_language("cl_language", "none", "Current game language code (en, es, etc). 'none' for auto-detect.", CVAR_SAVE);

    void LoadLanguage(const std::string& targetCode)
    {
        s_localMap.clear();
        std::string content = Filesystem::ReadText("localization.txt");
        if (content.empty()) 
            return;

        std::stringstream ss(content);
        std::string line;
        bool inCorrectBlock = false;

        while (std::getline(ss, line))
        {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            size_t last = line.find_last_not_of(" \t\r\n");
            if (last != std::string::npos) line.erase(last + 1);

            if (line.empty()) 
                continue;

            if (line[0] == '\"' && line.find('{') == std::string::npos && !inCorrectBlock)
            {
                size_t first = line.find('\"');
                size_t second = line.find('\"', first + 1);
                if (first != std::string::npos && second != std::string::npos)
                {
                    std::string code = line.substr(first + 1, second - first - 1);
                    inCorrectBlock = (code == targetCode);
                }
                continue;
            }

            if (line.find('{') != std::string::npos) 
                continue;

            if (line.find('}') != std::string::npos)
            {
                inCorrectBlock = false;
                continue;
            }

            if (inCorrectBlock)
            {
                size_t fq = line.find('\"');
                size_t sq = line.find('\"', fq + 1);
                size_t tq = line.find('\"', sq + 1);
                size_t lq = line.find('\"', tq + 1);

                if (fq != std::string::npos && lq != std::string::npos)
                {
                    std::string key = line.substr(fq + 1, sq - fq - 1);
                    std::string val = line.substr(tq + 1, lq - tq - 1);
                    s_localMap[key] = val;
                }
            }
        }
    }

    void Init()
    {
        std::string lang = cl_language.GetString();

        if (lang == "none")
        {
            int count = 0;
            SDL_Locale** locales = SDL_GetPreferredLocales(&count);
            if (locales && count > 0)
            {
                lang = locales[0]->language; // This returns "en", "es", "fr", etc.
                SDL_free(locales);
            }
            else
            {
                lang = "en";
            }
        }

        LoadLanguage(lang);
    }

    std::string Get(const std::string& key)
    {
        auto it = s_localMap.find(key);
        return (it != s_localMap.end()) ? it->second : key;
    }
}