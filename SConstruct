################################################################################
#                                                                              #
#         This file is part of the C! library.  A.K.A the cbang library.       #
#                                                                              #
#               Copyright (c) 2021-2025, Cauldron Development  Oy              #
#               Copyright (c) 2003-2021, Cauldron Development LLC              #
#                              All rights reserved.                            #
#                                                                              #
#        The C! library is free software: you can redistribute it and/or       #
#       modify it under the terms of the GNU Lesser General Public License     #
#      as published by the Free Software Foundation, either version 2.1 of     #
#              the License, or (at your option) any later version.             #
#                                                                              #
#       The C! library is distributed in the hope that it will be useful,      #
#         but WITHOUT ANY WARRANTY; without even the implied warranty of       #
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      #
#                Lesser General Public License for more details.               #
#                                                                              #
#        You should have received a copy of the GNU Lesser General Public      #
#                License along with the C! library.  If not, see               #
#                        <http://www.gnu.org/licenses/>.                       #
#                                                                              #
#       In addition, BSD licensing may be granted on a case by case basis      #
#       by written permission from at least one of the copyright holders.      #
#          You may request written permission by emailing the authors.         #
#                                                                              #
#                 For information regarding this software email:               #
#                                Joseph Coffland                               #
#                         joseph@cauldrondevelopment.com                       #
#                                                                              #
################################################################################

import os
import platform


# Version
version = '1.7.2'
libversion = '0'


# Setup
env = Environment(ENV = os.environ,
                  TARGET_ARCH = os.environ.get('TARGET_ARCH', 'x86'))
env.Tool('config', toolpath = ['.'])
env.CBAddVariables(
    BoolVariable('v8_compress_pointers', 'Enable pointer compression for libv8',
                 platform.machine().endswith('64')),
    BoolVariable('staticlib', 'Build a static library', True),
    BoolVariable('sharedlib', 'Build a shared library', False),
    ('soname', 'Shared library soname', 'libcbang%s.so' % libversion),
    ('libsuffix', 'Suffix for library path, e.g. "64" to get lib64', ''),
    ('libpath', 'Library installation path, e.g. "/lib64"', '/lib'),
    PathVariable('prefix', 'Install path prefix', '/usr/local',
                 PathVariable.PathAccept),
    ('docdir', 'Path for documentation', '${prefix}/share/doc/cbang'),
    BoolVariable('with_openssl', 'Build with OpenSSL support', True),
    ('force_local', 'List of 3rd party libs to be built locally', ''),
    ('disable_local', 'List of 3rd party libs not to be built locally', ''))
env.CBLoadTools('packager compiler cbang build_info resources')
conf = env.CBConfigure()


# Build Info
author = 'Joseph Coffland <joseph@cauldrondevelopment.com>'
env.Replace(PACKAGE_VERSION   = version)
env.Replace(PACKAGE_AUTHOR    = author)
env.Replace(PACKAGE_ORG       = 'Cauldron Development')
env.Replace(PACKAGE_COPYRIGHT = 'Cauldron Development, 2003-2024')
env.Replace(PACKAGE_HOMEPAGE  = 'https://cauldrondevelopment.com/')
env.Replace(PACKAGE_LICENSE   = 'LGPL-2.1-or-later')
env.Replace(BUILD_INFO_NS     = 'cb::BuildInfo')
env.Replace(RESOURCES_NS      = 'cb')


# Local lib control
force_local = env.get('force_local', '')
if hasattr(force_local, 'split'): force_local = force_local.split()
disable_local = env.get('disable_local', '')
if hasattr(disable_local, 'split'): disable_local = disable_local.split()


# Configure
if not env.GetOption('clean'):
    conf.CBConfig('compiler')
    conf.CBConfig('cbang-deps', with_openssl = env['with_openssl'],
                  with_local_boost = 'boost' not in disable_local)
    env.CBDefine('USING_CBANG') # Using CBANG macro namespace

# Local includes
env.Append(CPPPATH = ['#/include', '#/src', '#/src/boost'])


# Build third-party libs
Export('env conf')
resources_excludes = []
for lib in 'ZLib bzip2 lz4 sqlite3 expat boost libevent re2 libyaml'.split():
    if lib in disable_local: resources_excludes.append('licenses/%s\\.txt' % lib)
    elif not env.CBConfigEnabled(lib) or lib in force_local:
        Default(SConscript(
            'src/%s/SConscript' % lib, variant_dir = 'build/' + lib))
env.Append(RESOURCES_EXCLUDES = resources_excludes)

if 'boost' not in disable_local: env.CBConfigDef('HAVE_LOCAL_BOOST')


# Source
subdirs = [''] + '''
  oauth2 boost comp config db debug dns enum event geom http hw io js json log
  net os parse thread time util ws xml json/schema
'''.split()

if env.CBConfigEnabled('openssl'): subdirs += ['openssl', 'acmev2']
if env.CBConfigEnabled('v8'): subdirs.append('js/v8')
if env.CBConfigEnabled('mariadb'):
    subdirs += ['db/maria', 'api', 'api/arg', 'api/handler', 'api/ws']

if env['PLATFORM'] == 'win32': subdirs.append('os/win')
elif env['PLATFORM'] == 'darwin': subdirs.append('os/osx')
else: subdirs.append('os/lin')

src = []
for dir in subdirs:
    dir = 'src/cbang/' + dir
    src += Glob(dir + '/*.c')
    src += Glob(dir + '/*.cpp')


conf.Finish()


# Create config header
if not COMMAND_LINE_TARGETS: env.CBWriteConfigDef('include/cbang/config.h')


# Build in 'build'
import re
VariantDir('build', 'src', duplicate = False)
src = list(map(lambda path: re.sub(r'^src/', 'build/', str(path)), src))


# Resources
res = env.Resources('build/resources.cpp', ['#/src/resources'])
src.append(res)


# Build Info
info = env.BuildInfo('build/build_info.cpp', [])
if not COMMAND_LINE_TARGETS: AlwaysBuild(info)
src.append(info)


# Build
libs = []
install = []
prefix = env.get('prefix')
libsuffix = env.get('libsuffix')
libpath = env.get('libpath')
if env.get('staticlib'):
    libs.append(env.StaticLibrary('lib/cbang', src))
    install.append(env.Install(dir = '%s/%s%s' % (prefix, libpath, libsuffix),
                               source = libs))

if env.get('sharedlib'):
    shlib = env.SharedLibrary(
        'lib/cbang' + libversion, src, SHLIBVERSION = version,
        SONAME = env.get('soname'))
    libdir = '%s/%s%s' % (prefix, libpath, libsuffix)
    install.append(InstallVersionedLib(dir = libdir, source = shlib))
    libs.append(shlib)

for lib in libs: Default(lib)


# Clean
Clean(libs, 'build lib include config.log cbang-config.pyc package.txt'
      .split() + Glob('config/*.pyc'))


# Install
for dir in subdirs:
    files = Glob('src/cbang/%s/*.h' % dir)
    files += Glob('src/cbang/%s/*.def' % dir)
    dir = prefix + '/include/cbang/' + dir
    install.append(env.Install(dir = dir, source = files))

docs = ['README.md', 'LICENSE']
docdir = env.get('docdir').replace('${prefix}', prefix)
install.append(env.Install(dir = docdir, source = docs))

env.Alias('install', install)


if 'package' in COMMAND_LINE_TARGETS:
    # .deb Package
    if env.GetPackageType() == 'deb':
        arch = env.GetPackageArch()
        pkg = 'libcbang%s_%s_%s.deb' % (libversion, version, arch)
        dev = 'libcbang%s-dev_%s_%s.deb' % (libversion, version, arch)

        env['ENV']['DEB_DEST_DIR'] = '1'
        cmd = env.Command([pkg, dev], libs, 'fakeroot debian/rules binary')
        env.Alias('package', cmd)

        # Write package.txt
        env.WriteStringToFile('package.txt', [pkg, dev])
