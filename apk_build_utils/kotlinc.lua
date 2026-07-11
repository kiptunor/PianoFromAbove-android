function main(opts)
    local target    = opts.target
    local args = {"-d", opts.output, "-classpath", opts.classpath}
    for _, src in ipairs(opts.sources) do
        cprint("${green}[Kotlin]:${clear}        ${bright magenta}<%s>${clear} Compiling: %s", target:name(), path.relative(src))
        table.insert(args, src)
    end
    os.vrunv("kotlinc", args)
end