subdir('src')

libext = library('ext',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libext_srcs, fx_const_h]
)

nitrosdk_libs += libext
