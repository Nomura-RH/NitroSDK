subdir('src')

libcp = library('cp',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libcp_srcs, fx_const_h]
)

nitrosdk_libs += libcp