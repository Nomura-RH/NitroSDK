subdir('src')

librtc = library('rtc',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [librtc_srcs, fx_const_h]
)

librtc_arm7 = library('rtc_arm7',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [librtc_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += librtc
nitrosdk_arm7_libs += librtc_arm7
