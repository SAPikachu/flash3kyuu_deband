import re

import waflib
from waflib import Utils

APPNAME = "f3kdb"
VERSION = "2.0pre"

top = "."


def options(opt):
    opt.load("compiler_cxx")

    opt.add_option("--libdir", action="store", default="${PREFIX}/lib",
                   help="library installation directory")
    opt.add_option("--includedir", action="store",
                   default="${PREFIX}/include/f3kdb",
                   help="header installation directory")

    opt.add_option("--mode", action="store", default="release",
                   help="the mode to compile in (debug/release)")
    opt.add_option("--shared", action="store_true", default=True,
                   help="build shared libraries (default)")
    opt.add_option("--no-shared", action="store_false", dest="shared",
                   help="do not build shared libraries")
    opt.add_option("--static", action="store_true", default=False,
                   help="build static libraries")
    opt.add_option("--no-static", action="store_false", dest="static",
                   help="do not build static libraries (default)")


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
        conf.env[x.upper()] = val
        conf.msg(x.title() + " library", "yes" if val else "no")

    if (conf.env.SHARED, conf.env.STATIC) == (False, False):
        conf.fatal("Either shared or static library need to be selected.")

    for dir in ["libdir", "includedir"]:
        u = dir.upper()

        conf.env[u] = Utils.subst_vars(conf.options.__dict__[dir], conf.env)
        conf.msg("Setting {0} to".format(u), conf.env[u])

    conf.load("compiler_cxx")
    add_options(["CFLAGS", "CXXFLAGS"],
                ["-fPIC", "-Wall", "-Wextra", "-Wno-unused-parameter",
                 "-Werror", "-std=c++11"])
    add_options(["LINKFLAGS_cshlib",
                 "LINKFLAGS_cprogram",
                 "LINKFLAGS_cxxshlib",
                 "LINKFLAGS_cxxprogram"],
                ["-Wl,-Bsymbolic",
                 "-Wl,-z,noexecstack"])
    if conf.options.mode == "debug":
        add_options(["CFLAGS", "CXXFLAGS"],
                    ["-g", "-ggdb", "-ftrapv"])
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
        target=[bld.path.find_node(x) for x in gen_output_list],
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
                "flash3kyuu_deband_impl_ssse3.cpp",
                "flash3kyuu_deband_impl_sse2.cpp",
                "flash3kyuu_deband_impl_sse4.cpp",
            ],
        ),
        target="objs",
    )
    bld(
        features="cxx",
        source="flash3kyuu_deband_impl_sse2.cpp",
        target="impl-sse2",
        cxxflags=["-msse2"],
    )
    bld(
        features="cxx",
        source="flash3kyuu_deband_impl_ssse3.cpp",
        target="impl-ssse3",
        cxxflags=["-mssse3"],
    )
    bld(
        features="cxx",
        source="flash3kyuu_deband_impl_sse4.cpp",
        target="impl-sse4",
        cxxflags=["-msse4.1"],
    )
    for var, feature in [("SHARED", "cxxshlib"), ("STATIC", "cxxstlib")]:
        if bld.env[var]:
            bld(features="cxx " + feature,
                use=["objs", "impl-sse2", "impl-ssse3", "impl-sse4"],
                target="f3kdb",
                install_path="${LIBDIR}")

    bld.install_files("${INCLUDEDIR}", bld.path.ant_glob(["include/*.h"]))
