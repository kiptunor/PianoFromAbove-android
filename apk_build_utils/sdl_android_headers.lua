function main(opts)
    local target    = opts.target
    local project   = os.projectdir()
    local aar_cache = path.join(target:autogendir(), "aar_cache")

    local arch      = (opts.arch or get_config("arch") or "arm64-v8a")

    for _, aar_rel in ipairs(opts.aar or {}) do
        local aar      = path.join(project, aar_rel)
        local aar_name = path.basename(aar)
        local aar_dir  = path.join(aar_cache, aar_name)

        if not os.isdir(aar_dir) then
            os.mkdir(aar_dir)
            os.execv("unzip", {"-q", "-o", aar, "-d", aar_dir})
        end

        for _, module_json in ipairs(os.files(path.join(aar_dir, "prefab/modules/*/module.json"))) do
            local module_dir  = path.directory(module_json)
            local module_name = path.basename(module_dir)
            local include_dir = path.join(module_dir, "include")
            if os.isdir(include_dir) then
                target:add("includedirs", include_dir, {public = true})
            end

            local lib_dir = path.join(module_dir, "libs", "android." .. arch)
            if os.isdir(lib_dir) then
                target:add("linkdirs", lib_dir)
                for _, so in ipairs(os.files(path.join(lib_dir, "*.so"))) do
                    local name = path.basename(so)
                    local ext  = path.extension(name)
                    name = name:sub(1, #name - #ext)
                    if name:sub(1, 3) == "lib" then
                        name = name:sub(4)
                    end
                    target:add("links", name)
                end
            end
        end
    end
end

return main
