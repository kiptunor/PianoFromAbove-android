function main(opts)
    local apksigner = path.join(opts.sdk_tools, "apksigner")
    os.vrunv(apksigner, {"sign", "--ks", opts.keystore, "--ks-pass", "pass:" .. opts.keystore_pass, "--out", opts.output, "--in", opts.input})
end