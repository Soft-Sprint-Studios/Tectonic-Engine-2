cmake_minimum_required(VERSION 3.20)
project(TectonicEngine2)

if(MSVC)
    add_compile_options(/we4013 /MP)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- PATHS ----
set(SDL3_PATH ${CMAKE_SOURCE_DIR}/libs/SDL3-3.4.4)
set(SDL_TTF_PATH ${CMAKE_SOURCE_DIR}/libs/SDL3_ttf-3.2.0)
set(GLM_PATH ${CMAKE_SOURCE_DIR}/libs/GLM)
set(STB_IMAGE_PATH ${CMAKE_SOURCE_DIR}/libs/stb_image)
set(BULLET_PATH ${CMAKE_SOURCE_DIR}/libs/bullet)
set(CGLTF_PATH ${CMAKE_SOURCE_DIR}/libs/cgltf)
set(OPENAL_PATH ${CMAKE_SOURCE_DIR}/libs/openal-soft-1.24.3)
set(MINIMP3_PATH ${CMAKE_SOURCE_DIR}/libs/minimp3)
set(DISCORDRPC_PATH ${CMAKE_SOURCE_DIR}/libs/DiscordRPC)
set(SENTRY_PATH ${CMAKE_SOURCE_DIR}/libs/sentry)
set(PLMPEG_PATH ${CMAKE_SOURCE_DIR}/libs/pl_mpeg)
set(VTFLIB_PATH ${CMAKE_SOURCE_DIR}/libs/VTFLib)
set(MINISPEC_PATH ${CMAKE_SOURCE_DIR}/libs/minispec)
set(MINIZ_PATH ${CMAKE_SOURCE_DIR}/libs/miniz)

# ---- Detect 32/64-bit ----
if(WIN32)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "(ARM64|arm64)")
        set(ARCH_FOLDER "x64arm")
        set(IS_ARM64_CROSS TRUE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_FOLDER "x64")
    else()
        set(ARCH_FOLDER "x86")
    endif()
endif()

# ---- SOURCE FILES ----
file(GLOB_RECURSE ENGINE_SRC "src/engine/*.cpp")
file(GLOB_RECURSE ENGINE_HDR "src/engine/*.h" "src/engine/*.hpp")
set(MINIZ_SRC "${MINIZ_PATH}/src/miniz.c")

file(GLOB_RECURSE GAME_SRC "src/game/*.cpp")
file(GLOB_RECURSE GAME_HDR "src/game/*.h")

include(FetchContent)

set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    bgfx
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG master
)

FetchContent_MakeAvailable(bgfx)

# ---- GAME STATIC LIB ----
add_library(Game STATIC ${GAME_SRC} ${GAME_HDR})
set_target_properties(Game PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(Game PRIVATE GLM_ENABLE_EXPERIMENTAL ENGINE_BUILD)
target_include_directories(Game PRIVATE
    ${SDL3_PATH}/include
    ${GLM_PATH}/include
    ${BULLET_PATH}/include
    ${OPENAL_PATH}/include
    ${CMAKE_SOURCE_DIR}/src/shaders
    src/engine
    src/game
)

# ---- ENGINE DLL ----
add_library(Engine SHARED ${ENGINE_SRC} ${ENGINE_HDR} ${GLAD_SRC} ${MINIZ_SRC})

if(UNIX)
    set_target_properties(Engine PROPERTIES INSTALL_RPATH "$ORIGIN")
    set_target_properties(Engine PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
endif()

target_compile_definitions(Engine PRIVATE GLM_ENABLE_EXPERIMENTAL ENGINE_BUILD)

target_include_directories(Engine PRIVATE
    ${SDL3_PATH}/include
    ${SDL_TTF_PATH}/include
    ${GLM_PATH}/include
    ${STB_IMAGE_PATH}/include
    ${GLAD_PATH}/include
    ${BULLET_PATH}/include
    ${CGLTF_PATH}/include
    ${OPENAL_PATH}/include
    ${MINIMP3_PATH}/include
    ${DISCORDRPC_PATH}/include
    ${SENTRY_PATH}/include
    ${PLMPEG_PATH}/include
    ${MINISPEC_PATH}/include
    ${MINIZ_PATH}/include
    ${CMAKE_SOURCE_DIR}/src/shaders
    src/engine
    src/game
)

# ---- SHADER COMPILATION ----
if(NOT IS_ARM64_CROSS)
    set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
    set(SHADER_OUT_DIR "$<TARGET_FILE_DIR:Engine>/shaders")

    file(GLOB SHADER_FILES "${SHADER_SRC_DIR}/*.vert" "${SHADER_SRC_DIR}/*.frag" "${SHADER_SRC_DIR}/*.comp")

    foreach(SHADER_FILE ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
        get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)

        if(SHADER_EXT STREQUAL ".vert")
            set(SHADER_TYPE "vertex")
        elseif(SHADER_EXT STREQUAL ".frag")
            set(SHADER_TYPE "fragment")
        elseif(SHADER_EXT STREQUAL ".comp")
            set(SHADER_TYPE "compute")
        endif()

        add_custom_command(
            TARGET Engine PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}"
            COMMAND $<TARGET_FILE:shaderc> -f "${SHADER_FILE}" -i "${bgfx_SOURCE_DIR}/bgfx/src" -i "${bgfx_SOURCE_DIR}/src" -i "${SHADER_SRC_DIR}" --varyingdef "${SHADER_SRC_DIR}/varying.def.sc" -o "${SHADER_OUT_DIR}/${SHADER_NAME}.bin" --type ${SHADER_TYPE} --platform vulkan -p spirv
            COMMENT "Compiling shader ${SHADER_NAME} to Vulkan SPIR-V"
        )
    endforeach()
endif()

# ---- LINKING ----
if(WIN32)
    target_link_libraries(Engine PRIVATE Game)

    target_link_options(Engine PRIVATE "/WHOLEARCHIVE:Game")

    target_link_libraries(Engine PRIVATE
        ${SDL3_PATH}/lib/${ARCH_FOLDER}/SDL3.lib
        ${SDL_TTF_PATH}/lib/${ARCH_FOLDER}/SDL3_ttf.lib
        ${BULLET_PATH}/lib/${ARCH_FOLDER}/Bullet3Dynamics.lib
        ${BULLET_PATH}/lib/${ARCH_FOLDER}/BulletCollision.lib
        ${BULLET_PATH}/lib/${ARCH_FOLDER}/BulletDynamics.lib
        ${BULLET_PATH}/lib/${ARCH_FOLDER}/LinearMath.lib
        ${OPENAL_PATH}/lib/${ARCH_FOLDER}/OpenAL32.lib
        ${DISCORDRPC_PATH}/lib/${ARCH_FOLDER}/discord-rpc.lib
        ${SENTRY_PATH}/lib/${ARCH_FOLDER}/sentry.lib
        bgfx
        ws2_32
    )
elseif(UNIX)
    target_link_libraries(Engine PRIVATE -Wl,--whole-archive Game -Wl,--no-whole-archive)

    target_link_libraries(Engine PRIVATE
        "${SDL3_PATH}/lib/linux/libSDL3.so"
        "${SDL_TTF_PATH}/lib/linux/libSDL3_ttf.so"
        "${BULLET_PATH}/lib/linux/libBulletDynamics.a"
        "${BULLET_PATH}/lib/linux/libBulletCollision.a"
        "${BULLET_PATH}/lib/linux/libLinearMath.a"
        "${DISCORDRPC_PATH}/lib/linux/libdiscord-rpc.a"
        "${SENTRY_PATH}/lib/linux/libsentry.so"
        "${OPENAL_PATH}/lib/Linux/libopenal.a"
        bgfx
        ${CMAKE_DL_LIBS}
        pthread
    )
endif()

# ---- LAUNCHER ----
add_executable(Launcher ${CMAKE_SOURCE_DIR}/src/launcher/launcher.cpp)
target_include_directories(Launcher PRIVATE src/engine src/game)

if(WIN32)
    set(LAUNCHER_ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/src/launcher/resources/launcher.rc")
    set(LAUNCHER_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/src/launcher/resources/launcher.manifest")
    target_sources(Launcher PRIVATE ${LAUNCHER_ICON_RC} ${LAUNCHER_MANIFEST})
    target_link_options(Launcher PRIVATE "/LARGEADDRESSAWARE")
    target_link_options(Engine PRIVATE "/LARGEADDRESSAWARE")
    if(MSVC AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(Engine PRIVATE "/SAFESEH:NO")
        target_link_options(Launcher PRIVATE "/SAFESEH:NO")
        target_link_options(Game PRIVATE "/SAFESEH:NO")
    endif()
    target_link_options(Launcher PRIVATE "/MANIFEST:EMBED" "/MANIFESTINPUT:${LAUNCHER_MANIFEST}" /subsystem:windows)
endif()

# for now win 64 bit only
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 
   AND NOT CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64"
   AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "(ARM64|arm64)")

    set(TOOL_SRC "src/tools/importer/main.cpp")
    add_executable(TectonicImport ${TOOL_SRC})

    target_include_directories(TectonicImport PRIVATE 
        ${GLM_PATH}/include
        ${CGLTF_PATH}/include
        ${VTFLIB_PATH}/include
        ${STB_IMAGE_PATH}/include
    )

    target_link_libraries(TectonicImport PRIVATE
        ${VTFLIB_PATH}/lib/${ARCH_FOLDER}/VTFLib.lib
    )

    add_custom_command(TARGET TectonicImport POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Engine>"
        "$<TARGET_FILE_DIR:TectonicImport>"
    )

    add_custom_command(TARGET TectonicImport POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VTFLIB_PATH}/lib/${ARCH_FOLDER}/VTFLib.dll"
        $<TARGET_FILE_DIR:TectonicImport>
    )

endif()

# ---- DLL COPY ----
if (WIN32)
    add_custom_command(TARGET Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_PATH}/lib/${ARCH_FOLDER}/SDL3.dll"
        "${SDL_TTF_PATH}/lib/${ARCH_FOLDER}/SDL3_ttf.dll"
        "${SENTRY_PATH}/lib/${ARCH_FOLDER}/sentry.dll"
        "${SENTRY_PATH}/lib/${ARCH_FOLDER}/crashpad_wer.dll"
        "${SENTRY_PATH}/lib/${ARCH_FOLDER}/crashpad_handler.exe"
        "${OPENAL_PATH}/lib/${ARCH_FOLDER}/OpenAL32.dll"
        $<TARGET_FILE_DIR:Engine>
    )
elseif(UNIX)
    add_custom_command(TARGET Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SENTRY_PATH}/lib/linux/libsentry.so" $<TARGET_FILE_DIR:Engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SENTRY_PATH}/lib/linux/crashpad_handler" $<TARGET_FILE_DIR:Engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENAL_PATH}/lib/Linux/libopenal.a" $<TARGET_FILE_DIR:Engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL3_PATH}/lib/linux/libSDL3.so" $<TARGET_FILE_DIR:Engine>/libSDL3.so.0
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL3_PATH}/lib/linux/libSDL3.so" $<TARGET_FILE_DIR:Engine>/libSDL3.so
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_TTF_PATH}/lib/linux/libSDL3_ttf.so" $<TARGET_FILE_DIR:Engine>/libSDL3_ttf.so.0
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_TTF_PATH}/lib/linux/libSDL3_ttf.so" $<TARGET_FILE_DIR:Engine>/libSDL3_ttf.so
    )
endif()