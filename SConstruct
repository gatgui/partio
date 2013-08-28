import os
import sys
import platform

def kernel_version():
    return os.popen("uname -r").read().strip().split('-')[0]

default_cxx = ("g++" if sys.platform != "win32" else "cl")

options = Variables("SConstruct.options")

uos = platform.system()
ver = kernel_version()
arch = platform.machine()

options.AddVariables(('CXX', 'C++ compiler', default_cxx),
                     ('mac', 'Is a mac', uos == 'Darwin'),
                      EnumVariable("TYPE", "Type of build (e.g. optimize,debug)", "optimize",
                                   allowed_values=("profile","optimize","debug")))

variant_basename = '%s-%s-%s' % (uos, ver, arch)

AddOption('--prefix',
          dest='prefix',
          type='string',
          nargs=1,
          action='store',
          default='',
          metavar='DIR',
          help='installation prefix')

if sys.platform == "win32":
    zlib_dir = ARGUMENTS.get("with-zlib", None)
    if zlib_dir:
        zlib_dir = os.path.abspath(zlib_dir)
    zlib_name = "zlib"

    swig_dir = ARGUMENTS.get("with-swig", None)
    if swig_dir:
        swig_dir = os.path.abspath(swig_dir)

    glut_dir = ARGUMENTS.get("with-glut", None)
    if glut_dir:
        glut_dir = os.path.abspath(glut_dir)
else:
    zlib_dir = None
    zlib_name = "z"
    swig_dir = None
    glut_dir = None

if sys.platform == "win32":
    env = Environment(options=options, prefix=GetOption('prefix'), MSVC_VERSION=ARGUMENTS.get("msvc-ver", "10.0"))
else:
    env = Environment(options=options, prefix=GetOption('prefix'))

buildType = env["TYPE"]
variant_basename += "-%s" % buildType
env.Append(CPPDEFINES=["PARTIO_USE_ZLIB"])

# --prefix is typically an absolute path, but we
# should also allow relative paths for convenience.

# prefix is the installation prefix, e.g. /usr
# variant_install_abs is the path to the install root.

prefix = GetOption('prefix') or os.path.join('dist', variant_basename)
if os.path.isabs(prefix):
    variant_install_abs = prefix
else:
    variant_install_abs = os.path.join(Dir(".").abspath, prefix)

# variant_build_abs is the path to the temporary build directory.
variant_build = os.path.join('build', variant_basename)
variant_build_abs = os.path.join(Dir(".").abspath, variant_build)

if env["mac"] == True:
    env.Append(CPPDEFINES=["__DARWIN__"])
    #env.Append(LINKFLAGS=["-m32"])

if default_cxx == "g++":
    if env["TYPE"] == "optimize":
        env.Append(CXXFLAGS=" -fPIC -DNDEBUG -O3 -fno-strict-aliasing -Wall -Werror -Wstrict-aliasing=0 -mfpmath=sse -msse3")
    elif env["TYPE"] == "profile":
        env.Append(CXXFLAGS=" -fPIC -DNDEBUG -O3 -fno-strict-aliasing -Wall -Werror -Wstrict-aliasing=0 -mfpmath=sse -msse3 -g -fno-omit-frame-pointer")
    elif env["TYPE"] == "debug":
        env.Append(CXXFLAGS=" -fPIC -D_DEBUG -O0 -g -Wall -Werror -Wstrict-aliasing=0")
elif default_cxx == "cl":
    env.Append(CXXFLAGS=" /W3 /GR /EHsc /MD")
    env.Append(CPPDEFINES=["_CRT_SECURE_NO_WARNINGS"])
    if env["TYPE"] == "optimize":
        env.Append(CXXFLAGS=" /O2 /DNDEBUG")
    elif env["TYPE"] == "profile":
        env.Append(CXXFLAGS=" /Od /Zi /DNDEBUG")
    elif env["TYPE"] == "debug":
        env.Append(CXXFLAGS=" /Od /Zi /D_DEBUG")


VariantDir(variant_build, '.', duplicate=0)


def GetInstallPath():
    return variant_install_abs

Export("env variant_build variant_build_abs variant_install_abs glut_dir swig_dir zlib_dir zlib_name GetInstallPath")

env.SConscript(variant_build + "/src/lib/SConscript")
env.SConscript(variant_build + "/src/tools/SConscript")
env.SConscript(variant_build + "/src/tests/SConscript")
env.SConscript(variant_build + "/src/doc/SConscript")
env.SConscript(variant_build + "/src/py/SConscript")
env.SConscript(variant_build + "/contrib/partio4Maya/SConscript")
