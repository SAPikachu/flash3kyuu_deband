
#pragma once

/*************************
 * Script generated code *
 *     Do not modify     *
 *************************/

#include "avisynth.h"

#include "constants.h"

static const char* FLASH3KYUU_DEBAND_AVS_PARAMS = "c[range]i[Y]i[Cb]i[Cr]i[grainY]i[grainC]i[sample_mode]i[seed]i[blur_first]b[dynamic_grain]b[opt]i[mt]b[dither_algo]i[keep_tv_range]b[input_mode]i[input_depth]i[output_mode]i[output_depth]i[enable_fast_skip_plane]b[random_algo_ref]i[random_algo_dither]i";

class flash3kyuu_deband_parameter_storage_t
{
protected:

    int _range; 
    unsigned short _Y; 
    unsigned short _Cb; 
    unsigned short _Cr; 
    int _grainY; 
    int _grainC; 
    int _sample_mode; 
    int _seed; 
    bool _blur_first; 
    bool _dynamic_grain; 
    int _opt; 
    bool _mt; 
    DITHER_ALGORITHM _dither_algo; 
    bool _keep_tv_range; 
    PIXEL_MODE _input_mode; 
    int _input_depth; 
    PIXEL_MODE _output_mode; 
    int _output_depth; 
    bool _enable_fast_skip_plane; 
    RANDOM_ALGORITHM _random_algo_ref; 
    RANDOM_ALGORITHM _random_algo_dither; 

public:

    flash3kyuu_deband_parameter_storage_t(const flash3kyuu_deband_parameter_storage_t& o)
    {
        _range = o._range; 
        _Y = o._Y; 
        _Cb = o._Cb; 
        _Cr = o._Cr; 
        _grainY = o._grainY; 
        _grainC = o._grainC; 
        _sample_mode = o._sample_mode; 
        _seed = o._seed; 
        _blur_first = o._blur_first; 
        _dynamic_grain = o._dynamic_grain; 
        _opt = o._opt; 
        _mt = o._mt; 
        _dither_algo = o._dither_algo; 
        _keep_tv_range = o._keep_tv_range; 
        _input_mode = o._input_mode; 
        _input_depth = o._input_depth; 
        _output_mode = o._output_mode; 
        _output_depth = o._output_depth; 
        _enable_fast_skip_plane = o._enable_fast_skip_plane; 
        _random_algo_ref = o._random_algo_ref; 
        _random_algo_dither = o._random_algo_dither; 
    }

    flash3kyuu_deband_parameter_storage_t( int range, unsigned short Y, unsigned short Cb, unsigned short Cr, int grainY, int grainC, int sample_mode, int seed, bool blur_first, bool dynamic_grain, int opt, bool mt, DITHER_ALGORITHM dither_algo, bool keep_tv_range, PIXEL_MODE input_mode, int input_depth, PIXEL_MODE output_mode, int output_depth, bool enable_fast_skip_plane, RANDOM_ALGORITHM random_algo_ref, RANDOM_ALGORITHM random_algo_dither )
    {
        _range = range; 
        _Y = Y; 
        _Cb = Cb; 
        _Cr = Cr; 
        _grainY = grainY; 
        _grainC = grainC; 
        _sample_mode = sample_mode; 
        _seed = seed; 
        _blur_first = blur_first; 
        _dynamic_grain = dynamic_grain; 
        _opt = opt; 
        _mt = mt; 
        _dither_algo = dither_algo; 
        _keep_tv_range = keep_tv_range; 
        _input_mode = input_mode; 
        _input_depth = input_depth; 
        _output_mode = output_mode; 
        _output_depth = output_depth; 
        _enable_fast_skip_plane = enable_fast_skip_plane; 
        _random_algo_ref = random_algo_ref; 
        _random_algo_dither = random_algo_dither; 
    }
};

typedef struct _FLASH3KYUU_DEBAND_RAW_ARGS
{
    AVSValue child, range, Y, Cb, Cr, grainY, grainC, sample_mode, seed, blur_first, dynamic_grain, opt, mt, dither_algo, keep_tv_range, input_mode, input_depth, output_mode, output_depth, enable_fast_skip_plane, random_algo_ref, random_algo_dither;
} FLASH3KYUU_DEBAND_RAW_ARGS;

#define FLASH3KYUU_DEBAND_ARG_INDEX(name) (offsetof(FLASH3KYUU_DEBAND_RAW_ARGS, name) / sizeof(AVSValue))

#define FLASH3KYUU_DEBAND_ARG(name) args[FLASH3KYUU_DEBAND_ARG_INDEX(name)]

#define FLASH3KYUU_DEBAND_CREATE_CLASS(klass) new klass( child, flash3kyuu_deband_parameter_storage_t( range, (unsigned short)Y, (unsigned short)Cb, (unsigned short)Cr, grainY, grainC, sample_mode, seed, blur_first, dynamic_grain, opt, mt, (DITHER_ALGORITHM)dither_algo, keep_tv_range, (PIXEL_MODE)input_mode, input_depth, (PIXEL_MODE)output_mode, output_depth, enable_fast_skip_plane, (RANDOM_ALGORITHM)random_algo_ref, (RANDOM_ALGORITHM)random_algo_dither ) )

#ifdef FLASH3KYUU_DEBAND_SIMPLE_MACRO_NAME

#ifdef SIMPLE_MACRO_NAME
#error Simple macro name has already been defined for SIMPLE_MACRO_NAME
#endif

#define SIMPLE_MACRO_NAME flash3kyuu_deband

#define ARG FLASH3KYUU_DEBAND_ARG

#define CREATE_CLASS FLASH3KYUU_DEBAND_CREATE_CLASS

#endif


