
/*************************
 * Script generated code *
 *     Do not modify     *
 *************************/

#include "random.h"

void params_set_defaults(f3kdb_params_t* params)
{
    params->range = 15;
    params->Y = 64;
    params->Cb = 64;
    params->Cr = 64;
    params->grainY = 64;
    params->grainC = 64;
    params->sample_mode = 2;
    params->seed = 0;
    params->blur_first = true;
    params->dynamic_grain = false;
    params->opt = -1;
    params->dither_algo = DA_HIGH_FLOYD_STEINBERG_DITHERING;
    params->keep_tv_range = false;
    params->input_mode = DEFAULT_PIXEL_MODE;
    params->input_depth = -1;
    params->output_mode = DEFAULT_PIXEL_MODE;
    params->output_depth = -1;
    params->random_algo_ref = RANDOM_ALGORITHM_UNIFORM;
    params->random_algo_grain = RANDOM_ALGORITHM_UNIFORM;
    params->random_param_ref = DEFAULT_RANDOM_PARAM;
    params->random_param_grain = DEFAULT_RANDOM_PARAM;
}
