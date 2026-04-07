cmake_minimum_required(VERSION 3.20)
project(TectonicEngine2)

if(MSVC)
    add_compile_options(/we4013)
endif()

if(UNIX)
    add_compile_options(-Wno-backslash-newline-escape)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- PATHS ----
set(SDL3_PATH ${CMAKE_SOURCE_DIR}/libs/SDL3-3.4.4)
set(GLM_PATH ${CMAKE_SOURCE_DIR}/libs/GLM)
set(STB_IMAGE_PATH ${CMAKE_SOURCE_DIR}/libs/stb_image)
set(GLAD_PATH ${CMAKE_SOURCE_DIR}/libs/glad)
set(BULLET_PATH ${CMAKE_SOURCE_DIR}/libs/bullet)
set(CGLTF_PATH ${CMAKE_SOURCE_DIR}/libs/cgltf)
set(FLTK_PATH ${CMAKE_SOURCE_DIR}/libs/fltk-1.4.4)
set(OPENAL_PATH ${CMAKE_SOURCE_DIR}/libs/openal-soft-1.24.3)
set(MINIMP3_PATH ${CMAKE_SOURCE_DIR}/libs/minimp3)
set(DISCORDRPC_PATH ${CMAKE_SOURCE_DIR}/libs/DiscordRPC)

# ---- Detect 32/64-bit ----
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_FOLDER "x64")
    else()
        set(ARCH_FOLDER "x86")
    endif()
elseif(UNIX)
    set(ARCH_FOLDER "linux")
endif()

# ---- SOURCE FILES ----
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "src/*.h" "src/*.hpp")

# Add GLAD source file specifically
set(GLAD_SRC "${GLAD_PATH}/src/glad.c")

# ---- EXECUTABLE ----
add_library(TectonicEngine2 SHARED ${SRC_FILES} ${HEADER_FILES} ${GLAD_SRC})
target_compile_definitions(TectonicEngine2 PRIVATE GLM_ENABLE_EXPERIMENTAL ENGINE_BUILD)

# ---- INCLUDE DIRECTORIES ----
target_include_directories(TectonicEngine2 PRIVATE
    ${SDL3_PATH}/include
    ${GLM_PATH}/include
    ${STB_IMAGE_PATH}/include
    ${GLAD_PATH}/include
    ${BULLET_PATH}/include
    ${CGLTF_PATH}/include
    ${OPENAL_PATH}/include
    ${MINIMP3_PATH}/include
    ${DISCORDRPC_PATH}/include
    src
)

# ---- LINK LIBRARIES ----
find_package(OpenGL REQUIRED)

if(WIN32)
    target_link_libraries(TectonicEngine2 PRIVATE
        ${SDL3_PATH}/lib/${ARCH_FOLDER}/SDL3.lib
        ${BULLET_PATH}/lib/${ARCH_FOLDER}/Bullet3Dynamics.lib
        ${BULLET_PATH}/lib/${ARCH_FOLDER}/BulletCollision.lib
        ${BULLET_PATH}/lib/${ARCH_FOLDER}/BulletDynamics.lib
        ${BULLET_PATH}/lib/${ARCH_FOLDER}/LinearMath.lib
        ${OPENAL_PATH}/lib/${ARCH_FOLDER}/OpenAL32.lib
        ${DISCORDRPC_PATH}/lib/${ARCH_FOLDER}/discord-rpc.lib
        OpenGL::GL
    )
elseif(UNIX)
    find_package(OpenAL REQUIRED)

    target_link_directories(TectonicEngine2 PRIVATE
        ${SDL3_PATH}/lib/linux
        ${BULLET_PATH}/lib/linux
        ${DISCORDRPC_PATH}/lib/linux
    )

    target_link_libraries(TectonicEngine2 PRIVATE
        SDL3
        OpenAL::OpenAL
        Bullet3Dynamics
        BulletCollision
        BulletDynamics
        LinearMath
        discord-rpc
        OpenGL::GL
        dl
        pthread
        m 
    )
endif()

if(MSVC AND WIN32 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_options(TectonicEngine2 PRIVATE "/SAFESEH:NO")
endif()

# ---- DLL COPY ----
if (WIN32)
    add_custom_command(TARGET TectonicEngine2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_PATH}/lib/${ARCH_FOLDER}/SDL3.dll"
        $<TARGET_FILE_DIR:TectonicEngine2>
    )
endif()

# launcher executable
add_executable(Launcher ${CMAKE_SOURCE_DIR}/launcher/launcher.cpp)

if(WIN32)
    set(LAUNCHER_ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/launcher/resources/launcher.rc")
    set(LAUNCHER_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/launcher/resources/launcher.manifest")

    target_sources(Launcher PRIVATE ${LAUNCHER_ICON_RC} ${LAUNCHER_MANIFEST})
    target_link_options(Launcher PRIVATE "/MANIFEST:EMBED" "/MANIFESTINPUT:${LAUNCHER_MANIFEST}" /subsystem:windows)
    target_link_options(Launcher PRIVATE /subsystem:windows)
endif()

# TConsole
add_executable(TConsole ${CMAKE_SOURCE_DIR}/tools/tconsole/main.cpp)
target_compile_features(TConsole PRIVATE cxx_std_17)

if(WIN32)
    target_compile_definitions(TConsole PRIVATE PLATFORM_WINDOWS)
endif()

target_include_directories(TConsole PRIVATE ${FLTK_PATH}/include)

if(WIN32)
    set_target_properties(TConsole PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_directories(TConsole PRIVATE ${FLTK_PATH}/lib/${ARCH_FOLDER})
    target_link_libraries(TConsole PRIVATE fltk ws2_32 comctl32 gdiplus)
else()
    target_link_directories(TConsole PRIVATE ${FLTK_PATH}/lib/linux)

    target_link_libraries(TConsole PRIVATE
        fltk
        fltk_gl
        fltk_images
        X11
        Xext
        Xinerama
        Xft
        fontconfig
        Xrender
        Xfixes
        Xcursor
        dl
        pthread
        m
    )
endif()