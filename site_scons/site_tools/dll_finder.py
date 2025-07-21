#!/usr/bin/env python3
# dllfinder: Given one or more Windows executables, get paths of necessary DLLs.
__version__ = '1.0'
__copyright__ = 'Copyright 2023 Eric Smith'
# SPDX-License-Identifier: GPL-3.0-only
__license__ = 'GPL 3.0 only'

# This program is free software: you can redistribute it and/or modify
# it under the terms of version 3 of the GNU General Public License
# as published by the Free Software Foundation.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

__author__ = 'Eric Smith'
__email__ = 'spacewar@gmail.com'
__description__ = 'dll_finder'
__url__ = 'https://github.com/brouhaha/dll_finder'

import sys
__min_python_version__ = '3.11'
if sys.version_info < tuple([int(n) for n in __min_python_version__.split('.')]):
    sys.exit("Python version %s or later is required.\n" % __min_python_version__)

__all__ = ['__version__', '__copyright__', '__author__', '__email__',
           '__license__', '__description__', '__url__',
           'DllFinder']

import os
import os.path
import re
import subprocess

predef_excluded_dll_names = ['ADVAPI32.dll',
                             'api-ms-win-core-synch-l1-2-0.dll',
                             'api-ms-win-core-winrt-l1-1-0.dll',
                             'api-ms-win-core-winrt-string-l1-1-0.dll',
                             'AUTHZ.dll',
                             'comdlg32.dll',
                             'd3d9.dll',
                             'd3d11.dll',
                             'd3d12.dll',
                             'DWrite.dll',
                             'dwmapi.dll',
                             'dxgi.dll',
                             'GDI32.dll',
                             'IMM32.dll',
                             'KERNEL32.dll',
                             'MPR.dll',
                             'msvcrt.dll',
                             'NETAPI32.dll',
                             'ole32.dll',
                             'OLEAUT32.dll',
                             'SETUPAPI.dll',
                             'SHCORE.dll',
                             'SHELL32.dll',
                             'SHLWAPI.dll',
                             'USER32.dll',
                             'USERENV.dll',
                             'UxTheme.dll',
                             'VERSION.dll',
                             'WINMM.dll',
                             'WS2_32.dll',
                             'WTSAPI32.dll']

dll_re = re.compile('^\\s*DLL Name: (\\S*)$', flags = re.MULTILINE)

class DllFinder:
    def __init__(self,
                 pe_fns: list[str],
                 dll_search_path: list[str],
                 excluded_dlls: list[str] = []):
        self._trace = []
        self._search_dirs = dll_search_path
        self._excluded_dlls = set([dn.lower() for dn in predef_excluded_dll_names])
        self._dlls = { }   # dict, key = short name, value = path

        for pe_fn in pe_fns:
            if pe_fn.endswith('.dll') and not '/' in pe_fn:
                pe_path = self._add_dll(pe_fn)
            else:
                self._trace.append(pe_fn)

    def _find_dll_path(self, dll_fn):
        for dir in self._search_dirs:
            for (dirpath, dirnames, filenames) in os.walk(dir, followlinks = True):
                if dll_fn in filenames:
                    return os.path.join(dirpath, dll_fn)
        return None

    def _add_dll(self, dll_name):
        if dll_name.lower() in self._excluded_dlls:
            return
        if dll_name in self._dlls:
            return

        dll_path = self._find_dll_path(dll_name)
        if dll_path is None:
            print(f"ERROR: can't find {dll_name}")
            return

        self._trace.append(dll_path)
        self._dlls[dll_name] = dll_path

    # This uses subprocess to invoke the objdump executable.
    # It might be better to access the PE files directly,
    # perhaps using PEReader: https://github.com/matthewPeart/PEReader
    def _pe_get_dll_names(self, pe_fn):
        result = subprocess.run(['objdump', '--private-headers', pe_fn],
                                encoding = 'utf-8',
                                capture_output = True)
        if result.returncode != 0:
            raise Exception(f'objdump return code {result.returncode}')

        spos = 0
        dll_names = []
        while True:
            match = dll_re.search(result.stdout, spos)
            if match is None:
                break
            spos = match.end()
            dll_names.append(match.group(1))
        return dll_names

    def recursive_get_dlls(self):
        while len(self._trace):
            pe_fn = self._trace.pop()
            pe_dll_names = self._pe_get_dll_names(pe_fn)
            for pe_dll_name in pe_dll_names:
                self._add_dll(pe_dll_name)
        return self._dlls
                    

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('pe',
                        type = str,
                        nargs = '+')

    parser.add_argument('-d', '--dll_directory',
                        type = str,
                        action = 'append',
                        help = 'dll search directory')

    parser.add_argument('-e', '--exclude_dll',
                        type = str,
                        action = 'append',
                        help = 'exclude dll')

    args = parser.parse_args()

    finder = DllFinder(pe_fns = args.pe,
                       dll_search_path = args.dll_directory,
                       excluded_dlls = args.exclude_dll)

    dlls = finder.recursive_get_dlls()

    for dll in dlls.values():
        print(dll)


if __name__ == '__main__':
    import argparse
    main()



# Examples:
"""dll_finder.py -d /usr/x86_64-w64-mingw32/sys-root/mingw \
                 build/win64/foo.exe \
                 build/win64/bar.exe \
                 qwindows.dll"""

"""dll_finder.py -d /usr/i686-w64-mingw32/sys-root/mingw \
                 build/win32/foo.exe \
                 build/win32/bar.exe \
                 qwindows.dll"""
