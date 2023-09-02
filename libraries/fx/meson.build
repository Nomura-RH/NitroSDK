subdir('src')

libfx = library('fx',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libfx_srcs, fx_const_h]
)

nitrosdk_libs += libfx
