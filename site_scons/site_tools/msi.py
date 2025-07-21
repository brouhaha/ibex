# Copyright 2023-2024 Eric Smith
# SPDX-License-Identifier: GPL-3.0-only

import SCons.Builder
import SCons.Scanner
from xml.etree import ElementTree

def wxs_scan(node, env, path):
    tree = ElementTree.parse(str(node))
    root = tree.getroot()

    # find all File elements and collect their Source attributes
    sources = []
    for file in root.iter('{http://schemas.microsoft.com/wix/2006/wi}File'):
        source = file.get('Source')
        if source[0] != '/':
            source = '../../' + source
        sources.append(source)

    return sources
    
wxs_scanner = SCons.Scanner.Scanner(function = wxs_scan,
                      skeys = ['.wxs'])

msi_builder = SCons.Builder.Builder(action = 'wixl $SOURCE',
                                    suffix = '.msi',
                                    src_suffix = '.wxs',
                                    source_scanner = wxs_scanner)

def generate(env, **kw):
    env.Append(BUILDERS = {'MSI': msi_builder})

def exists(env):
    return True
