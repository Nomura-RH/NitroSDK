subdir('src')

libmi_includes = include_directories('include')

libmi = library('mi',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libmi_includes],
    pic: false,
    sources: [libmi_srcs, fx_const_h],
)

libmi_arm7 = library('mi_arm7',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libmi_includes],
    pic: false,
    sources: [libmi_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libmi
nitrosdk_arm7_libs += libmi_arm7
