#pragma once

#include "avisynth/avisynth.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <malloc.h>

#include "process_plane_context.h"

#include "constants.h"

#ifdef FLASH3KYUU_DEBAND_EXPORTS
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllexport)
#else
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllimport)
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define LIKELY(x)       __builtin_expect((x),1)
#define UNLIKELY(x)     __builtin_expect((x),0)
#define EXPECT(x, val)  __builtin_expect((x),val)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#define EXPECT(x, val)  (x)
#endif

#ifdef __INTEL_COMPILER
#define __PRAGMA_NOUNROLL__ __pragma(nounroll)
#else
#define __PRAGMA_NOUNROLL__
#endif


typedef __declspec(align(4)) struct _pixel_dither_info {
    signed char ref1, ref2;
    signed short change;
} pixel_dither_info;

typedef struct _process_plane_params
{
    const unsigned char *src_plane_ptr;

    int src_width;
    int src_height;
    int src_pitch;

    unsigned char *dst_plane_ptr;
    int dst_pitch;

    int plane_width_in_pixels;
    int plane_height_in_pixels;

    PIXEL_MODE input_mode;
    int input_depth;
    PIXEL_MODE output_mode;
    int output_depth;

    unsigned short threshold;
    pixel_dither_info *info_ptr_base;
    int info_stride;
    
    short* grain_buffer;
    int grain_buffer_stride;

    int plane;

    unsigned char width_subsampling;
    unsigned char height_subsampling;
    
    int pixel_max;
    int pixel_min;
    
    // Helper functions
    __inline int get_dst_width() const {
        int width = src_width;
        if (input_mode != output_mode) {
            if (output_mode == HIGH_BIT_DEPTH_INTERLEAVED) {
                width *= 2;
            } else if (input_mode == HIGH_BIT_DEPTH_INTERLEAVED) {
                width /= 2;
            }
        }
        return width;
    }
    __inline int get_dst_height() const {
        int height = src_height;
        if (input_mode != output_mode) {
            if (output_mode == HIGH_BIT_DEPTH_STACKED) {
                height *= 2;
            } else if (input_mode == HIGH_BIT_DEPTH_STACKED) {
                height /= 2;
            }
        }
        return height;
    }
} process_plane_params;

typedef void (__cdecl *process_plane_impl_t)(const process_plane_params& params, process_plane_context* context);

