
#pragma once

/*************************
 * Script generated code *
 *     Do not modify     *
 *************************/

#include "avisynth.h"

static const char* FLASH3KYUU_DEBAND_AVS_PARAMS = "c[range]i[Y]i[Cb]i[Cr]i[ditherY]i[ditherC]i[sample_mode]i[seed]i[blur_first]b[diff_seed]b[opt]i[mt]b[precision_mode]i[keep_tv_range]b[input_mode]i[input_depth]i[enable_fast_skip_plane]b[random_algo_ref]i[random_algo_dither]i";

class flash3kyuu_deband_parameter_storage_t
{
protected:

    int _range; 
    unsigned short _Y; 
    unsigned short _Cb; 
    unsigned short _Cr; 
    int _ditherY; 
    int _ditherC; 
    int _sample_mode; 
    int _seed; 
    bool _blur_first; 
    bool _diff_seed_for_each_frame; 
    int _opt; 
    bool _mt; 
    int _precision_mode; 
    bool _keep_tv_range; 
    int _input_mode; 
    int _input_depth; 
    bool _enable_fast_skip_plane; 
    int _random_algo_ref; 
    int _random_algo_dither; 

public:

    flash3kyuu_deband_parameter_storage_t(const flash3kyuu_deband_parameter_storage_t& o)
    {
        _range = o._range; 
        _Y = o._Y; 
        _Cb = o._Cb; 
        _Cr = o._Cr; 
        _ditherY = o._ditherY; 
        _ditherC = o._ditherC; 
        _sample_mode = o._sample_mode; 
        _seed = o._seed; 
        _blur_first = o._blur_first; 
        _diff_seed_for_each_frame = o._diff_seed_for_each_frame; 
        _opt = o._opt; 
        _mt = o._mt; 
        _precision_mode = o._precision_mode; 
        _keep_tv_range = o._keep_tv_range; 
        _input_mode = o._input_mode; 
        _input_depth = o._input_depth; 
        _enable_fast_skip_plane = o._enable_fast_skip_plane; 
        _random_algo_ref = o._random_algo_ref; 
        _random_algo_dither = o._random_algo_dither; 
    }

    flash3kyuu_deband_parameter_storage_t( int range, unsigned short Y, unsigned short Cb, unsigned short Cr, int ditherY, int ditherC, int sample_mode, int seed, bool blur_first, bool diff_seed_for_each_frame, int opt, bool mt, int precision_mode, bool keep_tv_range, int input_mode, int input_depth, bool enable_fast_skip_plane, int random_algo_ref, int random_algo_dither )
    {
        _range = range; 
        _Y = Y; 
        _Cb = Cb; 
        _Cr = Cr; 
        _ditherY = ditherY; 
        _ditherC = ditherC; 
        _sample_mode = sample_mode; 
        _seed = seed; 
        _blur_first = blur_first; 
        _diff_seed_for_each_frame = diff_seed_for_each_frame; 
        _opt = opt; 
        _mt = mt; 
        _precision_mode = precision_mode; 
        _keep_tv_range = keep_tv_range; 
        _input_mode = input_mode; 
        _input_depth = input_depth; 
        _enable_fast_skip_plane = enable_fast_skip_plane; 
        _random_algo_ref = random_algo_ref; 
        _random_algo_dither = random_algo_dither; 
    }
};

typedef struct _FLASH3KYUU_DEBAND_RAW_ARGS
{
    AVSValue child, range, Y, Cb, Cr, ditherY, ditherC, sample_mode, seed, blur_first, diff_seed_for_each_frame, opt, mt, precision_mode, keep_tv_range, input_mode, input_depth, enable_fast_skip_plane, random_algo_ref, random_algo_dither;
} FLASH3KYUU_DEBAND_RAW_ARGS;

#define FLASH3KYUU_DEBAND_ARG_INDEX(name) (offsetof(FLASH3KYUU_DEBAND_RAW_ARGS, name) / sizeof(AVSValue))

#define FLASH3KYUU_DEBAND_ARG(name) args[FLASH3KYUU_DEBAND_ARG_INDEX(name)]

#define FLASH3KYUU_DEBAND_CREATE_CLASS(klass) new klass( child, flash3kyuu_deband_parameter_storage_t( range, (unsigned short)Y, (unsigned short)Cb, (unsigned short)Cr, ditherY, ditherC, sample_mode, seed, blur_first, diff_seed_for_each_frame, opt, mt, precision_mode, keep_tv_range, input_mode, input_depth, enable_fast_skip_plane, random_algo_ref, random_algo_dither ) )

#ifdef FLASH3KYUU_DEBAND_SIMPLE_MACRO_NAME

#ifdef SIMPLE_MACRO_NAME
#error Simple macro name has already been defined for SIMPLE_MACRO_NAME
#endif

#define SIMPLE_MACRO_NAME flash3kyuu_deband

#define ARG FLASH3KYUU_DEBAND_ARG

#define CREATE_CLASS FLASH3KYUU_DEBAND_CREATE_CLASS

#endif


