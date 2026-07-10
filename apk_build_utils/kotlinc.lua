function main(opts)
    local args = {"-d", opts.output, "-classpath", opts.classpath}
    for _, src in ipairs(opts.sources) do
        cprint("${green}[Kotlin]:${clear} compiling: %s", path.relative(src))
        table.insert(args, src)
    end
    os.vrunv("kotlinc", args)
end