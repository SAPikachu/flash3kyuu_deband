// flash3kyuu_deband.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <intrin.h>
#include <process.h>
#include <windows.h>

#include "flash3kyuu_deband.h"
#include "impl_dispatch.h"
#include "icc_override.h"

#include "check.h"

AVSValue __cdecl Create_flash3kyuu_deband(AVSValue args, void* user_data, IScriptEnvironment* env){
    PClip child = args[0].AsClip();
    const VideoInfo& vi = child->GetVideoInfo();
    check_video_format("flash3kyuu_deband", vi, env);

    SYSTEM_INFO si;
    memset(&si, 0, sizeof(si));
    GetSystemInfo(&si);

    int range = args[1].AsInt(15);

    int sample_mode = args[7].AsInt(2);
    int seed = args[8].AsInt(0);
    bool blur_first = args[9].AsBool(true);
    bool diff_seed_for_each_frame = args[10].AsBool(false);
    int opt = args[11].AsInt(-1);
    bool mt = args[12].AsBool(si.dwNumberOfProcessors > 1);
    int precision_mode = args[13].AsInt(sample_mode == 0 ? PRECISION_LOW : PRECISION_HIGH_FLOYD_STEINBERG_DITHERING);
    bool keep_tv_range = args[14].AsBool(false);

    int default_val = (precision_mode == PRECISION_LOW || sample_mode == 0) ? 1 : 64;
    int Y = args[2].AsInt(default_val);
    int Cb = args[3].AsInt(default_val);
    int Cr = args[4].AsInt(default_val);
    int ditherY = args[5].AsInt(sample_mode == 0 ? 0 : default_val);
    int ditherC = args[6].AsInt(sample_mode == 0 ? 0 : default_val);

    if (sample_mode == 0)
    {
        if (precision_mode != PRECISION_LOW)
        {
            env->ThrowError("flash3kyuu_deband: sample_mode = 0 is valid only when precision_mode = 0.");
        }
        
        if (!blur_first)
        {
            env->ThrowError("flash3kyuu_deband: When sample_mode = 0, setting blur_first has no effect.");
        }

        if (ditherY != 0)
        {
            env->ThrowError("flash3kyuu_deband: When sample_mode = 0, setting ditherY has no effect.");
        }

        if (ditherC != 0)
        {
            env->ThrowError("flash3kyuu_deband: When sample_mode = 0, setting ditherC has no effect.");
        }
    }

    if (vi.IsYUY2())
    {
        // currently only C code is available
        opt = 0;
    }

#define CHECK_PARAM(value, lower_bound, upper_bound) \
    check_parameter_range("flash3kyuu_deband", value, lower_bound, upper_bound, #value, env);
    
    int threshold_upper_limit = default_val * 8 - 1;
    int dither_upper_limit = (precision_mode == PRECISION_LOW || sample_mode == 0) ? 3 : 4096;

    CHECK_PARAM(range, 0, 31);
    CHECK_PARAM(Y, 0, threshold_upper_limit);
    CHECK_PARAM(Cb, 0, threshold_upper_limit);
    CHECK_PARAM(Cr, 0, threshold_upper_limit);
    CHECK_PARAM(ditherY, 0, dither_upper_limit);
    CHECK_PARAM(ditherC, 0, dither_upper_limit);
    CHECK_PARAM(sample_mode, 0, 2);
    CHECK_PARAM(seed, 0, 127);
    CHECK_PARAM(opt, -1, (IMPL_COUNT - 1) );
    CHECK_PARAM(precision_mode, 0, (PRECISION_COUNT - 1) );

    // now the internal bit depth is 16, 
    // scale parameters to be consistent with 14bit range in previous versions
    Y <<= 2;
    Cb <<= 2;
    Cr <<= 2;
    ditherY <<= 2;
    ditherC <<= 2;
    
    return new flash3kyuu_deband(child, range, 
        (unsigned short)Y, (unsigned short)Cb, (unsigned short)Cr, 
        ditherY, ditherC, sample_mode, seed, 
        blur_first, diff_seed_for_each_frame, opt, mt, precision_mode, 
        keep_tv_range);
}

flash3kyuu_deband::flash3kyuu_deband(PClip child, int range, unsigned short Y, unsigned short Cb, unsigned short Cr, 
        int ditherY, int ditherC, int sample_mode, int seed,
        bool blur_first, bool diff_seed_for_each_frame, int opt, bool mt, int precision_mode,
        bool keep_tv_range) :
            GenericVideoFilter(child),
            _range(range),
            _Y(Y),
            _Cb(Cb),
            _Cr(Cr),
            _ditherY(ditherY),
            _ditherC(ditherC),
            _sample_mode(sample_mode),
            _seed(seed),
            _blur_first(blur_first),
            _diff_seed_for_each_frame(diff_seed_for_each_frame),
            _y_info(NULL),
            _cb_info(NULL),
            _cr_info(NULL),
            _opt(opt),
            _mt(mt),
            _precision_mode(precision_mode),
            _keep_tv_range(keep_tv_range),
            _mt_info(NULL),
            _process_plane_impl(NULL)
{
    this->init();
}

void flash3kyuu_deband::destroy_frame_luts(void)
{
    _aligned_free(_y_info);
    _aligned_free(_cb_info);
    _aligned_free(_cr_info);
    
    _y_info = NULL;
    _cb_info = NULL;
    _cr_info = NULL;
    
    // contexts are likely to be dependent on lut, so they must also be destroyed
    destroy_context(&_y_context);
    destroy_context(&_cb_context);
    destroy_context(&_cr_context);
}

static int inline rand_next(int &seed)
{
    int seed_tmp = (((seed << 13) ^ (unsigned int)seed) >> 17) ^ (seed << 13) ^ seed;
    seed = 32 * seed_tmp ^ seed_tmp;
    return seed & 0x7fffffff;
}

static int inline min_multi( int first, ... )
{
    int ret = first, i = first;
    va_list marker;

    va_start( marker, first );
    while( i >= 0 )
    {
        if (i < ret)
        {
            ret = i;
        }
        i = va_arg( marker, int);
    }
    va_end( marker );
    return ret;
}


void flash3kyuu_deband::init_frame_luts(int n)
{
    destroy_frame_luts();

    int seed = 0x92D68CA2 - _seed;
    if (_diff_seed_for_each_frame) 
    {
        seed -= n;
    }

    const VideoInfo& vi = GetVideoInfo();

    int y_stride;
    y_stride = FRAME_LUT_STRIDE(vi.RowSize(PLANAR_Y));

    int y_size = sizeof(pixel_dither_info) * y_stride * vi.height;
    _y_info = (pixel_dither_info*)_aligned_malloc(y_size, FRAME_LUT_ALIGNMENT);

    // ensure unused items are also initialized
    memset(_y_info, 0, y_size);

    int c_stride;
    bool has_chroma_plane = vi.IsPlanar() && !vi.IsY8();
    if (has_chroma_plane)
    {
        c_stride = FRAME_LUT_STRIDE(vi.RowSize(PLANAR_U));
        int c_size = sizeof(pixel_dither_info) * c_stride * (vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U));
        _cb_info = (pixel_dither_info*)_aligned_malloc(c_size, FRAME_LUT_ALIGNMENT);
        _cr_info = (pixel_dither_info*)_aligned_malloc(c_size, FRAME_LUT_ALIGNMENT);

        memset(_cb_info, 0, c_size);
        memset(_cr_info, 0, c_size);
    }

    pixel_dither_info *y_info_ptr, *cb_info_ptr, *cr_info_ptr;

    int ditherY_limit = _ditherY * 2 + 1;
    int ditherC_limit = _ditherC * 2 + 1;
    
    int width_subsamp = 0;
    int height_subsamp = 0;
    if (!vi.IsY8())
    {
        width_subsamp = vi.GetPlaneWidthSubsampling(PLANAR_U);
        height_subsamp = vi.GetPlaneHeightSubsampling(PLANAR_U);
    }

    for (int y = 0; y < vi.height; y++)
    {
        y_info_ptr = _y_info + y * y_stride;
        if (has_chroma_plane)
        {
            cb_info_ptr = _cb_info + (y >> height_subsamp) * c_stride;
            cr_info_ptr = _cr_info + (y >> height_subsamp) * c_stride;
        }
        for (int x = 0; x < vi.width; x++)
        {
            pixel_dither_info info_y = {0, 0, 0};
            info_y.change = (signed short)(rand_next(seed) % ditherY_limit - _ditherY);

            int cur_range = min_multi(_range, y, vi.height - y - 1, -1);
            if (_sample_mode == 2)
            {
                cur_range = min_multi(cur_range, x, vi.width - x - 1, -1);
            }

            if (cur_range > 0) {
                int range_limit = cur_range * 2 + 1;
                info_y.ref1 = (signed char)(rand_next(seed) % range_limit - cur_range);
                if (_sample_mode == 2)
                {
                    info_y.ref2 = (signed char)(rand_next(seed) % range_limit - cur_range);
                }
                if (_sample_mode > 0)
                {
                    info_y.ref1 = abs(info_y.ref1);
                    info_y.ref2 = abs(info_y.ref2);
                }
            }

            *y_info_ptr = info_y;

            bool should_set_c = false;
            if (has_chroma_plane)
            {
                should_set_c = ((x & ( ( 1 << width_subsamp ) - 1)) == 0 && 
                                (y & ( ( 1 << height_subsamp ) - 1)) == 0);
            } else if (vi.IsYUY2()) {
                should_set_c = ((x & 1) == 0);
            }

            if (should_set_c) {
                pixel_dither_info info_cb = info_y;
                pixel_dither_info info_cr = info_cb;
                
                // don't shift ref values here, since subsampling of width and height may be different
                // shift them in actual processing

                info_cb.change = (signed short)(rand_next(seed) % ditherC_limit - _ditherC);
                info_cr.change = (signed short)(rand_next(seed) % ditherC_limit - _ditherC);

                if (vi.IsPlanar())
                {
                    *cb_info_ptr = info_cb;
                    *cr_info_ptr = info_cr;
                    cb_info_ptr++;
                    cr_info_ptr++;
                } else if (vi.IsYUY2()) {
                    *(y_info_ptr + 1) = info_cb;
                    *(y_info_ptr + 3) = info_cr;
                }
            }
            if (vi.IsYUY2())
            {
                y_info_ptr += 2;
            } else {
                y_info_ptr++;
            }
        }
    }
}

flash3kyuu_deband::~flash3kyuu_deband()
{
    if (_mt_info) {
        _mt_info->exit = true;
        SetEvent(_mt_info->work_event);
        WaitForSingleObject(_mt_info->thread_handle, INFINITE);
        mt_info_destroy(_mt_info);
    }

    destroy_frame_luts();
}

static process_plane_impl_t get_process_plane_impl(int sample_mode, bool blur_first, int opt, int precision_mode)
{
    if (opt == -1) {
        int cpu_info[4] = {-1};
        __cpuid(cpu_info, 1);
        if (cpu_info[2] & 0x80000) {
            opt = IMPL_SSE4;
        } else if (cpu_info[2] & 0x200) {
            opt = IMPL_SSSE3;
        } else if (cpu_info[3] & 0x04000000) {
            opt = IMPL_SSE2;
        } else {
            opt = IMPL_C;
        }
    }
    const process_plane_impl_t* impl_table = process_plane_impls[precision_mode][opt];
    return impl_table[select_impl_index(sample_mode, blur_first)];
}

void flash3kyuu_deband::init(void) 
{
    ___intel_cpu_indicator_init();

    init_context(&_y_context);
    init_context(&_cb_context);
    init_context(&_cr_context);

    _src_vi = child->GetVideoInfo();

    init_frame_luts(0);

    if (_precision_mode == PRECISION_16BIT_STACKED)
    {
        vi.height *= 2;
    } else if (_precision_mode == PRECISION_16BIT_INTERLEAVED) {
        vi.width *= 2;
    }

    _process_plane_impl = get_process_plane_impl(_sample_mode, _blur_first, _opt, _precision_mode);

}

void flash3kyuu_deband::process_plane(PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane, IScriptEnvironment* env)
{
    process_plane_params params;

    memset(&params, 0, sizeof(process_plane_params));

    params.src_plane_ptr = src->GetReadPtr(plane);
    params.src_pitch = src->GetPitch(plane);
    params.src_width = _src_vi.RowSize(plane);
    params.src_height = _src_vi.height >> _src_vi.GetPlaneHeightSubsampling(plane);

    params.dst_plane_ptr = dstp;
    params.dst_pitch = dst->GetPitch(plane);

    params.plane = plane;
    
    params.width_subsampling = _src_vi.GetPlaneWidthSubsampling(plane);
    params.height_subsampling = _src_vi.GetPlaneHeightSubsampling(plane);

    params.vi = &_src_vi;
    params.dst_vi = &vi;

    params.info_stride = FRAME_LUT_STRIDE(params.src_width);

    process_plane_context* context;

    switch (plane & 7)
    {
    case 0:
    case PLANAR_Y:
        params.info_ptr_base = _y_info;
        params.threshold = _Y;
        params.pixel_max = _keep_tv_range ? TV_RANGE_Y_MAX : FULL_RANGE_Y_MAX;
        params.pixel_min = _keep_tv_range ? TV_RANGE_Y_MIN : FULL_RANGE_Y_MIN;
        context = &_y_context;
        break;
    case PLANAR_U:
        params.info_ptr_base = _cb_info;
        params.threshold = _Cb;
        params.pixel_max = _keep_tv_range ? TV_RANGE_C_MAX : FULL_RANGE_C_MAX;
        params.pixel_min = _keep_tv_range ? TV_RANGE_C_MIN : FULL_RANGE_C_MIN;
        context = &_cb_context;
        break;
    case PLANAR_V:
        params.info_ptr_base = _cr_info;
        params.threshold = _Cr;
        params.pixel_max = _keep_tv_range ? TV_RANGE_C_MAX : FULL_RANGE_C_MAX;
        params.pixel_min = _keep_tv_range ? TV_RANGE_C_MIN : FULL_RANGE_C_MIN;
        context = &_cr_context;
        break;
    default:
        abort();
    }
    

    bool copy_plane = false;
    if (vi.IsPlanar() && _precision_mode < PRECISION_16BIT_STACKED)
    {
        copy_plane = params.threshold == 0;
    }

    if (copy_plane) {
        // no need to process
        env->BitBlt(params.dst_plane_ptr, params.dst_pitch, params.src_plane_ptr, params.src_pitch, params.src_width, params.src_height);
        return;
    }

    params.pixel_max_c = _keep_tv_range ? TV_RANGE_C_MAX : FULL_RANGE_C_MAX;
    params.pixel_min_c = _keep_tv_range ? TV_RANGE_C_MIN : FULL_RANGE_C_MIN;
    params.threshold_y = _Y;
    params.threshold_cb = _Cb;
    params.threshold_cr = _Cr;
    params.range = (plane == PLANAR_Y ? _range : _range >> 1);
    _process_plane_impl(params, context);
}

void flash3kyuu_deband::mt_proc(void)
{
    volatile mt_info* info = _mt_info;
    assert(info);

    while (!info->exit)
    {
        process_plane(info->src, info->dst, info->dstp_u, PLANAR_U, info->env);
        process_plane(info->src, info->dst, info->dstp_v, PLANAR_V, info->env);

        mt_info_reset_pointers(info);

        SetEvent(info->work_complete_event);
        WaitForSingleObject(info->work_event, INFINITE);
    }
}

unsigned int __stdcall mt_proc_wrapper(void* filter_instance) 
{
    assert(filter_instance);
    ((flash3kyuu_deband*)filter_instance)->mt_proc();
    return 0;
}

PVideoFrame __stdcall flash3kyuu_deband::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src = child->GetFrame(n, env);
    // interleaved 16bit output needs extra alignment
    PVideoFrame dst = env->NewVideoFrame(vi, _precision_mode != PRECISION_16BIT_INTERLEAVED ? PLANE_ALIGNMENT : PLANE_ALIGNMENT * 2);

    if (_diff_seed_for_each_frame)
    {
        init_frame_luts(n);
    }

    if (vi.IsPlanar() && !vi.IsY8())
    {
        if (!_mt) 
        {
            process_plane(src, dst, dst->GetWritePtr(PLANAR_Y), PLANAR_Y, env);
            process_plane(src, dst, dst->GetWritePtr(PLANAR_U), PLANAR_U, env);
            process_plane(src, dst, dst->GetWritePtr(PLANAR_V), PLANAR_V, env);
        } else {
            bool new_thread = _mt_info == NULL;
            if (UNLIKELY(new_thread)) 
            {
                _mt_info = mt_info_create();
                if (!_mt_info) {
                    env->ThrowError("flash3kyuu_deband: Failed to allocate mt_info.");
                    return NULL;
                }
            }
            // we must get write pointer before copying the frame pointer
            // otherwise NULL will be returned
            unsigned char* dstp_y = dst->GetWritePtr(PLANAR_Y);
            _mt_info->dstp_u = dst->GetWritePtr(PLANAR_U);
            _mt_info->dstp_v = dst->GetWritePtr(PLANAR_V);
            _mt_info->env = env;
            _mt_info->dst = dst;
            _mt_info->src = src;
            if (LIKELY(!new_thread))
            {
                SetEvent(_mt_info->work_event);
            }
            else
            {
                _mt_info->thread_handle = (HANDLE)_beginthreadex(NULL, 0, mt_proc_wrapper, this, 0, NULL);
                if (!_mt_info->thread_handle) {
                    int err = errno;
                    mt_info_destroy(_mt_info);
                    _mt_info = NULL;
                    env->ThrowError("flash3kyuu_deband: Failed to create worker thread, code = %d.", err);
                    return NULL;
                }
            }
            process_plane(src, dst, dstp_y, PLANAR_Y, env);
            WaitForSingleObject(_mt_info->work_complete_event, INFINITE);
        }
    } else {
        // YUY2 or Y8
        process_plane(src, dst, dst->GetWritePtr(), PLANAR_Y, env);
    }
    return dst;
}