subdir('src')

libmath = library('math',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libmath_srcs, fx_const_h]
)

nitrosdk_libs += libmath
