subdir('src')

libstubsisd = library('stubsisd',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libstubsisd_srcs, fx_const_h]
)

nitrosdk_libs += libstubsisd
