
def generate_output():
    p = FilterParam
    print(generate_definition("flash3kyuu_deband",
        p("c", "child", optional=False, has_field=False),
        p("i", "range"),
        p("i", "Y", c_type="unsigned short"),
        p("i", "Cb", c_type="unsigned short"),
        p("i", "Cr", c_type="unsigned short"),
        p("i", "grainY"),
        p("i", "grainC"),
        p("i", "sample_mode"),
        p("i", "seed"),
        p("b", "blur_first"),
        p("b", "dynamic_grain"),
        p("i", "opt"),
        p("b", "mt"),
        p("i", "dither_algo", c_type="DITHER_ALGORITHM"),
        p("b", "keep_tv_range"),
        p("i", "input_mode", c_type="PIXEL_MODE"),
        p("i", "input_depth"),
        p("i", "output_mode", c_type="PIXEL_MODE"),
        p("i", "output_depth"),
        p("i", "random_algo_ref", c_type="RANDOM_ALGORITHM"),
        p("i", "random_algo_grain", c_type="RANDOM_ALGORITHM"),
    ))

PARAM_TYPES = {
    "c": "PClip",
    "b": "bool",
    "i": "int",
    "s": "const char*",
}

class FilterParam:
    def __init__(
            self, 
            type, 
            name, 
            field_name=None,
            c_type=None, 
            optional=True,
            has_field=True):
        if type not in PARAM_TYPES.keys():
            raise ValueError("Type {} is not supported.".format(type))

        self.type = type
        self.name = name
        self.field_name = field_name or name
        self.c_type = c_type or PARAM_TYPES[type]
        self.custom_c_type = c_type is not None
        self.optional = optional
        self.has_field = has_field

OUTPUT_TEMPLATE = """
#pragma once

/*************************
 * Script generated code *
 *     Do not modify     *
 *************************/

#include "avisynth.h"

#include "constants.h"

static const char* {filter_name_u}_AVS_PARAMS = "{avs_params}";

class {filter_name}_parameter_storage_t
{{
protected:

    {class_field_def}

public:

    {filter_name}_parameter_storage_t(const {filter_name}_parameter_storage_t& o)
    {{
        {class_field_copy}
    }}

    {filter_name}_parameter_storage_t( {init_param_list_with_field_func_def} )
    {{
        {class_field_init}
    }}
}};

typedef struct _{filter_name_u}_RAW_ARGS
{{
    AVSValue {init_param_list};
}} {filter_name_u}_RAW_ARGS;

#define {filter_name_u}_ARG_INDEX(name) (offsetof({filter_name_u}_RAW_ARGS, name) / sizeof(AVSValue))

#define {filter_name_u}_ARG(name) args[{filter_name_u}_ARG_INDEX(name)]

#define {filter_name_u}_CREATE_CLASS(klass) new klass( {init_param_list_without_field_invoke}, {filter_name}_parameter_storage_t( {init_param_list_with_field_invoke} ) )

#ifdef {filter_name_u}_SIMPLE_MACRO_NAME

#ifdef SIMPLE_MACRO_NAME
#error Simple macro name has already been defined for SIMPLE_MACRO_NAME
#endif

#define SIMPLE_MACRO_NAME {filter_name}

#define ARG {filter_name_u}_ARG

#define CREATE_CLASS {filter_name_u}_CREATE_CLASS

#endif

"""

def build_avs_params(params):
    def get_param(param):
        return param.optional and '[{0.name}]{0.type}'.format(param) or param.type

    return ''.join([get_param(x) for x in params])

def build_init_param_list_invoke(params, predicate=lambda x:True):
    return ", ".join([x.custom_c_type and "({}){}".format(x.c_type, x.field_name) or x.field_name for x in params if predicate(x)])

def build_declaration_list(params, name_prefix='', predicate=lambda x:True):
    return ["{} {}{}".format(x.c_type, name_prefix, x.field_name) for x in params if predicate(x)]

def build_init_param_list_func_def(params, predicate=lambda x: True):
    return ", ".join(build_declaration_list(params, predicate=predicate))

def build_class_field_def(params):
    return "\n    ".join([x + "; " for x in build_declaration_list(params, "_", lambda x: x.has_field)])

def build_class_field_init(params):
    return "\n        ".join(["_{0} = {0}; ".format(x.field_name) for x in params if x.has_field])

def build_class_field_copy(params):
    return "\n        ".join(["_{0} = o._{0}; ".format(x.field_name) for x in params if x.has_field])

def generate_definition(filter_name, *params):
   format_params = {
       "filter_name": filter_name,
       "filter_name_u": filter_name.upper(),
       "avs_params": build_avs_params(params),
       "init_param_list": ', '.join([x.field_name for x in params]),
       "init_param_list_with_field_invoke": build_init_param_list_invoke(params, lambda x: x.has_field),
       "init_param_list_without_field_invoke": build_init_param_list_invoke(params, lambda x: not x.has_field),
       "init_param_list_with_field_func_def": build_init_param_list_func_def(params, lambda x: x.has_field),
       "class_field_def": build_class_field_def(params),
       "class_field_init": build_class_field_init(params),
       "class_field_copy": build_class_field_copy(params),
   }

   return OUTPUT_TEMPLATE.format(**format_params)


if __name__ == "__main__":
    generate_output()

