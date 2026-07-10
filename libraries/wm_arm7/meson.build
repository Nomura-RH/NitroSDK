subdir('src')

libwm_arm7_includes = include_directories('include')

libwm_arm7 = library('wm_arm7',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libwm_arm7_includes],
    pic: false,
    sources: [libwm_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_arm7_libs += libwm_arm7
