subdir('src')

libmi_includes = include_directories('include')

libmi = library('mi',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libmi_includes],
    pic: false,
    sources: [libmi_srcs, fx_const_h],
)

nitrosdk_libs += libmi
