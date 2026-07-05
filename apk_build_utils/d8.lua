function main(opts)
    local d8 = path.join(opts.sdk_tools, "d8")
    local args = {"--release", "--lib", opts.android_jar, "--output", opts.output}
    for _, input in ipairs(opts.inputs) do
        table.insert(args, input)
    end
    os.vrunv(d8, args)
end
