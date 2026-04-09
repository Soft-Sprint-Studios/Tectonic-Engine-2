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
#include "build_date.h"
#include <ctime>
#include <cstdio>
#include <cstring>

namespace Build
{
    static int s_cachedBuildNumber = -1;

    const char* GetCompileDate()
    {
        return __DATE__;
    }

    const char* GetCompileTime()
    {
        return __TIME__;
    }

    int GetBuildNumber()
    {
        if (s_cachedBuildNumber != -1)
        {
            return s_cachedBuildNumber;
        }

        char monthStr[4];
        int day, year;
        std::sscanf(__DATE__, "%s %d %d", monthStr, &day, &year);

        const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        int month = 0;
        for (int i = 0; i < 12; i++)
        {
            if (std::strcmp(monthStr, months[i]) == 0)
            {
                month = i;
                break;
            }
        }

        std::tm buildTm = {};
        buildTm.tm_year = year - 1900;
        buildTm.tm_mon = month;
        buildTm.tm_mday = day;
        buildTm.tm_isdst = -1;

        std::tm startTm = {};
        startTm.tm_year = 2026 - 1900;
        startTm.tm_mon = 3;
        startTm.tm_mday = 4;
        startTm.tm_isdst = -1;

        // Compare time
        std::time_t buildTime = std::mktime(&buildTm);
        std::time_t startTime = std::mktime(&startTm);

        double seconds = std::difftime(buildTime, startTime);
        s_cachedBuildNumber = static_cast<int>(seconds / (60 * 60 * 24));

        return s_cachedBuildNumber;
    }
}