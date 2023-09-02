subdir('src')

libctrdg_includes = include_directories('include')

libctrdg = library('ctrdg',
    c_pch: nitro_pch,
    include_directories: [nitrosdk_includes, libctrdg_includes],
    pic: false,
    sources: [libctrdg_srcs, fx_const_h]
)

nitrosdk_libs += libctrdg
