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
                     EnumVariable("TYPE", "Type of build", "optimize",
                                  allowed_values=("profile", "optimize", "debug")),
                     ('SWIG', 'swig program path', ''),
                     ('ZLIB_ROOT', 'zlib prefix', ''),
                     ('ZLIB_INC_DIR', 'zlib includes path ($ZLIB_ROOT/include)', ''),
                     ('ZLIB_LIB_DIR', 'zlib library path ($ZLIB_ROOT/lib)', ''),
                     ('ZLIB_LIB_NAME', 'zlib library name (by default, "zlib" on windows or "z" otherwise)', ('zlib' if sys.platform == 'win32' else 'z')),
                     ('GLUT_ROOT', 'GLUT prefix', ''),
                     ('GLUT_INC_DIR', 'GLUT includes path ($GLUT_ROOT/include)', ''),
                     ('GLUT_LIB_DIR', 'GLUT library path ($GLUT_ROOT/lib)', ''),
                     ('GLUT_LIB_NAME', 'GLUT library name (by default "glut64" on windows or "glut" otherwise', ('glut64' if sys.platform == 'win32' else 'glut')),
                     ('MAYA_VERSION', 'Maya version', '2013'),
                     ('MAYA_ROOT', 'Maya path', ''),
                     ('MAYA_INC_DIR', 'Maya includes path', ''),
                     ('MAYA_LIB_DIR', 'Maya libraries path', ''),
                     ('MSVC_VERSION', 'Visual Studio compiler version (windows only)', '10.0'),
                     BoolVariable('WITH_PYTHON', 'Build python binding', 'yes'),
                     BoolVariable('WITH_TOOLS', 'Build command line tools', 'yes'),
                     BoolVariable('WITH_MAYA', 'Build maya plugins', 'no'),
                     BoolVariable('WITH_TESTS', 'Build partio library tests', 'no'),
                     BoolVariable('WITH_DOCS', 'Generate documenation', 'no'))

variant_basename = '%s-%s-%s' % (uos, ver, arch)

AddOption('--prefix',
          dest='prefix',
          type='string',
          nargs=1,
          action='store',
          default='',
          metavar='DIR',
          help='installation prefix')

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
    env.Append(CXXFLAGS=" /W3 /GR /EHsc")
    env.Append(CPPDEFINES=["_CRT_SECURE_NO_WARNINGS"])
    if env["TYPE"] == "optimize":
        env.Append(CXXFLAGS=" /O2 /MD /DNDEBUG")
        env.Append(LINKFLAGS=" /release /incremental:no /opt:ref /opt:icf")
    elif env["TYPE"] == "profile":
        env.Append(CXXFLAGS=" /Od /Zi /Gm /MD /DNDEBUG")
        env.Append(LINKFLAGS=" /release /incremental /opt:noref /opt:noicf")
    elif env["TYPE"] == "debug":
        env.Append(CXXFLAGS=" /Od /Zi /Gm /MDd /D_DEBUG")
        env.Append(LINKFLAGS=" /debug /incremental")


VariantDir(variant_build, '.', duplicate=0)


def GetInstallPath():
    return variant_install_abs

Export("env variant_build variant_build_abs variant_install_abs GetInstallPath")

env.SConscript(variant_build + "/src/lib/SConscript")
if env["WITH_TOOLS"]:
    env.SConscript(variant_build + "/src/tools/SConscript")
if env["WITH_PYTHON"]:
    env.SConscript(variant_build + "/src/py/SConscript")
if env["WITH_MAYA"]:
    env.SConscript(variant_build + "/contrib/partio4Maya/SConscript")
if env["WITH_TESTS"]:
    env.SConscript(variant_build + "/src/tests/SConscript")
if env["WITH_DOCS"]:
    env.SConscript(variant_build + "/src/doc/SConscript")

options.Save("SConstruct.options", env)

Help(options.GenerateHelpText(env))

