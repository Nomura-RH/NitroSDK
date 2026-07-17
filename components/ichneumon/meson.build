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
    '-wrap:sdk', '2.0/sp2p2',
]

FP_fullI_v4t_LE = fs.copyfile('FP_fullI_v4t_LE.a')

ichneumon_sub = executable('ichneumon_sub',
    sources: [
        ichneumon_sub_srcs,
    ],
    c_pch: nitro_pch,
    dependencies: [nitrosdk_arm7_dep],
    link_args: [
        ichneumon_sub_link_args,
        ichneumon_sub_lcf.full_path(),
    ],
    link_with: [
        FP_fullI_v4t_LE,
    ],
    link_depends: ichneumon_sub_lcf,
    native: false
)

