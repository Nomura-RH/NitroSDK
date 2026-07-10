subdir('src')

libwvr = library('wvr',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libwvr_srcs, fx_const_h]
)

libwvr_arm7 = library('wvr_arm7',
    c_pch: nitro_pch,
    include_directories: [libwm_arm7_includes, nitrosdk_includes],
    pic: false,
    sources: [libwvr_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libwvr
nitrosdk_arm7_libs += libwvr_arm7
