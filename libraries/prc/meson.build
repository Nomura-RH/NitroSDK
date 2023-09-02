subdir('src')

libprc = library('prc',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libprc_srcs, fx_const_h]
)

nitrosdk_libs += libprc
