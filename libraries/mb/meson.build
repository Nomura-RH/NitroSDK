subdir('src')

libmb_includes = include_directories('include')

libmb = library('mb',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libmb_includes],
    pic: false,
    sources: [libmb_srcs, fx_const_h]
)

nitrosdk_libs += libmb
