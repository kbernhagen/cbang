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

from __future__ import print_function

import os
import sys
import traceback
from SCons.Script import *
import inspect
import types
import re
import shlex


def CBCheckEnv(ctx, name, require = False):
    ctx.did_show_result = 1

    if name in os.environ: return os.environ[name]
    elif require: raise Exception("Missing environment variable: " + name)


def CBRequireEnv(ctx, name):
    ctx.did_show_result = 1
    return ctx.sconf.CBCheckEnv(name, True)


def CBCheckEnvPath(ctx, name):
    ctx.did_show_result = 1
    paths = ctx.sconf.CBCheckEnv(name)

    existing = []
    if paths:
        for path in paths.split(';'):
            if os.path.isdir(path): existing.append(path)

    return existing


def CBCheckPathWithSuffix(ctx, base, suffixes):
    ctx.did_show_result = 1
    if suffixes is None: return []
    if not type(suffixes) in (list, tuple): suffixes = [suffixes]
    existing = []
    for suffix in suffixes:
        if os.path.isdir(base + suffix): existing.append(base + suffix)
    return existing


def CBCheckHome(ctx, name, inc_suffix = '/include', lib_suffix = '/lib',
                require = False, suffix = '_HOME'):
    ctx.did_show_result = 1
    name = name.upper().replace('-', '_')

    # Include
    user_inc = ctx.sconf.CBCheckEnvPath(name + '_INCLUDE')
    for path in user_inc:
        ctx.env.AppendUnique(CPPPATH = [path])
        require = False

    # Lib path
    user_libpath = ctx.sconf.CBCheckEnvPath(name + '_LIBPATH')
    for path in user_libpath:
        ctx.env.Prepend(LIBPATH = [path])
        require = False

    # Link flags
    linkflags = ctx.sconf.CBCheckEnv(name + '_LINKFLAGS')
    if linkflags: ctx.env.AppendUnique(LINKFLAGS = linkflags.split())

    # Home
    home = ctx.sconf.CBCheckEnv(name + suffix, require)
    if home:
        if not user_inc:
            for path in ctx.sconf.CBCheckPathWithSuffix(home, inc_suffix):
                ctx.env.AppendUnique(CPPPATH = [path])

        if not user_libpath:
            for path in ctx.sconf.CBCheckPathWithSuffix(home, lib_suffix):
                ctx.env.Prepend(LIBPATH = [path])

    return home


def CBRequireHome(ctx, name, inc_suffix = '/include', lib_suffix = '/lib',
                  suffix = '_HOME'):
    ctx.did_show_result = 1
    return ctx.sconf.CBCheckHome(name, inc_suffix, lib_suffix, True, suffix)


def CBCheckLib(ctx, lib, unique = False, append = False, **kwargs):
    ctx.did_show_result = 1

    # Lib path
    libpath = ctx.sconf.CBCheckEnv(lib.upper() + '_LIBPATH')
    if libpath: ctx.env.Prepend(LIBPATH = [libpath])

    # Lib name
    libname = ctx.sconf.CBCheckEnv(lib.upper() + '_LIBNAME')
    if libname: lib = libname

    if ctx.sconf.CheckLib(lib, autoadd = 0, **kwargs):
        if '/' in lib or '\\' in lib: lib = File(lib)
        if unique: ctx.env.PrependUnique(LIBS = [lib])
        else: ctx.env.Prepend(LIBS = [lib])

        return True

    return False


def CBRequireLib(ctx, lib, **kwargs):
    ctx.did_show_result = 1
    if not ctx.sconf.CBCheckLib(lib, **kwargs):
        raise Exception('Need library ' + lib)


def CBCheckHeader(ctx, hdr, **kwargs):
    ctx.did_show_result = 1
    return ctx.sconf.CheckHeader(hdr, **kwargs)


def CBRequireHeader(ctx, hdr, **kwargs):
    ctx.did_show_result = 1
    if not ctx.sconf.CheckHeader(hdr, **kwargs):
        raise Exception('Need header ' + hdr)


def CBCheckCHeader(ctx, hdr, **kwargs):
    ctx.did_show_result = 1
    return ctx.sconf.CheckCHeader(hdr, **kwargs)


def CBRequireCHeader(ctx, hdr, **kwargs):
    ctx.did_show_result = 1
    if not ctx.sconf.CheckCHeader(hdr, **kwargs):
        raise Exception('Need C header ' + hdr)


def CBCheckCXXHeader(ctx, hdr, **kwargs):
    ctx.did_show_result = 1
    return ctx.sconf.CheckCXXHeader(hdr, **kwargs)


def CBRequireCXXHeader(ctx, hdr, **kwargs):
    ctx.did_show_result = 1
    if not ctx.sconf.CheckCXXHeader(hdr, **kwargs):
        raise Exception('Need C++ header ' + hdr)


def CBCheckFunc(ctx, func, **kwargs):
    ctx.did_show_result = 1
    return ctx.sconf.CheckFunc(func, **kwargs)


def CBRequireFunc(ctx, func, **kwargs):
    ctx.did_show_result = 1
    if not ctx.sconf.CheckFunc(func, **kwargs):
        raise Exception('Need Function ' + func)


def CBConfig(ctx, name, required = True, **kwargs):
    ctx.did_show_result = 1
    env = ctx.env
    conf = ctx.sconf

    if name in env.cb_methods:
        ret = False
        try:
            conf.env = env.Clone()
            ret = conf.env.cb_methods[name](conf, **kwargs)
            if ret is None: ret = True

            # Commit changes
            if ret: env.Replace(**conf.env.Dictionary())

        except Exception as e:
            if required: raise
            ctx.Message(str(e))

        finally:
            conf.env = env # Put back master env

        if ret: env.cb_enabled.add(name)
        elif required: raise Exception('Failed to configure ' + name)

        return ret

    elif required:
        raise Exception('Config method not defined for tool ' + name)

    return False


def CBTryLoadTool(env, name, path):
    if name in env.cb_loaded: return True

    if not os.path.exists(str(path) + '/' + name + '/__init__.py'):
        return False

    try:
        env.cb_paths.append(path)
        env.cb_loaded.add(name)
        env.Tool(name, toolpath = [path])
        env.cb_paths.pop()
        return True

    except Exception as e:
        traceback.print_exc()
        env.cb_loaded.remove(name)
        env.cb_paths.pop()
        return False


def CBLoadTool(env, name, paths = []):
    if name in env.cb_loaded: return True

    if hasattr(paths, 'split'): paths = paths.split()
    else: paths = list(paths)
    paths += env.cb_paths

    home_env_var = name.upper().replace('-', '_') + '_HOME'

    home = os.environ.get(home_env_var, None)
    if home is not None: paths.insert(0, home + '/config')

    path = inspect.getfile(inspect.currentframe())
    cd = os.path.dirname(os.path.abspath(path))
    paths.append(cd)

    paths.append('./config')

    for path in paths:
        if env.CBTryLoadTool(name, path): return True

    msg = 'Failed to load tool ' + name + ' from the following paths:'
    for path in paths: msg += '\n  ' + path
    if home is None:
        msg += '\nHave you set ' + home_env_var + '?'

    print(msg)
    raise Exception('Failed to load tool ' + name)


def CBLoadTools(env, tools, paths = []):
    if hasattr(tools, 'split'): tools = tools.split()
    for name in tools: env.CBLoadTool(name, paths)


def CBDefine(env, defs):
    if not type(defs) in (list, tuple): defs = [defs]
    env.AppendUnique(CPPDEFINES = defs)


def CBConfigDef(env, defs):
    if not type(defs) in (list, tuple): defs = [defs]
    env.cb_config_defs.update(defs)


def CBWriteConfigDef(env, path):
    if not os.path.exists(os.path.dirname(path)):
        os.makedirs(os.path.dirname(path))

    with open(path, 'w') as f:
        f.write('// Autogenerated file\n')
        f.write('#pragma once\n\n')

        for var in sorted(env.cb_config_defs):
            if '=' in var:
                name, value = var.split('=', 1)
                f.write('#define %s %s\n' % (name, value))

            else: f.write('#define %s\n' % var)


def CBAddVariables(env, *args):
    env.cb_vars += args


def CBAddTest(env, name, func = None):
    if func is None:
        func = name
        name = func.__name__

    env.cb_tests[name] = func


def CBAddConfigTest(env, name, func):
    env.cb_methods[name] = func


def CBConfigEnabled(env, name):
    return name in env.cb_enabled


def on_config_finish(conf):
    env = conf.env
    for cb in env.cb_finish_cbs: cb(env)
    conf.OrigFinish()
    if env.get('gen_ninja', False): env.GenerateNinja()


updated_csig = set()


def decider_hack(dep, target, prev_ni, *args, **kwargs):
    from hashlib import sha256

    ninfo = dep.get_ninfo();

    # Make sure csigs get updated
    if not hasattr(ninfo, 'csig') or str(dep) not in updated_csig:
        ninfo.csig = sha256(dep.get_contents()).hexdigest()
        updated_csig.add(str(dep))

    # .csig may not exist, because no target was built yet...
    if not hasattr(prev_ni, 'csig'): return True

    # Target file may not exist yet
    if not os.path.exists(str(target.abspath)): return True

    # Some change on source file => update installed one
    if ninfo.csig != prev_ni.csig: return True

    return False


def CBConfigure(env):
    env.Decider(decider_hack)

    env.CBLoadTool('test')
    env.CBLoadTool('ninja')

    conf = Configure(env)

    for name, test in env.cb_tests.items():
        conf.AddTest(name, test)

    # Load config files
    configs = []

    if os.path.exists('default-options.py'):
        configs.append('default-options.py')
    if os.path.exists('options.py'): configs.append('options.py')

    if 'SCONS_OPTIONS' in os.environ:
        options = os.environ['SCONS_OPTIONS']
        if os.path.exists(options): configs.append(options)
        else:
            if '=' not in options:
                print('options file "%s" set in SCONS_OPTIONS does not exist' %
                      options)
                Exit(1)

            env_opts = 'scons-env-options.py'
            if os.path.exists(env_opts): os.unlink(env_opts)
            with open(env_opts, 'w') as f:
                for item in shlex.split(options):
                    n, v = item.split('=', 1)
                    f.write('%s="%s"\n' % (n, v))

            configs.append(env_opts)

    # Load variables
    v = Variables(configs)
    v.AddVariables(*env.cb_vars)
    v.Update(env)

    Help(v.GenerateHelpText(env))

    # Override Finish
    conf.OrigFinish = conf.Finish
    conf.Finish = types.MethodType(on_config_finish, conf)

    for cb in env.cb_configure_cbs: cb(env)

    return conf


def CBDownload(env, target, url):
    try:
        import urllib # Python 3+
    except ImportError:
        import urllib2 as urllib

    sys.stdout.write('Downloading ' + url + '.')
    sys.stdout.flush()

    ftp_proxy = os.getenv('ftp_proxy', None)
    http_proxy = os.getenv('http_proxy', None)

    if ftp_proxy or http_proxy:
        handlers = {}
        if ftp_proxy: handlers['ftp'] = ftp_proxy
        if http_proxy: handlers['http'] = http_proxy

        opener = urllib.build_opener(urllib.ProxyHandler(handlers))
        urllib.install_opener(opener)

    with urllib.urlopen(url) as stream:
        with open(target, 'wb', 0) as f: # Unbuffered
            while stream and f:
                data = stream.read(1024 * 1024)
                if not data: break
                f.write(data)
                sys.stdout.write('.')
                sys.stdout.flush()

        sys.stdout.write('ok\n')
        sys.stdout.flush()


def CBAddConfigFinishCB(env, cb):
    env.cb_finish_cbs.append(cb)


def CBAddConfigureCB(env, cb):
    env.cb_configure_cbs.append(cb)


def CBBuildSetRegex(env, pats):
    if hasattr(pats, 'split'): pats = pats.split()
    return re.compile('^(' + ')|('.join(pats) + ')$')


def generate(env):
    def version_less(a, b):
        return a.major < b.major or (a.major == b.major and a.minor < b.minor)


    class objdict(object):
        def __init__(self, **kwargs):
            self.__dict__ = kwargs

    # Check Python version
    v2 = objdict(major = 2, minor = 7)
    v3 = objdict(major = 3, minor = 1)
    v = sys.version_info

    if (2 < v.major and version_less(v, v3)) or version_less(v, v2):
        raise Exception("C! requires Python %d.%d+ or %d.%d+ found %d.%d." % (
            v2.major, v2.minor, v3.major, v3.minor, v.major, v.minor))

    # Add member variables
    env.cb_loaded = set()
    env.cb_enabled = set()
    env.cb_methods = {}
    env.cb_tests = {}
    env.cb_vars = []
    env.cb_paths = []
    env.cb_finish_cbs = []
    env.cb_configure_cbs = []
    env.cb_config_defs = set()

    # Add methods
    env.AddMethod(CBTryLoadTool)
    env.AddMethod(CBLoadTool)
    env.AddMethod(CBLoadTools)
    env.AddMethod(CBDefine)
    env.AddMethod(CBConfigDef)
    env.AddMethod(CBWriteConfigDef)
    env.AddMethod(CBAddVariables)
    env.AddMethod(CBAddTest)
    env.AddMethod(CBAddConfigTest)
    env.AddMethod(CBConfigEnabled)
    env.AddMethod(CBConfigure)
    env.AddMethod(CBDownload)
    env.AddMethod(CBAddConfigFinishCB)
    env.AddMethod(CBAddConfigureCB)
    env.AddMethod(CBBuildSetRegex)

    # Add tests
    env.CBAddTest(CBCheckEnv)
    env.CBAddTest(CBRequireEnv)
    env.CBAddTest(CBCheckEnvPath)
    env.CBAddTest(CBCheckPathWithSuffix)
    env.CBAddTest(CBCheckHome)
    env.CBAddTest(CBRequireHome)
    env.CBAddTest(CBCheckLib)
    env.CBAddTest(CBRequireLib)
    env.CBAddTest(CBCheckHeader)
    env.CBAddTest(CBRequireHeader)
    env.CBAddTest(CBCheckCHeader)
    env.CBAddTest(CBRequireCHeader)
    env.CBAddTest(CBCheckCXXHeader)
    env.CBAddTest(CBRequireCXXHeader)
    env.CBAddTest(CBCheckFunc)
    env.CBAddTest(CBRequireFunc)
    env.CBAddTest(CBConfig)


def exists(env):
    return 1
