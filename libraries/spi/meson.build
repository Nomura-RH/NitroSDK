subdir('src')

libspi_includes = include_directories('include')

libspi = library('spi',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libspi_includes],
    pic: false,
    sources: [libspi_srcs, fx_const_h]
)

libspi_arm7 = library('spi_arm7',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libspi_includes],
    pic: false,
    sources: [libspi_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libspi
nitrosdk_arm7_libs += libspi_arm7
