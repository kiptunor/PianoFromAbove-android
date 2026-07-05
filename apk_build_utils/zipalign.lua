function main(opts)
    local zipalign = path.join(opts.sdk_tools, "zipalign")
    local align = tostring(opts.alignment or 4)
    os.vrunv(zipalign, {"-f", align, opts.input, opts.output})
end