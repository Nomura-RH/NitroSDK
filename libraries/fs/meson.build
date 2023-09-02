subdir('src')

libfs_includes = include_directories('include')

libfs = library('fs',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libfs_includes],
    pic: false,
    sources: [libfs_srcs, fx_const_h]
)

nitrosdk_libs += libfs
