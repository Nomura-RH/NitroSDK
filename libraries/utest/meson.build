subdir('src')

libutest = library('utest',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libutest_srcs, fx_const_h]
)

nitrosdk_libs += libutest
