subdir('src')

libpxi = library('pxi',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libpxi_srcs, fx_const_h]
)

libpxi_arm7 = library('pxi_arm7',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libpxi_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libpxi
nitrosdk_arm7_libs += libpxi_arm7
