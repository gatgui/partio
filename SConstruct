import os
import sys
import glob
import excons
import excons.tools.gl as gl
import excons.tools.glut as glut
import excons.tools.zlib as zlib
import excons.tools.python as python


env = excons.MakeBaseEnv()

libdefs = []

cmndefs = []
cmncppflags = ""
cmncusts = []

if sys.platform == "win32":
   cmndefs.extend(["PARTIO_WIN32", "_USE_MATH_DEFINES"])
else:
   cmncppflags += " -Wno-unused-parameter"

if excons.GetArgument("use-zlib", 1, int) != 0:
   cmncusts.append(zlib.Require)
   libdefs.append("PARTIO_USE_ZLIB")

swig_opts = ""

if excons.GetArgument("use-seexpr", 0, int) != 0:
   cmndefs.append("PARTIO_USE_SEEXPR")
   swig_opts = "-DPARTIO_USE_SEEXPR"
   # TODO

SConscript("gto/SConstruct")
Import("RequireGto")
cmncusts.append(RequireGto(static=True))

partio_headers = env.Install(excons.OutputBaseDirectory() + "/include", glob.glob("src/lib/*.h"))

py_prefix = python.ModulePrefix() + "/" + python.Version()

swig_builder = Builder(action='$SWIG -o $TARGET -c++ -python -Wall %s $SOURCE' % swig_opts, suffix='.cpp', src_suffix='.i')
env.Append(BUILDERS={"SwigGen": swig_builder})

prjs = [
   {"name": "partio",
    "type": "staticlib",
    "defs": cmndefs + libdefs,
    "cppflags": cmncppflags,
    "srcs": glob.glob("src/lib/core/*.cpp") +
            glob.glob("src/lib/io/*.cpp") +
            glob.glob("src/lib/*.cpp"),
    "custom": cmncusts
   },
   {"name": "partattr",
    "type": "program",
    "alias": "tools",
    "defs": cmndefs,
    "cppflags": cmncppflags,
    "srcs": ["src/tools/partattr.cpp"],
    "staticlibs": ["partio"],
    "custom": cmncusts
   },
   {"name": "partinfo",
    "type": "program",
    "alias": "tools",
    "defs": cmndefs,
    "cppflags": cmncppflags,
    "srcs": ["src/tools/partinfo.cpp"],
    "staticlibs": ["partio"],
    "custom": cmncusts
   },
   {"name": "partconv",
    "type": "program",
    "alias": "tools",
    "defs": cmndefs,
    "cppflags": cmncppflags,
    "srcs": ["src/tools/partconv.cpp"],
    "staticlibs": ["partio"],
    "custom": cmncusts
   },
   {"name": "partview",
    "type": "program",
    "alias": "tools",
    "defs": cmndefs,
    "cppflags": cmncppflags + ("" if sys.platform != "darwin" else " -Wno-deprecated-declarations"),
    "srcs": ["src/tools/partview.cpp"],
    "staticlibs": ["partio"],
    "custom": cmncusts + [glut.Require, gl.Require]
   }
]

# Python
swig = excons.GetArgument("with-swig", None)
if not swig:
  swig = excons.Which("swig")
if swig:
  gen = env.SwigGen(["src/py/partio_wrap.cpp", "src/py/partio.py"], "src/py/partio.i")
  prjs.append({"name": "_partio",
               "type": "dynamicmodule",
               "alias": "python",
               "ext": python.ModuleExtension(),
               "prefix": py_prefix,
               "srcs": [gen[0]],
               "staticlibs": ["partio"],
               "custom": cmncusts + [python.SoftRequire],
               "install": {py_prefix: [gen[1]]}})
# Tests
tests = filter(lambda x: x.endswith("_main.cpp"), glob.glob("src/tests/*.cpp"))
for test in tests:
  name = os.path.basename(test).replace("_main.cpp", "")
  srcs = glob.glob(test.replace("_main.cpp", "*.cpp"))
  prjs.append({"name": name,
               "type": "program",
               "alias": "tests",
               "prefix": "../tests",
               "defs": cmndefs,
               "cppflags": cmncppflags,
               "incdirs": ["src/lib"],
               "srcs": srcs,
               "staticlibs": ["partio"],
               "custom": cmncusts})
for test in ["makecircle", "makeline", "testcluster"]:
  prjs.append({"name": test,
               "type": "program",
               "alias": "tests",
               "prefix": "../tests",
               "defs": cmndefs,
               "cppflags": cmncppflags,
               "incdirs": ["src/lib"],
               "srcs": ["src/tests/%s.cpp" % test],
               "staticlibs": ["partio"],
               "custom": cmncusts})

build_opts = """PARTIO OPTIONS
  use-zlib=0|1   : Enable zlib compression.                 [1]
  use-seexpr=0|1 : Enable particle expression using SeExpr. [0]"""
excons.AddHelpOptions(partio=build_opts)

tgts = excons.DeclareTargets(env, prjs)

env.Depends(tgts["partio"], partio_headers)
