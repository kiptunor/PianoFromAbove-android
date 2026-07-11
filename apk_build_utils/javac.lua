function main(opts)
    local target    = opts.target
    local args = {"-d", opts.output, "-cp", opts.classpath}
    for _, src in ipairs(opts.sources) do
        cprint("${green}[Java]:${clear}        ${bright magenta}<%s>${clear} Compiling: %s", target:name(), opts.sources)
        table.insert(args, src)
    end
    os.vrunv("javac", args)
end