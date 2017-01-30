import sys
import glob
import excons
import excons.tools.zlib as zlib


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

if excons.GetArgument("use-seexpr", 0, int) != 0:
   # TODO
   pass

SConscript("gto/SConstruct")
Import("RequireGto")
cmncusts.append(RequireGto(static=True))

partio_headers = env.Install(excons.OutputBaseDirectory() + "/include", ["src/lib/Partio.h",
                                                                         "src/lib/PartioAttribute.h",
                                                                         "src/lib/PartioIterator.h",
                                                                         "src/lib/PartioVec3.h"])

prjs = [
   {"name": "partio",
    "type": "staticlib",
    "defs": cmndefs + libdefs,
    "cppflags": cmncppflags,
    "srcs": glob.glob("src/lib/core/*.cpp") +
            glob.glob("src/lib/io/*.cpp"),
    "custom": cmncusts}
]

build_opts = """PARTIO OPTIONS
  use-zlib=0|1   : Enable zlib compression.                 [1]
  use-seexpr=0|1 : Enable particle expression using SeExpr. [0]"""
excons.AddHelpOptions(partio=build_opts)

tgts = excons.DeclareTargets(env, prjs)

env.Depends(tgts["partio"], partio_headers)
