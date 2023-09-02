subdir('src')

libstubscw = library('stubscw',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libstubscw_srcs, fx_const_h]
)

nitrosdk_libs += libstubscw
