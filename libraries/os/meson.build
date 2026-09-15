subdir('src')


mwrap_libos = get_option('mwrap_libos')
libos_c_args = []
if mwrap_libos != 'default'
    libos_c_args += ['-wrap:sdk', mwrap_libos]
    libos_c_args += ['-USP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS']
endif

libos = library('os',
    c_pch: nitro_pch,
    c_args: libos_c_args,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libos_srcs, fx_const_h]
)

libos_arm7 = library('os_arm7',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libos_arm7_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

nitrosdk_libs += libos
nitrosdk_arm7_libs += libos_arm7
