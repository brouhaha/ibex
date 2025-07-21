# Copyright 2023-2024 Eric Smith
# SPDX-License-Identifier: GPL-3.0-only

import hashlib
import os
import uuid
from xml.etree import ElementTree as ET

from dll_finder import DllFinder

import SCons.Builder


def make_guid(s: str):
    m = hashlib.shake_128()
    m.update(s.encode('utf-8'))
    return str(uuid.UUID(m.hexdigest(16)))

def get_unique_tag(doc, tag):
    elems = list(doc.iter(tag))
    if len(elems) != 1:
        raise Exception(f'.xws template must have eactly one {tag} element')
    return elems[0]

def get_tag_attr(doc, tag, attr, val):
    elems = [elem for elem in list(doc.iter(tag)) if (elem.get(attr) == val)]
    if len(elems) != 1:
        raise Exception(f'.xws template must have exactly on {tag} elements with attr {attr} == {val}')
    return elems[0]
        

def insert_components(doc, paths):
    dir_app = get_tag_attr(doc, '{http://schemas.microsoft.com/wix/2006/wi}DirectoryRef', 'Id', 'INSTALLDIR')
    dir_qt_platforms = get_tag_attr(doc, '{http://schemas.microsoft.com/wix/2006/wi}DirectoryRef', 'Id', 'qt-platforms-dir')
    f = get_unique_tag(doc, '{http://schemas.microsoft.com/wix/2006/wi}Feature')

    for path in paths:
        base = os.path.basename(path)
        if base == 'qwindows.dll':
            dr = dir_qt_platforms
        else:
            dr = dir_app

        # instert Components into DirectoryRef
        e_comp = ET.SubElement(dr, 'Component')
        e_comp.set('Id', base)
        e_comp.set('Guid', make_guid(base))

        e_file = ET.SubElement(e_comp, 'File')
        e_file.set('Id', base)
        e_file.set('Name', base)
        e_file.set('DiskId', '1')
        e_file.set('Source', path)
        e_file.set('KeyPath', 'yes')

        e_compref = ET.SubElement(f, 'ComponentRef')
        e_compref.set('Id', base)


def insert_metadata(doc, metadata):
    # find Product element and insert value for Version attribute
    for product_elem in doc.iter('{http://schemas.microsoft.com/wix/2006/wi}Product'):
        product_elem.set('Version', metadata['version'])


# one of the "sources" has to be the .wxst file
def wxs_action(target, source, env):
    dll_search_path = env['DLL_SEARCH_PATH']
    other_dlls = env['OTHER_DLLS']

    wxs_template = None
    executable_names = []
    other_file_names = []
    for f in source:
        if type(f) != str:
            f = f.path
        if f.endswith('.wxst'):
            if wxs_template is not None:
                raise Exception('too many wxs template files')
            wxs_template = f
        elif f.endswith('.exe') or f.endswith(".dll"):
            executable_names.append(f)
        else:
            other_file_names.append(f)

    dll_finder = DllFinder(pe_fns = executable_names + other_dlls,
                           dll_search_path = dll_search_path)
    dll_files = dll_finder.recursive_get_dlls()
    dll_paths = [v for v in dll_files.values()]

    files = executable_names + dll_paths + other_file_names

    ET.register_namespace('', 'http://schemas.microsoft.com/wix/2006/wi')
    doc = ET.parse(wxs_template)
    insert_metadata(doc, env['app_metadata'])
    insert_components(doc, files)
    doc.write(target[0].path)


wxs_builder = SCons.Builder.Builder(action = wxs_action,
                                    suffix = '.wxs',
                                    src_suffix = '.wxst')

def generate(env, **kw):
    env.Append(BUILDERS = {'WXS': wxs_builder})

def exists(env):
    return True
