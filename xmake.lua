add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate")
set_toolchains("ndk")

if is_plat("linux") then
    add_requires("sdl3", {system = true})
    add_requires("sdl3-image", {system = true})
end

target("nvi-pfa")
    if is_plat("android") then
        add_defines("PLATFORM_ANDROID")
        set_kind("shared")
    else
        set_kind("binary")
        add_packages("sdl3", "sdl3-image")
    end

    add_includedirs(
        "extern/imgui/", "extern/imgui/extra",
        "extern/", "extern/bass_libs"
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
        add_includedirs("src/android/libs/SDL3/prefab/modules/SDL3-Headers/include", {public = true})
        add_includedirs("src/android/libs/SDL3_image/prefab/modules/SDL3_image-shared/include", {public = true})
        add_linkdirs("extern/lib/arm64-v8a")
        add_links("bass", "bassmidi")

        local sdl3_lib_dir = "src/android/libs/SDL3/prefab/modules/SDL3-shared/libs/android." .. (get_config("arch") or "arm64-v8a")
        local sdl3img_lib_dir = "src/android/libs/SDL3_image/prefab/modules/SDL3_image-shared/libs/android." .. (get_config("arch") or "arm64-v8a")
        add_linkdirs(sdl3_lib_dir)
        add_links("SDL3")
        add_linkdirs(sdl3img_lib_dir)
        add_links("SDL3_image")

        after_build(function(target)
            import("apk_build_utils.sdl_android")({
                target = target,
                android_sdk_version = "35",
                manifest = "src/android/AndroidManifest.xml",
                assets = "assets",
                res = "src/android/res",

                package = "com.qsp.nvpfa",
                keystore = "<path/to/keystore.jks>", -- Put here the path to your keystore
                keystore_pass = "<keystore_password>", -- And the keystore password

                aar = {
                    "src/android/libs/SDL3-3.4.12.aar",
                    "src/android/libs/SDL3_image-3.4.4.aar"
                },

                kotlin = {
                    "src/android/MainActivity.kt"
                },

                native_libs = {
                    "extern/lib/arm64-v8a/libbass.so",
                    "extern/lib/arm64-v8a/libbassmidi.so"
                }
            })
        end)

        on_install(function(target)
            if not target:is_plat("android") then return end
            local adb = "/usr/bin/adb"
            local apk = path.join(target:targetdir(), target:basename() .. ".apk")
            assert(os.isfile(apk), "apk file %s not found!", apk)
            os.vrunv(adb, {"install", "-r", apk})
        end)

        on_uninstall(function(target)
            if not target:is_plat("android") then return end
            local adb = "/usr/bin/adb"
            os.vrunv(adb, {"uninstall", "com.qsp.nvpfa"})
        end)

        on_run(function(target)
            if not target:is_plat("android") then return end
            local adb = "/usr/bin/adb"
            os.vrunv(adb, {"shell", "am", "start", "-n", "com.qsp.nvpfa/.NvpfaActivity"})
        end)
    end
