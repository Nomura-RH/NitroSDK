subdir('src')

libmath = library('math',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libmath_srcs, fx_const_h]
)

libmath_arm7 = library('math_arm7',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libmath_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libmath
nitrosdk_arm7_libs += libmath_arm7
