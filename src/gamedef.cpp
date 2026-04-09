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
#include "gamedef.h"
#include "filesystem.h"
#include "console.h"
#include <sstream>

namespace Gamedef
{
    static std::string s_startingMap = "";

    void Init()
    {
        std::string content = Filesystem::ReadText("gamedef.txt");
        if (content.empty())
        {
            Console::Warn("gamedef.txt not found.");
            return;
        }

        std::stringstream ss(content);
        std::string line;
        while (std::getline(ss, line))
        {
            if (line.empty()) 
                continue;

            if (line.find("startmap") != std::string::npos)
            {
                size_t firstQuote = line.find('\"');
                size_t secondQuote = line.find('\"', firstQuote + 1);
                if (firstQuote != std::string::npos && secondQuote != std::string::npos)
                {
                    s_startingMap = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                }
            }
        }
    }

    std::string GetStartingMap()
    {
        return s_startingMap;
    }
}