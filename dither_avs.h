#pragma once

#include "stdafx.h"

#include "avisynth.h"

AVSValue __cdecl Create_dither(AVSValue args, void* user_data, IScriptEnvironment* env);


class dither_avs : public GenericVideoFilter {
private:
    int _mode;
    bool _stacked;
    int _input_depth;
    bool _keep_tv_range;
    
    void process_plane(PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane, IScriptEnvironment* env);
    void process_plane_yuy2(PVideoFrame src, PVideoFrame dst, unsigned char *dstp, IScriptEnvironment* env);
    void get_pixel_limits(int plane, unsigned char &pixel_min, unsigned char &pixel_max);


public:
    dither_avs(PClip child, int mode, bool stacked, int input_depth, bool keep_tv_range);
    ~dither_avs();

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};