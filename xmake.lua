add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate")



add_requires("sdl3",       {system = true})
add_requires("sdl3-image", {system = true})





set_toolchains("clang")



target("nvi-pfa-ksr")
    
    set_kind("binary")
    add_packages("sdl3", "sdl3-image")

    add_includedirs(
        "extern/kasaria/",
        "extern/imgui/",
        "extern/imgui/extra",
        "extern/"
    )

    add_files(
        "extern/imgui/backend_render/*.cpp",
        "extern/imgui/*.cpp",
        "extern/imgui/extra/file_dlg/*.cpp",
        "extern/imgui/extra/*.cpp",
        "src/audio/*.cpp",
        "src/config/*.cpp",
        "src/nv_midi/*.cpp",
        "src/render/*.cpp",
        "src/*.cpp"
    )

    if is_plat("linux") then
        add_linkdirs("extern/lib/x86_64_linux")
        add_links("kasaria")
    end