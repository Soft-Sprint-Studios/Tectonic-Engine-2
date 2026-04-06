#pragma once

#ifdef _WIN32
    #ifdef ENGINE_BUILD
        #define ENGINE_API extern "C" __declspec(dllexport)
    #else
        #define ENGINE_API extern "C" __declspec(dllimport)
    #endif
#else
    #define ENGINE_API extern "C"
#endif