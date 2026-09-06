subdir('src')

ichneumon_sub_lsf = files('ichneumon_sub.lsf')
ichneumon_sub_lcf_template = files('ichneumon_sub.lcf.template')

ichneumon_sub_lcf = custom_target('ichneumon_sub.lcf',
    output: 'ichneumon_sub.lcf',
    input: [ichneumon_sub_lsf, ichneumon_sub_lcf_template],
    command: [makelcf_exe, '@INPUT@', '@OUTPUT@']
)

ichneumon_sub_link_args = [
    '-w', 'off',
    '-proc', 'arm7tdmi',
    '-nopic',
    '-nopid',
    '-interworking',
    '-map', 'closure,unused',
    '-symtab', 'sort',
    '-msgstyle' ,'gcc',
    '-m', '_start',
    '-sym', 'on',
]

fp_fulli_v4t_le = files('FP_fullI_v4t_LE.a')[0]
fp_fulli_v4t_le_relpath = fs.relative_to(fp_fulli_v4t_le, meson.global_build_root())
ichneumon_sub_lcf_relpath = fs.relative_to(ichneumon_sub_lcf, meson.global_build_root())

ichneumon_sub = executable('ichneumon_sub',
    sources: [
        ichneumon_sub_srcs,
    ],
    c_pch: nitro_pch,
    dependencies: [nitrosdk_arm7_dep],
    link_args: [
        fp_fulli_v4t_le_relpath,
        ichneumon_sub_link_args,
        ichneumon_sub_lcf_relpath,
    ],
    link_depends: ichneumon_sub_lcf,
    native: false
)
