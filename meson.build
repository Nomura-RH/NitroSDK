project('NitroSDK', ['c', 'nasm'],
    default_options: {
        'default_library': 'static',
        'prefer_static':    true
    },
    meson_version: '>=1.3.0',
    version: '4.2.30001'
)

fs = import('fs')

buildtype = get_option('buildtype')
cc = meson.get_compiler('c')

############################################################
###                    COMPILER FLAGS                    ###
############################################################

if not meson.is_subproject() and buildtype != 'plain'
c_args = [
    '-O4,p',
    '-proc', 'arm946e',
    '-enum', 'int',
    '-lang', 'c99',
    '-Cpp_exceptions', 'off',
    '-gccext,on',
    '-msgstyle', 'gcc',
    '-ipa', 'file',
    '-interworking',
    '-inline', 'on,noauto',
    '-char', 'signed',
    '-nosyspath',
    '-stdinc'
]
else
c_args = []
endif

mwrap_libos = get_option('mwrap_libos')
libos_c_args = c_args
if mwrap_libos != 'default'
    libos_c_args += ['-wrap:sdk', mwrap_libos]
    libos_c_args += ['-USP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS']
endif

mwrap_libcard = get_option('mwrap_libcard')
libcard_c_args = c_args
if mwrap_libcard != 'default'
    libcard_c_args += ['-wrap:sdk', mwrap_libcard]
    libcard_c_args += ['-USP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS']
endif

mwrap_ver = get_option('mwrap_ver')
if mwrap_ver != 'default'
    c_args += ['-wrap:ver', mwrap_ver]
endif

mwrap_sdk = get_option('mwrap_sdk')
if mwrap_sdk != 'default'
  c_args += ['-wrap:sdk', mwrap_sdk]
endif

if get_option('sp1p3_conditionals')
    c_args += '-DSP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS'
endif

if get_option('patch3')
    c_args += '-DSDK_PATCH3'
endif

if get_option('hg_card_spi')
    c_args += '-DHG_CARD_SPI'
endif

add_project_arguments(c_args,
    language: 'c',
    native: false
)

nitrosdk_args = [
    '-DSDK_CW_FORCE_EXPORT_SUPPORT',
    '-DSDK_TS',
    '-DSDK_4M',
    '-DSDK_ARM9',
    '-DSDK_CW',
    '-DSDK_FINALROM',
    '-DSDK_CODE_ARM',
    '-DNNS_FINALROM'
]

nitrosdk_arm7_args = [
    '-proc', 'arm7tdmi',
    '-USDK_ARM9',
    '-DSDK_ARM7',
    '-DSDK_WIRELESS_IN_VRAM',
    '-DSDK_NOINIT',
    '-O4,s',
    '-sym', 'on',
    '-fp', 'soft',
]

mwrap_arm7 = get_option('mwrap_arm7')
if mwrap_arm7 != 'default'
    nitrosdk_arm7_args += ['-wrap:sdk', mwrap_arm7]
    nitrosdk_arm7_args += ['-USP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS']
endif

if get_option('patch3')
    nitrosdk_arm7_args += '-DSDK_PATCH3'
endif

if get_option('hg_card_spi')
    nitrosdk_arm7_args += '-DHG_CARD_SPI'
endif

nitrosdk_arm7_args_dep = declare_dependency(
  compile_args: nitrosdk_arm7_args
)

add_project_arguments(nitrosdk_args,
    language: 'c',
    native: false
)

############################################################
###                  GENERATED HEADERS                   ###
############################################################
subdir('gen')
nitro_pch = meson.current_source_dir() / 'include/pch/nitro_pch.h'

############################################################
###                    INCLUDE PATHS                     ###
############################################################
nitrosdk_includes = include_directories('gen', 'include')
include_nitro = include_directories('include' / 'nitro')
wl_includes = include_directories('include' / 'nitro_wl' / 'ARM7')

############################################################
###                      LIBRARIES                       ###
############################################################
subdir('libraries')

############################################################
###                      SPECFILES                       ###
############################################################
subdir('specfiles')

############################################################
###                        TOOLS                         ###
############################################################
subdir('tools')

nitrosdk_arm7_dep = declare_dependency(
    compile_args: nitrosdk_args + nitrosdk_arm7_args,
    include_directories: nitrosdk_includes,
    link_with: nitrosdk_arm7_libs,
    objects: crt0_obj_arm7,
    sources: fx_const_h
)

############################################################
###                ARM7 PREBUILT BINARIES                ###
############################################################
subdir('components')

############################################################
###                    FOR CONSUMERS                     ###
############################################################

nitrosdk_dep = declare_dependency(
    compile_args: nitrosdk_args,
    include_directories: nitrosdk_includes,
    link_with: nitrosdk_libs + nitrosdk_prebuilt,
    objects: crt0_obj,
    sources: fx_const_h
)

meson.override_dependency('NitroSDK', nitrosdk_dep)
