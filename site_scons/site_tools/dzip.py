# Copyright 2023-2024 Eric Smith
# SPDX-License-Identifier: GPL-3.0-only

import os
import zipfile

from dll_finder import DllFinder

import SCons.Builder

def dzip_action(target, source, env):
    dll_search_path = env['DLL_SEARCH_PATH']
    other_dlls = env['OTHER_DLLS']

    executable_names = []
    other_file_names = []
    for f in source:
        if type(f) != str:
            f = f.path
        if f.endswith('.exe') or f.endswith(".dll"):
            executable_names.append(f)
        else:
            other_file_names.append(f)

    dll_finder = DllFinder(pe_fns = executable_names + other_dlls,
                           dll_search_path = dll_search_path)
    dll_files = dll_finder.recursive_get_dlls()
    dll_paths = [v for v in dll_files.values()]

    files = executable_names + dll_paths + other_file_names

    zip_path = target[0].get_abspath()
    z = zipfile.ZipFile(zip_path, 'w')
    dir_name = os.path.basename(zip_path)
    if dir_name.endswith('.zip'):
        dir_name = dir_name[:-4]
    z.mkdir(dir_name)
    for f in files:
        z.write(f, dir_name + '/' + os.path.basename(f))

dzip_builder = SCons.Builder.Builder(action = dzip_action,
                                    suffix = '.zip')

def generate(env, **kw):
    env.Append(BUILDERS = {'DZIP': dzip_builder})

def exists(env):
    return True
