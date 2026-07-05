function main(opts)
    local aapt = path.join(opts.sdk_tools, "aapt")
    local args = {"add", opts.apk}
    for _, file in ipairs(opts.files) do
        table.insert(args, file)
    end
    os.vrunv(aapt, args, {curdir = opts.curdir or "."})
end
