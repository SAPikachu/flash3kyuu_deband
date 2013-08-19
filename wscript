import re

import waflib

APPNAME = "f3kdb"
VERSION = "2.0pre"

top = "."


def options(opt):
    opt.load("compiler_cxx")

    opt.add_option("--mode", action="store", default="release",
                   help="the mode to compile in (debug/release)")
    opt.add_option("--shared", action="store", default="true",
                   help="build shared libraries (true/false)")
    opt.add_option("--static", action="store", default="false",
                   help="build static libraries (true/false)")


def _check_cxx(conf, feature, fragment, mandatory=False):
    conf.check_cxx(
        msg=" - " + feature,
        define_name="HAVE_" + feature.replace(" ", "_").upper(),
        fragment=fragment,
        mandatory=mandatory,
    )


def configure(conf):
    def add_options(flags, options):
        for flag in flags:
            conf.env.append_unique(flag, options)

    for x in ["shared", "static"]:
        val = conf.options.__dict__[x]

        if not val in ["true", "false"]:
            conf.fatal("--{0} must be either true or false.".format(x))
        else:
            u = x.upper()

            conf.env[u] = val

    if (conf.env.SHARED, conf.env.STATIC) == ("false", "false"):
        conf.fatal("--static and --shared cannot both be false.")

    conf.load("compiler_cxx")
    add_options(["CFLAGS", "CXXFLAGS"],
                ["-fPIC", "-mavx"])
    add_options(["CFLAGS", "CXXFLAGS"],
                ["-Wall", "-Werror", "-std=c++11"])
    add_options(["LINKFLAGS_cshlib",
                 "LINKFLAGS_cprogram",
                 "LINKFLAGS_cxxshlib",
                 "LINKFLAGS_cxxprogram"],
                ["-Wl,-Bsymbolic",
                 "-Wl,-z,noexecstack"])
    if conf.options.mode == "debug":
        add_options(["CFLAGS", "CXXFLAGS"],
                    ["-DVS_CORE_DEBUG", "-g", "-ggdb", "-ftrapv"])
    elif conf.options.mode == "release":
        add_options(["CFLAGS", "CXXFLAGS"],
                    ["-O3"])
    else:
        conf.fatal("--mode must be either debug or release.")

    _check_cxx(
        conf,
        "alignas",
        "int main() { alignas(8) int x = 0; return x; }",
        mandatory=True,
    )

    conf.find_program("python3", var="PYTHON3")


def build(bld):
    gen_output = bld.cmd_and_log(
        [bld.env["PYTHON3"], "gen_filter_def.py", "--list-outputs"],
        quiet=waflib.Context.BOTH,
    )
    gen_output_list = re.split(r"\s+", gen_output.strip(), flags=re.S)

    bld(
        rule="${PYTHON3} ${SRC[0].abspath()}",
        source="gen_filter_def.py",
        target=map(bld.path.find_node, gen_output_list),
        cwd=bld.path.abspath(),
    )
    bld(
        features="cxx",
        source=bld.path.ant_glob(
            ["*.cpp", "vapoursynth/*.cpp"],
            excl=[
                "dllmain.cpp",
                "icc_override.cpp",
                "stdafx.cpp",
                "debug_dump.cpp",
            ],
        ),
        target="objs",
    )
    if bld.env.SHARED == "true":
        bld(features="cxx cxxshlib",
            use=["objs"],
            target="f3kdb",
            install_path="${LIBDIR}")

    if bld.env.STATIC == "true":
        bld(features="cxx cxxstlib",
            use=["objs"],
            target="f3kdb",
            install_path="${LIBDIR}")
