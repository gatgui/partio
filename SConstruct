import os
import sys
import excons
import excons.tools.dl as dl
import excons.tools.gl as gl
import excons.tools.glut as glut
import excons.tools.zlib as zlib
import excons.tools.python as python
import excons.tools.glew as glew
import excons.tools.maya as maya

excons.InitGlobals()

# Check for maya first to decide on the Visual Studio version to use on windows
# This needs to be set before creating the environment
build_maya = (maya.Version() != "")
if build_maya:
   maya.SetupMscver()

# Setup SeExpr
def SeExprLibname(static):
  return "SeExpr2"

def SeExprDefines(static):
  return (["SEEXPR_WIN32"] if sys.platform == "win32" else [])

def SeExprExtra(env, static):
  if sys.platform != "win32":
    dl.Require(env)

# Always link to static seexpr library
excons.SetArgument("seexpr-static", 1)
rv = excons.ExternalLibRequire(name="seexpr", libnameFunc=SeExprLibname, definesFunc=SeExprDefines, extraEnvFunc=SeExprExtra)
RequireSeExpr = rv["require"]
# SeExpr2 support requires C++11, this needs to be set before creating the environment
excons.SetArgument("use-c++1", 0 if RequireSeExpr is None else 1)

# Create base build environment
env = excons.MakeBaseEnv()

# Setup zlib (either specify external one with with-zlib* flags or build from source)
def ZlibLibname(static):
   return ("z" if sys.platform != "win32" else ("zlib" if static else "zdll"))

def ZlibDefines(static):
   return ([] if static else ["ZLIB_DLL"])

rv = excons.ExternalLibRequire(name="zlib", libnameFunc=ZlibLibname, definesFunc=ZlibDefines)
if rv["require"] is None:
   # When build from source, always use static version of the library
   excons.PrintOnce("Build zlib from sources ...")
   excons.Call("zlib", imp=["RequireZlib"])
   def ZlibRequire(env):
      RequireZlib(env, static=True)
else:
   ZlibRequire = rv["require"]

cmndefs = []
cmncppflags = ""
cmnincdirs = []
cmnlibdirs = []
cmnlibs = []
cmncusts = [ZlibRequire]
libdefs = ["PARTIO_USE_ZLIB"]
swig_opts = ""

if sys.platform == "win32":
   cmndefs.extend(["PARTIO_WIN32", "_CRT_NONSTDC_NO_DEPRECATE"])
   if excons.warnl != "all":
     cmncppflags += " -wd4267 -wd4244"
else:
   cmncppflags += " -Wno-unused-parameter"

if RequireSeExpr:
   cmndefs.append("PARTIO_SE_ENABLED")
   cmncusts.append(RequireSeExpr)
   swig_opts = "-DPARTIO_SE_ENABLED"

# Setup Gto
#   Don't use zlib in GTO, doesn't seem very stable
excons.SetArgument("gto-use-zlib", 0)
excons.Call("gto", imp=["RequireGto"])

cmncusts.append(RequireGto(static=True))

partio_headers = env.Install(excons.OutputBaseDirectory() + "/include", excons.glob("src/lib/*.h"))

swig_builder = Builder(action='$SWIG -o $TARGET -c++ -python -Wall %s $SOURCE' % swig_opts, suffix='.cpp', src_suffix='.i')
env.Append(BUILDERS={"SwigGen": swig_builder})

def PartioName():
  return "partio"

def PartioPath():
  name = PartioName()
  if sys.platform == "win32":
    libname = name + ".lib"
  else:
    libname = "lib" + name + ".a"
  return excons.OutputBaseDirectory() + "/lib/" + libname

def RequirePartio(env):
  env.Append(CPPDEFINES=cmndefs)
  env.Append(CPPFLAGS=cmncppflags)
  env.Append(CPPPATH=[excons.OutputBaseDirectory() + "/include"] + cmnincdirs)
  excons.Link(env, PartioPath(), static=True, force=True, silent=True)
  env.Append(LIBPATH=cmnlibdirs)
  env.Append(LIBS=cmnlibs)
  for c in cmncusts:
    c(env)

prjs = [
   {"name": "partio",
    "type": "staticlib",
    "alias": "partio-lib",
    "defs": cmndefs + libdefs,
    "cppflags": cmncppflags,
    "incdirs": cmnincdirs,
    "srcs": excons.glob("src/lib/core/*.cpp") +
            excons.glob("src/lib/io/*.cpp") +
            excons.glob("src/lib/io/3rdParty/nextLimit/*.cpp") +
            excons.glob("src/lib/*.cpp"),
    "custom": cmncusts
   },
   {"name": "partattr",
    "type": "program",
    "alias": "partio-tools",
    "srcs": ["src/tools/partattr.cpp"],
    "custom": [RequirePartio]
   },
   {"name": "partinfo",
    "type": "program",
    "alias": "partio-tools",
    "srcs": ["src/tools/partinfo.cpp"],
    "custom": [RequirePartio]
   },
   {"name": "partconv",
    "type": "program",
    "alias": "partio-tools",
    "srcs": ["src/tools/partconv.cpp"],
    "custom": [RequirePartio]
   },
   {"name": "partview",
    "type": "program",
    "alias": "partio-tools",
    "cppflags": ("" if sys.platform == "win32" else " -Wno-deprecated-declarations"),
    "srcs": ["src/tools/partview.cpp"],
    "custom": [RequirePartio, glut.Require, gl.Require]
   }
]

Export("PartioName PartioPath RequirePartio")

# Python
swig = excons.GetArgument("with-swig", None)
if not swig:
   swig = excons.Which("swig")
if swig:
   env["SWIG"] = swig
   gen = env.SwigGen(["src/py/partio_wrap.cpp", "src/py/partio.py"], "src/py/partio.i")
   py_prefix = python.ModulePrefix() + "/" + python.Version()
   prjs.append({"name": "_partio",
                "type": "dynamicmodule",
                "alias": "partio-python",
                "ext": python.ModuleExtension(),
                "prefix": py_prefix,
                "srcs": [gen[0]],
                "custom": [RequirePartio, python.SoftRequire],
                "install": {py_prefix: [gen[1]]}})

# Maya
if build_maya:
   maya_prefix = "maya/" + maya.Version(nice=True)
   # Use regex from gto on windows
   mayadefs = ["GLEW_STATIC"]
   mayaincdirs = ["src/ext/glew-2.0.0/include"]
   mayacppflags = ""
   if sys.platform == "win32":
      mayadefs.append("REGEX_STATIC")
      mayaincdirs.append("gto/regex/src")
   else:
      mayacppflags += " -Wno-unused-parameter"
   prjs.append({"name": "partio4Maya",
                "type": "dynamicmodule",
                "alias": "partio-maya",
                "ext": maya.PluginExt(),
                "prefix": maya_prefix + "/plug-ins",
                "defs": mayadefs,
                "cppflags": mayacppflags,
                "incdirs": mayaincdirs,
                "srcs": excons.glob("contrib/partio4Maya/*.cpp") + ["src/ext/glew-2.0.0/src/glew.c"],
                "custom": [maya.Require, RequirePartio, gl.Require],
                "libs": (["shell32"] if sys.platform == "win32" else []),
                "install": {maya_prefix + "/icons": excons.glob("contrib/partio4Maya/icons/*"),
                            maya_prefix + "/scripts": excons.glob("contrib/partio4Maya/scripts/*")}})

# Tests
tests = filter(lambda x: x.endswith("_main.cpp"), excons.glob("src/tests/*.cpp"))
for test in tests:
   name = os.path.basename(test).replace("_main.cpp", "")
   srcs = excons.glob(test.replace("_main.cpp", "*.cpp"))
   prjs.append({"name": name,
                "type": "program",
                "alias": "partio-tests",
                "prefix": "../tests",
                "incdirs": ["src/lib"],
                "srcs": srcs,
                "custom": [RequirePartio]})
for test in ["makecircle", "makeline", "testcluster", "testse"]:
   prjs.append({"name": test,
                "type": "program",
                "alias": "partio-tests",
                "prefix": "../tests",
                "incdirs": ["src/lib"],
                "srcs": ["src/tests/%s.cpp" % test],
                "custom": [RequirePartio]})

build_opts = """PARTIO OPTIONS
  partio-use-zlib=0|1   : Enable zlib compression. [1]"""
excons.AddHelpOptions(partio=build_opts)

tgts = excons.DeclareTargets(env, prjs)
tgts["partio-headers"] = partio_headers

env.Depends(tgts["partio-lib"], partio_headers)

# Ecosystem

ecoroot = "/" + excons.EcosystemPlatform()

tgtdirs = {"partio-lib": ecoroot + "/lib",
           "partio-headers": ecoroot + "/include",
           "partio-tools": ecoroot + "/bin"}
if swig:
  tgts["partio-python"].append(gen[1])
  tgtdirs["partio-python"] = "%s/lib/python/%s" % (ecoroot, python.Version())
if build_maya:
  tgts["partio-maya-scripts"] = excons.glob("contrib/partio4Maya/scripts/*")
  tgts["partio-maya-icons"] = excons.glob("contrib/partio4Maya/icons/*")
  tgtdirs["partio-maya"] = "%s/maya/%s/plug-ins" % (ecoroot, maya.Version(nice=True))
  tgtdirs["partio-maya-scripts"] = "%s/maya/%s/scripts" % (ecoroot, maya.Version(nice=True))
  tgtdirs["partio-maya-icons"] = "%s/maya/%s/icons" % (ecoroot, maya.Version(nice=True))

excons.EcosystemDist(env, "partio.env", tgtdirs, version="0.9.8")


