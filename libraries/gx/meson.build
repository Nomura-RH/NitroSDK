subdir('src')

libgx_includes = include_directories('include')

libgx = library('gx',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libgx_includes],
    pic: false,
    sources: [libgx_srcs, fx_const_h],
)

nitrosdk_libs += libgx
