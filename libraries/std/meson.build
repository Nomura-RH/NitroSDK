subdir('src')

libstd_includes = include_directories('include')

libstd = library('std',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libstd_includes],
    pic: false,
    sources: [libstd_srcs, fx_const_h]
)

nitrosdk_libs += libstd

