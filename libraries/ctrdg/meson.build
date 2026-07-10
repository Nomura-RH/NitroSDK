subdir('src')

libctrdg_includes = include_directories('include')

libctrdg = library('ctrdg',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libctrdg_includes],
    pic: false,
    sources: [libctrdg_srcs, fx_const_h]
)

libctrdg_arm7 = library('ctrdg_arm7',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libctrdg_includes],
    pic: false,
    sources: [libctrdg_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libctrdg
nitrosdk_arm7_libs += libctrdg_arm7
