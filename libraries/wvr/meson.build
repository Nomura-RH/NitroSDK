subdir('src')

libwvr = library('wvr',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libwvr_srcs, fx_const_h]
)

nitrosdk_libs += libwvr
