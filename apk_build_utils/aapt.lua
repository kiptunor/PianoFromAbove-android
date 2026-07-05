function main(opts)
    local aapt = path.join(opts.sdk_tools, "aapt")
    local args = {"package", "-f", "-M", opts.manifest, "-I", opts.android_jar, "-F", opts.output}
    if opts.res and os.isdir(opts.res) then
        table.insert(args, "-S")
        table.insert(args, opts.res)
    end
    if opts.assets and os.isdir(opts.assets) then
        table.insert(args, "-A")
        table.insert(args, opts.assets)
    end
    os.vrunv(aapt, args, {curdir = opts.curdir or "."})
end