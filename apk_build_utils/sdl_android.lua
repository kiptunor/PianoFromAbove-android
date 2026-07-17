function main(opts)
    local target = assert(opts.target, "sdl_android: 'target' is required")
    if not target:is_plat("android") then return end

    local ndk           = target:toolchain("ndk")
    local ndk_root      = ndk:config("ndk")
    local sdk_root      = ndk:config("android_sdk")
    local bt_ver        = ndk:config("build_toolver") or "35.0.0"
    local sdk_tools     = path.join(sdk_root, "build-tools", bt_ver)
    local android_api   = opts.android_sdk_version or "36"
    local android_jar   = path.join(sdk_root, "platforms", "android-" .. android_api, "android.jar")
    local arch          = target:arch()
    local project       = os.projectdir()

    local manifest      = path.join(project, opts.manifest)
    local res_dir       = opts.res and path.join(project, opts.res) or nil
    local assets_dir    = opts.assets and path.join(project, opts.assets) or nil
    local keystore      = path.join(project, opts.keystore)
    local keystore_pass = opts.keystore_pass or "android"
    local final_apk     = path.join(target:targetdir(), target:basename() .. ".apk")
    local tmp           = path.absolute(path.join(target:autogendir(), "apk_pack"))

    os.tryrm(tmp)
    os.mkdir(tmp)
    os.mkdir(path.join(tmp, "lib", arch))

    cprint("=============================[${bright cyan}APK Build${clear}]=============================")

    -- 1. Copy built native library
    os.cp(target:targetfile(), path.join(tmp, "lib", arch, "libmain.so"))

    -- 2. Process AARs (extract classes + native .so)
    local all_classes = path.join(tmp, "all_classes")
    os.mkdir(all_classes)
    local classpath_entries = { android_jar }

    local aar_cache = path.join(target:autogendir(), "aar_cache")
    for _, aar_rel in ipairs(opts.aar or {}) do
        local aar      = path.join(project, aar_rel)
        local aar_name = path.basename(aar)
        local aar_dir  = path.join(aar_cache, aar_name)
        if not os.isdir(aar_dir) then
            os.mkdir(aar_dir)
            cprint("${green}[AAR]:${clear} unpacking: %s", aar_name)
            os.execv("unzip", { "-q", "-o", aar, "-d", aar_dir })
        end
        local lib_out = path.join(tmp, "lib", arch)

        -- classes.jar (present in some AARs, e.g. SDL3)
        local aar_jar = path.join(aar_dir, "classes.jar")
        if os.isfile(aar_jar) then
            table.insert(classpath_entries, aar_jar)
            os.vrunv("unzip", { "-q", "-o", aar_jar, "-d", all_classes })
        end

        -- Native .so from prefab-format AAR
        for _, so in ipairs(os.files(path.join(aar_dir, "prefab/modules/*/libs/android." .. arch, "*.so"))) do
            os.cp(so, lib_out)
        end

        -- Native .so from traditional jni-format AAR
        for _, so in ipairs(os.files(path.join(aar_dir, "jni", arch, "*.so"))) do
            os.cp(so, lib_out)
        end
    end

    local classpath = table.concat(classpath_entries, ":")

    -- 3. Compile Kotlin sources
    if opts.kotlin and #opts.kotlin > 0 then
        local kotlin_sources = {}
        for _, src in ipairs(opts.kotlin) do
            table.insert(kotlin_sources, path.join(project, src))
        end
        import("kotlinc")({
            target    = target,
            sources   = kotlin_sources,
            classpath = classpath,
            output    = all_classes,
        })
    end

    -- 4. Compile Java sources
    if opts.java and #opts.java > 0 then
        local java_sources = {}
        for _, pattern in ipairs(opts.java) do
            for _, f in ipairs(os.files(path.join(project, pattern))) do
                table.insert(java_sources, f)
            end
        end
        if #java_sources > 0 then
            import("javac")({
                sources   = java_sources,
                classpath = classpath,
                output    = all_classes,
            })
        end
    end

    -- 5. Convert merged classes to DEX
    -- [Kotlin]: <
    cprint("${green}[D8]:${clear}            ${bright magenta}<%s>${clear} Generating Classes", target:name())
    local dex_out = path.join(tmp, "dex")
    os.mkdir(dex_out)
    local merged_jar = path.join(tmp, "all_classes.jar")
    os.vrunv("jar", { "cf", merged_jar, "-C", all_classes, "." })

    local d8_inputs     = { merged_jar }
    local kotlin_stdlib = "/usr/share/kotlin/lib/kotlin-stdlib.jar"
    if os.isfile(kotlin_stdlib) then
        table.insert(d8_inputs, kotlin_stdlib)
    end

    import("d8")({
        sdk_tools   = sdk_tools,
        android_jar = android_jar,
        output      = dex_out,
        inputs      = d8_inputs,
    })

    local dex_file = path.join(dex_out, "classes.dex")
    if os.isfile(dex_file) then
        os.cp(dex_file, tmp)
    end

    -- 6. Copy additional native libraries
    for _, lib in ipairs(opts.native_libs or {}) do
        local src = path.join(project, lib)
        if os.isfile(src) then
            os.cp(src, path.join(tmp, "lib", arch))
        end
    end

    -- 6b. Find and copy libc++_shared.so from NDK
    local cxx_patterns = {
        path.join(ndk_root, "toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so"),
        path.join(ndk_root, "sources/cxx-stl/llvm-libc++/libs/arm64-v8a/libc++_shared.so"),
    }
    local cxx_files = {}
    for _, p in ipairs(cxx_patterns) do
        cxx_files = os.files(p)
        if #cxx_files > 0 then break end
    end
    if #cxx_files == 0 then
        cxx_files = os.files(path.join(ndk_root, "toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/*/libc++_shared.so"))
    end
    if #cxx_files > 0 then
        os.cp(cxx_files[1], path.join(tmp, "lib", arch))
    end

    -- 7. Package resources with aapt
    cprint("${green}[APK Packing]:${clear}   ${bright magenta}<%s>${clear} Packaging resources", target:name())
    local res_apk = path.join(tmp, "res_only.apk")
    import("aapt")({
        sdk_tools   = sdk_tools,
        manifest    = manifest,
        android_jar = android_jar,
        res         = res_dir,
        assets      = assets_dir,
        output      = res_apk,
        curdir      = tmp,
    })

    -- 8. Add .so files and classes.dex to APK
    local add_files = {}
    for _, so in ipairs(os.files(path.join(tmp, "lib", arch, "*.so"))) do
        cprint("${green}[APK Packing]:${clear}   ${bright magenta}<%s>${clear} Adding Native JNI Libs: %s", target:name(), path.relative(so, tmp))
        table.insert(add_files, path.relative(so, tmp))
    end
    cprint("${green}[APK Packing]:${clear}   ${bright magenta}<%s>${clear} Adding Classes", target:name())
    if os.isfile(path.join(tmp, "classes.dex")) then
        table.insert(add_files, "classes.dex")
    end
    if #add_files > 0 then
        import("aapt_add")({
            sdk_tools = sdk_tools,
            apk       = res_apk,
            files     = add_files,
            curdir    = tmp,
        })
    end

    -- 9. Zipalign
    import("zipalign")({
        sdk_tools = sdk_tools,
        input     = res_apk,
        output    = path.join(tmp, "unsigned.apk"),
        alignment = 4,
    })

    -- 10. Sign APK
    cprint("${green}[APK Finishing]:${clear} ${bright magenta}<%s>${clear} Signing APK", target:name())
    import("apksigner")({
        sdk_tools     = sdk_tools,
        keystore      = keystore,
        keystore_pass = keystore_pass,
        input         = path.join(tmp, "unsigned.apk"),
        output        = final_apk,
    })
end

return main