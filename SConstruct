# Copyright 2025 Eric Smith
# SPDX-License-Identifier: GPL-3.0-only

env = Environment()

build_dir = 'build'
env['build_dir'] = build_dir

# include build dir in path, necessary for generated sources
env.Append(CPPPATH = '.')
env.Append(CPATH = '.')

SConscript('src/SConscript',
           variant_dir = build_dir,
           duplicate = False,
           exports = 'env' )

# Local Variables:
# mode: python
# End:
