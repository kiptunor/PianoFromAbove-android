function main(opts)
    local args = {"-d", opts.output, "-cp", opts.classpath}
    for _, src in ipairs(opts.sources) do
        table.insert(args, src)
    end
    os.vrunv("javac", args)
end
