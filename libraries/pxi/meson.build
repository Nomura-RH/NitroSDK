subdir('src')

libpxi = library('pxi',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libpxi_srcs, fx_const_h]
)

nitrosdk_libs += libpxi
