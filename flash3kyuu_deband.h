// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FLASH3KYUU_DEBAND_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FLASH3KYUU_DEBAND_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#pragma once

#include "avisynth.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <malloc.h>

#include "flash3kyuu_deband.def.h"

#include "process_plane_context.h"

#include "mt_info.h"

#include "constants.h"

#ifdef FLASH3KYUU_DEBAND_EXPORTS
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllexport)
#else
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllimport)
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define LIKELY(x)       __builtin_expect((x),1)
#define UNLIKELY(x)     __builtin_expect((x),0)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif

AVSValue __cdecl Create_flash3kyuu_deband(AVSValue args, void* user_data, IScriptEnvironment* env);

FLASH3KYUU_DEBAND_API const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env);


typedef __declspec(align(4)) struct _pixel_dither_info {
    signed char ref1, ref2;
    signed short change;
} pixel_dither_info;

typedef enum _INPUT_MODE
{
    LOW_BIT_DEPTH = 0,
    HIGH_BIT_DEPTH_STACKED,
    HIGH_BIT_DEPTH_INTERLEAVED
} INPUT_MODE;

typedef struct _process_plane_params
{
    const unsigned char *src_plane_ptr;
    int src_width;
    int src_height;
    int src_pitch;

    unsigned char *dst_plane_ptr;
    int dst_pitch;

    unsigned short threshold;
    pixel_dither_info *info_ptr_base;
    int info_stride;

    int plane;

    unsigned char width_subsampling;
    unsigned char height_subsampling;

    INPUT_MODE input_mode;
    int input_depth;
    
    int pixel_max;
    int pixel_min;

    // for use with YUY2
    int pixel_max_c;
    int pixel_min_c;
    
    unsigned short threshold_y, threshold_cb, threshold_cr;
    VideoInfo* vi;
    VideoInfo* dst_vi;
} process_plane_params;

typedef void (__cdecl *process_plane_impl_t)(const process_plane_params& params, process_plane_context* context);

class flash3kyuu_deband : public GenericVideoFilter, public flash3kyuu_deband_parameter_storage_t {
private:
    process_plane_impl_t _process_plane_impl;
        
    pixel_dither_info *_y_info;
    pixel_dither_info *_cb_info;
    pixel_dither_info *_cr_info;
    
    process_plane_context _y_context;
    process_plane_context _cb_context;
    process_plane_context _cr_context;

    volatile mt_info* _mt_info;

    VideoInfo _src_vi;

    void init(void);
    void init_frame_luts(int n);

    void destroy_frame_luts(void);
    
    void process_plane(PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane, IScriptEnvironment* env);


public:
    void mt_proc(void);
    flash3kyuu_deband(PClip child, flash3kyuu_deband_parameter_storage_t& o);
    ~flash3kyuu_deband();

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};