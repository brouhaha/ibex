# Copyright 2024 Eric Smith
# SPDX-License-Identifier: GPL-3.0-only

import functools

import SCons.Builder

def metadata_xml_to_hh(target, source, env):
    with open(str(target[0]), 'w') as f:
        am = env['app_metadata']
        pr = functools.partial(print, file = f, end = '')
        pr('// GENERATED FILE - DO NOT EDIT\n')
        pr('#ifndef APP_METADATA_HH\n')
        pr('#define APP_METADATA_HH\n')
        pr(f'static constexpr char name[] = "{am['name']}";\n')
        pr(f'static constexpr char app_version_string[] = "{am['version']}";\n')
        pr(f'static constexpr char release_type_string[] = "{am['release_type']}";\n')
        pr('#endif // APP_METADATA_HH\n')

metadata_xml_to_hh_builder = SCons.Builder.Builder(suffix = '.hh',
                                                   src_suffix = '.xml',
                                                   action = metadata_xml_to_hh)

def generate(env, **kw):
    env.Append(BUILDERS = {'APP_METADATA_XML_TO_HH': metadata_xml_to_hh_builder})

def exists(env):
    print('exists()')
    read_metadata()
    return True
