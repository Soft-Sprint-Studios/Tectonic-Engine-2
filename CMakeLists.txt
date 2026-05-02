cmake_minimum_required(VERSION 3.20)
project(TectonicEngine2)

if(MSVC)
    add_compile_options(/we4013)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- PATHS ----
set(SDL3_PATH ${CMAKE_SOURCE_DIR}/libs/SDL3-3.4.4)
set(SDL_TTF_PATH ${CMAKE_SOURCE_DIR}/libs/SDL3_ttf-3.2.0)
set(GLM_PATH ${CMAKE_SOURCE_DIR}/libs/GLM)
set(STB_IMAGE_PATH ${CMAKE_SOURCE_DIR}/libs/stb_image)
set(GLAD_PATH ${CMAKE_SOURCE_DIR}/libs/glad)
set(BULLET_PATH ${CMAKE_SOURCE_DIR}/libs/bullet)
set(CGLTF_PATH ${CMAKE_SOURCE_DIR}/libs/cgltf)
set(OPENAL_PATH ${CMAKE_SOURCE_DIR}/libs/openal-soft-1.24.3)
set(MINIMP3_PATH ${CMAKE_SOURCE_DIR}/libs/minimp3)
set(DISCORDRPC_PATH ${CMAKE_SOURCE_DIR}/libs/DiscordRPC)
set(SENTRY_PATH ${CMAKE_SOURCE_DIR}/libs/sentry)

# ---- Detect 32/64-bit ----
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_FOLDER "x64")
    else()
        set(ARCH_FOLDER "x86")
    endif()
endif()

# ---- SOURCE FILES ----
file(GLOB_RECURSE ENGINE_SRC "src/engine/*.cpp")
file(GLOB_RECURSE ENGINE_HDR "src/engine/*.h" "src/engine/*.hpp")
set(GLAD_SRC "${GLAD_PATH}/src/glad.c")

file(GLOB_RECURSE GAME_SRC "src/game/*.cpp")
file(GLOB_RECURSE GAME_HDR "src/game/*.h")

# ---- GAME STATIC LIB ----
add_library(Game STATIC ${GAME_SRC} ${GAME_HDR})
target_compile_definitions(Game PRIVATE GLM_ENABLE_EXPERIMENTAL ENGINE_BUILD)
target_include_directories(Game PRIVATE
    ${SDL3_PATH}/include
    ${GLM_PATH}/include
    ${BULLET_PATH}/include
    ${OPENAL_PATH}/include
    src/engine
    src/game
)

# ---- ENGINE DLL ----
add_library(Engine SHARED ${ENGINE_SRC} ${ENGINE_HDR} ${GLAD_SRC})
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
    src/engine
    src/game
)

# ---- LINKING ----
find_package(OpenGL REQUIRED)

if(WIN32)
    target_link_libraries(Engine PRIVATE Game)

    if(MSVC)
        target_link_options(Engine PRIVATE "/WHOLEARCHIVE:Game")
    else()
        target_link_options(Engine PRIVATE "-Wl,--whole-archive" "Game" "-Wl,--no-whole-archive")
    endif()

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
        OpenGL::GL
        ws2_32
    )
endif()

# ---- LAUNCHER ----
add_executable(Launcher ${CMAKE_SOURCE_DIR}/launcher/launcher.cpp)
target_include_directories(Launcher PRIVATE src/engine src/game)
target_link_libraries(Launcher PRIVATE Engine)

if(WIN32)
    set(LAUNCHER_ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/launcher/resources/launcher.rc")
    set(LAUNCHER_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/launcher/resources/launcher.manifest")
    target_sources(Launcher PRIVATE ${LAUNCHER_ICON_RC} ${LAUNCHER_MANIFEST})
    target_link_options(Launcher PRIVATE "/LARGEADDRESSAWARE")
    target_link_options(Engine PRIVATE "/LARGEADDRESSAWARE")
    target_link_options(Launcher PRIVATE "/MANIFEST:EMBED" "/MANIFESTINPUT:${LAUNCHER_MANIFEST}" /subsystem:windows)
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
        $<TARGET_FILE_DIR:Engine>
    )
elseif(UNIX)
    add_custom_command(TARGET Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SENTRY_PATH}/lib/linux/libsentry.so"
        "${SENTRY_PATH}/lib/linux/crashpad_handler"
        $<TARGET_FILE_DIR:Engine>
    )
endif()