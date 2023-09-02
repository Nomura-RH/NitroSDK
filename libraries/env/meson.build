subdir('src')

libenv = library('env',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libenv_srcs, fx_const_h]
)

nitrosdk_libs += libenv
