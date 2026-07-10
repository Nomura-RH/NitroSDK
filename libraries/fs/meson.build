subdir('src')

libfs_includes = include_directories('include')

libfs = library('fs',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libfs_includes],
    pic: false,
    sources: [libfs_srcs, fx_const_h]
)

libfs_arm7 = library('fs_arm7',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libfs_includes],
    pic: false,
    sources: [libfs_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libfs
nitrosdk_arm7_libs += libfs_arm7
