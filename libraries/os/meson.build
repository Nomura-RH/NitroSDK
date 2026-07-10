subdir('src')

libos = library('os',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libos_srcs, fx_const_h]
)

libos_arm7 = library('os_arm7',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libos_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libos
nitrosdk_arm7_libs += libos_arm7
