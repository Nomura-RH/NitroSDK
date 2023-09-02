subdir('src')

libwm_includes = include_directories('include')

libwm = library('wm',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libwm_includes],
    pic: false,
    sources: [libwm_srcs, fx_const_h]
)

nitrosdk_libs += libwm
