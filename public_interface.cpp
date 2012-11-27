#include <memory.h>
#include <assert.h>
#include <exception>
#include <stdio.h>
#include <stdarg.h>

#include "core.h"
#include "auto_utils.h"
#include "constants.h"
#include "impl_dispatch.h"

int f3kdb_params_init_defaults(f3kdb_params_t* params, int interface_version)
{
    if (interface_version != F3KDB_INTERFACE_VERSION)
    {
        return F3KDB_ERROR_INVALID_INTERFACE_VERSION;
    }
    memset(params, 0, sizeof(f3kdb_params_t));
    params_set_defaults(params);
    return F3KDB_SUCCESS;
}

int f3kdb_params_sanitize(f3kdb_params_t* params, int interface_version)
{
    if (interface_version != F3KDB_INTERFACE_VERSION)
    {
        return F3KDB_ERROR_INVALID_INTERFACE_VERSION;
    }

    if (params->input_mode == DEFAULT_PIXEL_MODE)
    {
        params->input_mode = params->input_depth <= 8 ? LOW_BIT_DEPTH : HIGH_BIT_DEPTH_STACKED;
    }
    if (params->input_depth == -1)
    {
        params->input_depth = params->input_mode == LOW_BIT_DEPTH ? 8 : 16;
    }

    if (params->output_mode == DEFAULT_PIXEL_MODE)
    {
        params->output_mode = params->output_depth <= 8 ? LOW_BIT_DEPTH : HIGH_BIT_DEPTH_STACKED;
    }
    if (params->output_depth == -1)
    {
        params->output_depth = params->output_mode == LOW_BIT_DEPTH ? 8 : 16;
    }

    return F3KDB_SUCCESS;
}

static void print_error(char* buffer, size_t buffer_size, const char* format, ...)
{
    if (!buffer || buffer_size <= 0)
    {
        return;
    }
    va_list va;
    va_start(va, format);
    vsnprintf(buffer, buffer_size, format, va);
}

int f3kdb_create(const f3kdb_video_info_t* video_info, const f3kdb_params_t* params_in, f3kdb_core_t** core_out, char* error_msg, size_t error_msg_size, int interface_version)
{
    if (interface_version != F3KDB_INTERFACE_VERSION)
    {
        return F3KDB_ERROR_INVALID_INTERFACE_VERSION;
    }
    *core_out = nullptr;
    if (error_msg && error_msg_size > 0)
    {
        error_msg[0] = 0;
    }

#define INVALID_PARAM_IF(cond) \
    do { if (cond) { print_error(error_msg, error_msg_size, "Invalid parameter condition: %s", #cond); return F3KDB_ERROR_INVALID_PARAMETER; } } while (0)

    INVALID_PARAM_IF(!video_info);
    INVALID_PARAM_IF(!params_in);
    INVALID_PARAM_IF(video_info->width < 16);
    INVALID_PARAM_IF(video_info->height < 16);
    INVALID_PARAM_IF(video_info->chroma_width_subsampling < 0 || video_info->chroma_width_subsampling > 4);
    INVALID_PARAM_IF(video_info->chroma_height_subsampling < 0 || video_info->chroma_height_subsampling > 4);
    INVALID_PARAM_IF(video_info->num_frames <= 0);

    f3kdb_params_t params;
    memcpy(&params, params_in, sizeof(f3kdb_params_t));

    f3kdb_params_sanitize(&params);

    if (params.output_depth == 8 && params.output_mode != LOW_BIT_DEPTH)
    {
        print_error(error_msg, error_msg_size, "%s", "output_mode > 0 is only valid when output_depth > 8");
        return F3KDB_ERROR_INVALID_PARAMETER;
    }
    if (params.output_depth > 8 && params.output_mode == LOW_BIT_DEPTH)
    {
        print_error(error_msg, error_msg_size, "%s", "output_mode = 0 is only valid when output_depth = 8");
        return F3KDB_ERROR_INVALID_PARAMETER;
    }
    if (params.output_depth == 16)
    {
        // set to appropriate precision mode
        switch (params.output_mode)
        {
        case HIGH_BIT_DEPTH_INTERLEAVED:
            params.dither_algo = DA_16BIT_INTERLEAVED;
            break;
        case HIGH_BIT_DEPTH_STACKED:
            params.dither_algo = DA_16BIT_STACKED;
            break;
        default:
            // something is wrong here!
            assert(false);
            return F3KDB_ERROR_INVALID_STATE;
        }
    }

    int threshold_upper_limit = 64 * 8 - 1;
    int dither_upper_limit = 4096;

#define CHECK_PARAM(value, lower_bound, upper_bound) \
    do { if (params.value < lower_bound || params.value > upper_bound) { print_error(error_msg, error_msg_size, "Invalid parameter %s, must be between %d and %d", #value, lower_bound, upper_bound); return F3KDB_ERROR_INVALID_PARAMETER; } } while(0)

    CHECK_PARAM(range, 0, 31);
    CHECK_PARAM(Y, 0, threshold_upper_limit);
    CHECK_PARAM(Cb, 0, threshold_upper_limit);
    CHECK_PARAM(Cr, 0, threshold_upper_limit);
    CHECK_PARAM(grainY, 0, dither_upper_limit);
    CHECK_PARAM(grainC, 0, dither_upper_limit);
    CHECK_PARAM(sample_mode, 1, 2);
    CHECK_PARAM(opt, -1, (IMPL_COUNT - 1) );
    CHECK_PARAM(dither_algo, DA_HIGH_NO_DITHERING, (DA_COUNT - 1) );
    CHECK_PARAM(random_algo_ref, 0, (RANDOM_ALGORITHM_COUNT - 1) );
    CHECK_PARAM(random_algo_grain, 0, (RANDOM_ALGORITHM_COUNT - 1) );
    CHECK_PARAM(input_mode, 0, PIXEL_MODE_COUNT - 1);
    CHECK_PARAM(output_mode, 0, PIXEL_MODE_COUNT - 1);
    
    if (params.input_mode != LOW_BIT_DEPTH)
    {
        CHECK_PARAM(input_depth, 9, INTERNAL_BIT_DEPTH);
    }

    if (params.output_mode != LOW_BIT_DEPTH)
    {
        CHECK_PARAM(output_depth, 9, INTERNAL_BIT_DEPTH);
    }

    // now the internal bit depth is 16, 
    // scale parameters to be consistent with 14bit range in previous versions
    params.Y <<= 2;
    params.Cb <<= 2;
    params.Cr <<= 2;
    params.grainY <<= 2;
    params.grainC <<= 2;

    try
    {
        *core_out = new f3kdb_core_t(video_info, &params);
    } catch (std::bad_alloc&) {
        return F3KDB_INSUFFICIENT_MEMORY;
    }
    return F3KDB_SUCCESS;
}

int f3kdb_destroy(f3kdb_core_t* context)
{
    delete context;
    return F3KDB_SUCCESS;
}

int f3kdb_process_plane(f3kdb_core_t* core, int frame_index, int plane, unsigned char* dst_frame_ptr, int dst_pitch, const unsigned char* src_frame_ptr, int src_pitch)
{
    if (!core)
    {
        return F3KDB_ERROR_INVALID_PARAMETER;
    }
    return core->process_plane(frame_index, plane, dst_frame_ptr, dst_pitch, src_frame_ptr, src_pitch);
}
