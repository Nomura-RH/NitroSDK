subdir('src')

libspi_includes = include_directories('include')

libspi = library('spi',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libspi_includes],
    pic: false,
    sources: [libspi_srcs, fx_const_h]
)

nitrosdk_libs += libspi
