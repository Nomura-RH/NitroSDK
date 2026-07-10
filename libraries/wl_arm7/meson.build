subdir('src')

libwl_arm7_includes = include_directories('include')

libwl_arm7 = library('wl_arm7',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, wl_includes, libwl_arm7_includes],
    pic: false,
    sources: [libwl_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_arm7_libs += libwl_arm7

