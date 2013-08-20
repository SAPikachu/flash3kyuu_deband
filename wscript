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
                ["-fPIC"])
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
        if getattr(bld.env, var) == "true":
            bld(features="cxx " + feature,
                use=["objs", "impl-sse2", "impl-ssse3", "impl-sse4"],
                target="f3kdb",
                install_path="${LIBDIR}")
