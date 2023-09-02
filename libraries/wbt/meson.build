subdir('src')

libwbt = library('wbt',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libwbt_srcs, fx_const_h]
)

nitrosdk_libs += libwbt
