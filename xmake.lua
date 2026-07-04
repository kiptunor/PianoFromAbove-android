add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate")





set_toolchains("clang")


target("nvi-pfa")
    if is_plat("android") then
        add_defines("PLATFORM_ANDROID")
        set_kind("shared")
    else

        add_requires("sdl3", {system = true})
        add_requires("sdl3-image", {system = true})
    
        set_kind("binary")

        add_packages(
            "sdl3",
            "sdl3-image"
        )
    end
    
    add_includedirs(
        "extern/imgui/",
        "extern/imgui/extra",
        "extern/",
        "extern/bass_libs"
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
        add_links("bass", "bassmidi")
    end

    if is_plat("android") then
    
        -- Android application packaging
        add_rules("utils.android.app")

        set_values("android.package", "com.qsp.nvpfa")
        set_values("android.versioncode", "1")
        set_values("android.versionname", "1.0")

        add_files("android/AndroidManifest.xml")
        add_files("android/res/**")

        -- Android-specific native libraries
        add_linkdirs("extern/lib/android/arm64-v8a")
        add_links("bass", "bassmidi")
    end