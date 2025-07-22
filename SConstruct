# Copyright 2025 Eric Smith
# SPDX-License-Identifier: GPL-3.0-only

from xml.etree import ElementTree as ET

def read_app_metadata(fn):
    doc = ET.parse(fn)
    root = doc.getroot()
    app_metadata = {}

    name_elem = root.find('name')
    app_metadata['name'] = name_elem.text

    ver_elem = root.find('version')
    major = int(ver_elem.get('major'))
    minor = int(ver_elem.get('minor'))
    patch_str = ver_elem.get('patch')
    if patch_str:
        patch = int(patch_str)
    else:
        patch = 0
    if patch:
        app_metadata['version'] = f'{major}.{minor}.{patch}'
    else:
        app_metadata['version'] = f'{major}.{minor}'

    release_type_elem = root.find('release_type')
    app_metadata['release_type'] = release_type_elem.text

    return app_metadata

conf_file = 'ibex.conf'
vars = Variables(conf_file, ARGUMENTS)
vars.AddVariables(EnumVariable('target',
                               help = 'execution target platform',
                               allowed_values = ('posix', 'win32', 'win64'),
                               default = 'posix',
                               ignorecase = True))
env = Environment(variables = vars)
vars.Save(conf_file, env)
Help(vars.GenerateHelpText(env))

platform = env['PLATFORM']
target = env['target']

build_dir =  '#build/' + target
env['build_dir'] = build_dir

metadata_xml_fn = 'app_metadata.xml'
env['app_metadata'] = read_app_metadata(metadata_xml_fn)
env.Tool('app_metadata')
metadata_header = env.APP_METADATA_XML_TO_HH(build_dir + '/app_metadata.hh',
                                             metadata_xml_fn)
Default(metadata_header)

env['CROSS'] = (target == platform)

env.Append(CPPPATH = build_dir)  # for auto-generated sources or headers

#-----------------------------------------------------------------------------
# executables
#-----------------------------------------------------------------------------

executables = SConscript('src/SConscript',
                         duplicate = False,
                         variant_dir = build_dir,
                         exports = ['env'])

# Local Variables:
# mode: python
# End:
