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
#include "sentry.h"
#include "cvar.h"
#include "concmd.h"
#include "console.h"
#include <sentry.h>

namespace Sentry
{
    CVar sentry_enabled("sentry_enabled", "1", CVAR_SAVE);
    CVar sentry_dsn("sentry_dsn", "https://72d9d4bb2da65d9d3d590c08795704eb@o4505736231124992.ingest.us.sentry.io/4511183954116608", CVAR_SAVE);

    void Init()
    {
        if (sentry_enabled.GetInt() == 0)
        {
            return;
        }

        const std::string dsn = sentry_dsn.GetString();

        sentry_options_t *options = sentry_options_new();

        sentry_options_set_dsn(options, dsn.c_str());
        sentry_options_set_release(options, "tectonic-engine-2@1.0.0");
        sentry_options_set_environment(options, "development");

        #ifdef _WIN32
            sentry_options_set_handler_path(options, "crashpad_handler.exe");
        #else
            sentry_options_set_handler_path(options, "crashpad_handler");
        #endif

        int result = sentry_init(options);
    }

    void Shutdown()
    {
        if (sentry_enabled.GetInt() == 1)
        {
            sentry_close();
        }
    }
}