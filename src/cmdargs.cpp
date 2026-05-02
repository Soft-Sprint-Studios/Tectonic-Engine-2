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
#include "cmdargs.h"
#include "console.h"
#include <vector>

namespace CommandLine
{
    static std::vector<std::string> s_args;

    void Init(int argc, char** argv)
    {
        for (int i = 0; i < argc; i++)
        {
            s_args.push_back(argv[i]);
        }
    }

    int FindParm(const std::string& parm)
    {
        for (size_t i = 1; i < s_args.size(); i++)
        {
            if (s_args[i] == parm) 
                return (int)i;
        }
        return 0;
    }

    bool HasParm(const std::string& parm)
    {
        return FindParm(parm) != 0;
    }

    std::string GetValue(const std::string& parm, const std::string& defaultVal)
    {
        int idx = FindParm(parm);
        if (idx != 0 && (size_t)idx + 1 < s_args.size())
        {
            // dont return the next param if itss another flag
            if (s_args[idx + 1][0] != '-' && s_args[idx + 1][0] != '+')
                return s_args[idx + 1];
        }
        return defaultVal;
    }

    void ExecuteInitialCommands()
    {
        for (size_t i = 1; i < s_args.size(); i++)
        {
            if (s_args[i][0] == '+')
            {
                std::string cmd = s_args[i].substr(1);
                
                // append arguments following the command until the next flag
                for (size_t j = i + 1; j < s_args.size(); j++)
                {
                    if (s_args[j][0] == '+' || s_args[j][0] == '-') 
                        break;
                    cmd += " " + s_args[j];
                }

                Console::Execute(cmd);
            }
        }
    }
}