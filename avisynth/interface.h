#pragma once

#include "avisynth.h"
#include "flash3kyuu_deband.def.h"
#include "../flash3kyuu_deband.h"

#include "mt_info.h"

AVSValue __cdecl Create_flash3kyuu_deband(AVSValue args, void* user_data, IScriptEnvironment* env);
FLASH3KYUU_DEBAND_API const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env);

class flash3kyuu_deband : public GenericVideoFilter, public flash3kyuu_deband_parameter_storage_t {
private:
    process_plane_impl_t _process_plane_impl;
        
    pixel_dither_info *_y_info;
    pixel_dither_info *_cb_info;
    pixel_dither_info *_cr_info;
    
    process_plane_context _y_context;
    process_plane_context _cb_context;
    process_plane_context _cr_context;
    
    short* _grain_buffer_y;
    short* _grain_buffer_c;

    int* _grain_buffer_offsets;

    volatile mt_info* _mt_info;

    VideoInfo _src_vi;

    void init(void);
    void init_frame_luts(void);

    void destroy_frame_luts(void);
    
    void process_plane(int n, PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane, IScriptEnvironment* env);


public:
    void mt_proc(void);
    flash3kyuu_deband(PClip child, flash3kyuu_deband_parameter_storage_t& o);
    ~flash3kyuu_deband();

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};