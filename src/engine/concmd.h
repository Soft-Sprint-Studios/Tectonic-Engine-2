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
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

// Command callback
typedef std::function<void(const std::vector<std::string>&)> ConCmdCallback;

class ConCmd
{
public:
    ConCmd(const std::string& name, ConCmdCallback callback, const std::string& description = "");

    static ConCmd* Find(const std::string& name);
    static std::unordered_map<std::string, ConCmd*>& GetRegistry();

    void Execute(const std::vector<std::string>& args);

    std::string GetDescription() const 
    { 
        return m_description; 
    }

private:
    std::string m_name;
    std::string m_description;
    ConCmdCallback m_callback;
};

#define CON_COMMAND(name, description) \
    void name##_callback(const std::vector<std::string>& args); \
    ConCmd name##_cmd(#name, name##_callback, description); \
    void name##_callback(const std::vector<std::string>& args)
