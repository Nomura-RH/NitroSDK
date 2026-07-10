subdir('src')

libinit = library('init',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libinit_srcs, fx_const_h]
)

crt0_obj = libinit.extract_all_objects(recursive: true)

libinit_arm7 = library('init_arm7',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libinit_srcs, fx_const_h],
    dependencies: [nitrosdk_arm7_args_dep]
)

crt0_obj_arm7 = libinit_arm7.extract_all_objects(recursive: true)
