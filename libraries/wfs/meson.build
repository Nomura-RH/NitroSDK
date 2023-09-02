subdir('src')

libwfs = library('wfs',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libwfs_srcs, fx_const_h]
)

nitrosdk_libs += libwfs
