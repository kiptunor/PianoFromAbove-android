cmake_minimum_required(VERSION 3.6)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(my_app)


option(EXE_TYPE "Build core aplication as executable" ON)



if(UNIX)
    if(NOT TARGET SDL3::SDL3)
        find_package(SDL3 CONFIG)
    endif()

    if(NOT TARGET SDL3::SDL3)
        find_library(SDL3_LIBRARY NAMES "SDL3")
        find_path(SDL3_INCLUDE_DIR NAMES "SDL3/SDL.h")
        add_library(SDL3::SDL3 UNKNOWN IMPORTED)
        set_property(TARGET SDL3::SDL3 PROPERTY IMPORTED_LOCATION "${SDL3_LIBRARY}")
        set_property(TARGET SDL3::SDL3 PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}")
    endif()

    if(NOT TARGET SDL3::SDL3)
        message(FATAL_ERROR "Cannot find SDL3.
            
            Possible ways to fix this:
            - Use a SDL3 Android aar archive, and configure gradle to use it: prefab is required.
            - Add add_subdirectory(path/to/SDL) to your CMake script, and make sure a vendored SDL is present there.
            ")
    endif()
    
    
    if(NOT TARGET SDL3_image::SDL3_image)
        find_package(SDL3_image CONFIG)
    endif()
    
    if(NOT TARGET SDL3_image::SDL3_image)
        find_library(SDL3_IMAGE_LIBRARY NAMES "SDL3_image")
        find_path(SDL3_IMAGE_INCLUDE_DIR NAMES "SDL3_image/SDL_image.h")
        add_library(SDL3_image::SDL3_image UNKNOWN IMPORTED)
        set_property(TARGET SDL3_image::SDL3_image PROPERTY IMPORTED_LOCATION "${SDL3_IMAGE_LIBRARY}")
        set_property(TARGET SDL3_image::SDL3_image PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL3_IMAGE_INCLUDE_DIR}")
    endif()


endif()

set(CMAKE_CXX_STANDARD 17)

if(EXE_TYPE)
    message(STATUS "Building as executable")

    include(ProcessorCount)
    ProcessorCount(NPROC)
    if(NPROC EQUAL 0)
        set(NPROC 4)
    endif()
    
    message(STATUS "Using ${NPROC} parallel LTO jobs")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto=${NPROC}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
    set(CMAKE_CXX_FLAGS "-Ofast")
    set(CMAKE_CXX_FLAGS "-Os")
    set(CMAKE_CXX_FLAGS "-march=native")
    set(CMAKE_CXX_FLAGS "-ffast-math")              # Fast math
    set(CMAKE_CXX_FLAGS "-funroll-loops")           # Loop unrolling
    set(CMAKE_CXX_FLAGS "-ftree-vectorize")         # SIMDvectorization
    set(CMAKE_CXX_FLAGS "-flto")
else()
# Import BASS libs
    add_library(bass SHARED IMPORTED)
    set_target_properties(bass PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../libs/arm64-v8a/libbass.so")

    add_library(bassmidi SHARED IMPORTED)
    set_target_properties(bassmidi PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../libs/arm64-v8a/libbassmidi.so")
endif()


include_directories(".")
include_directories("extern/imgui/")
include_directories("extern/imgui/extra")
include_directories("extern/")



if(EXE_TYPE)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall")
add_compile_options(
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
)
add_executable(nvi-pfa
    extern/imgui/backend_render/imgui_impl_sdl3.cpp
    extern/imgui/backend_render/imgui_impl_sdlrenderer3.cpp
    extern/imgui/imgui.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/extra/imgui_styles.cpp
    extern/imgui/extra/file_dlg/ImGuiFileDialog.cpp
    src/cli.cpp
    src/audio/audio_effects.cpp
    src/audio/playback.cpp
    src/config/config.cpp
    src/nv_midi/MIDI.cpp
    src/nv_midi/list.cpp
    src/config/soundfont_list.cpp
    src/config/midi_list.cpp
    src/logger.cpp
    src/file_helpers.cpp
    src/main.cpp
    src/nv_midi/seq.cpp
    src/nv_midi/utils.cpp
    src/render/render.cpp
    src/render/ui.cpp
)
target_link_libraries(nvi-pfa PRIVATE SDL3::SDL3 SDL3_image::SDL3_image bass bassmidi stdc++fs)
endif()

#add_definitions(-DPLATFORM_ANDROID=0)