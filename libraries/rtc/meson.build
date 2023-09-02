subdir('src')

librtc = library('rtc',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [librtc_srcs, fx_const_h]
)

nitrosdk_libs += librtc
