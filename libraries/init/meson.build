subdir('src')

libinit = library('init',
    c_pch: nitro_pch,
    include_directories: nitrosdk_includes,
    pic: false,
    sources: [libinit_srcs, fx_const_h]
)

crt0_obj = libinit.extract_all_objects(recursive: true)
