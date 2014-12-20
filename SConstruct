import excons
import excons.tools.arnold as arnold
import excons.tools.threads as threads
import excons.tools.boost as boost

env = excons.MakeBaseEnv()

static = (int(ARGUMENTS.get("static", "1")) != 0)

# to force static build (or not) in gnet
ARGUMENTS["static"] = ("1" if static else "0")
SConscript("gnet/SConstruct")

customs = [arnold.Require]
if static:
   customs.append(threads.Require)

targets = [
   {"name": "rvdriver",
    "type": "dynamicmodule",
    "ext": arnold.PluginExt(),
    "defs": (["GCORE_STATIC", "GNET_STATIC"] if static else []),
    "incdirs": ["gnet/gcore/include", "gnet/include"],
    "srcs": ["driver/rvdriver.cpp"],
    "libs": ["gcore", "gnet"],
    "custom": customs 
   } 
]

excons.DeclareTargets(env, targets)

Default("rvdriver")