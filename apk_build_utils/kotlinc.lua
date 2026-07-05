function main(opts)
    local args = {"-d", opts.output, "-classpath", opts.classpath}
    for _, src in ipairs(opts.sources) do
        table.insert(args, src)
    end
    os.vrunv("kotlinc", args)
end