#include "dither_avs.h"

#include "../dither_high.h"

#include "../check.h"

#include "../constants.h"

#include "../sse_utils.h"

#include "../x64_compat.h"

AVSValue __cdecl Create_dither(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip child = args[0].AsClip();
    const VideoInfo& vi = child->GetVideoInfo();
    check_video_format("f3kdb_dither", vi, env);

    int mode = args[1].AsInt(1);
    bool stacked = args[2].AsBool(true);
    int input_depth = args[3].AsInt(16);
    bool keep_tv_range = args[4].AsBool(false);
    
    if (stacked && (vi.height & 3))
    {
        env->ThrowError("f3kdb_dither: Video height needs to be mod4");
    }
    if (!stacked && (vi.width & 3))
    {
        env->ThrowError("f3kdb_dither: Video width needs to be mod4");
    }

#define CHECK_PARAM(value, lower_bound, upper_bound) \
    check_parameter_range("f3kdb_dither", value, lower_bound, upper_bound, #value, env);

    CHECK_PARAM(mode, 0, 1);
    CHECK_PARAM(input_depth, 9, INTERNAL_BIT_DEPTH);

    return new dither_avs(child, mode, stacked, input_depth, keep_tv_range);
}

dither_avs::dither_avs(PClip child, int mode, bool stacked, int input_depth, bool keep_tv_range) : 
    GenericVideoFilter(child),
    _mode(mode),
    _stacked(stacked),
    _input_depth(input_depth),
    _keep_tv_range(keep_tv_range)
{
    if (_stacked)
    {
        vi.height >>= 1;
    } else {
        vi.width >>= 1;
    }
}

dither_avs::~dither_avs()
{
}

// inline memcpy for small unaligned src memory block
// dst is assumed to be aligned
// translated from Agner Fog's memcpy example
static void __forceinline _inline_memcpy(unsigned char * _Dst, const unsigned char * _Src, int _Size)
{
    assert(_Size < 32);
    assert((((POINTER_INT)_Dst) & 15) == 0); 
    assert((((POINTER_INT)_Src) & 15) != 0); 

    _Dst += _Size;
    _Src += _Size;
    _Size = -_Size;
    if (_Size <= -16)
    {
        _mm_store_si128((__m128i*)(_Dst + _Size), _mm_loadu_si128((__m128i*)(_Src + _Size)));
        _Size += 16;
    }
    if (_Size <= -8)
    {
        *(__int64*)(_Dst + _Size) = *(__int64*)(_Src + _Size);
        _Size += 8;
    }
    if (_Size <= -4)
    {
        *(int*)(_Dst + _Size) = *(int*)(_Src + _Size);
        _Size += 4;
        if (_Size == 0)
        {
            return;
        }
    }
    if (_Size <= -2)
    {
        *(short*)(_Dst + _Size) = *(short*)(_Src + _Size);
        _Size += 2;
    }
    if (_Size <= -1)
    {
        *(char*)(_Dst + _Size) = *(char*)(_Src + _Size);
    }
}

#define memcpy _inline_memcpy

static void __forceinline read_pixels(unsigned char const*&src_ptr_cur, int src_pitch, int target_height, bool aligned, bool stacked, bool need_special_read, int remaining_pixels, __m128i& pixels_0, __m128i& pixels_1)
{
    if (stacked)
    {
        // msb in 0, lsb in 1
        __m128i pixels_temp_0, pixels_temp_1;
        if (aligned)
        {
            pixels_temp_0 = _mm_load_si128((__m128i*)src_ptr_cur);
            pixels_temp_1 = _mm_load_si128((__m128i*)(src_ptr_cur + target_height * src_pitch));
        } else {
            pixels_temp_0 = _mm_loadu_si128((__m128i*)src_ptr_cur);
            if (!need_special_read)
            {
                pixels_temp_1 = _mm_loadu_si128((__m128i*)(src_ptr_cur + target_height * src_pitch));
            } else {
                __declspec(align(16))
                unsigned char buffer[16];
                memcpy(buffer, src_ptr_cur + target_height * src_pitch, remaining_pixels);
                pixels_temp_1 = _mm_load_si128((__m128i*)buffer);
            }
        }
        src_ptr_cur += 16;
        pixels_0 = _mm_unpacklo_epi8(pixels_temp_1, pixels_temp_0);
        pixels_1 = _mm_unpackhi_epi8(pixels_temp_1, pixels_temp_0);
    } else {
        if (aligned)
        {
            pixels_0 = _mm_load_si128((__m128i*)src_ptr_cur);
            pixels_1 = _mm_load_si128((__m128i*)(src_ptr_cur + 16));
        } else {
            if (!need_special_read)
            {
                pixels_0 = _mm_loadu_si128((__m128i*)src_ptr_cur);
                pixels_1 = _mm_loadu_si128((__m128i*)(src_ptr_cur + 16));
            } else {
                __declspec(align(16))
                unsigned char buffer[32];
                memcpy(buffer, src_ptr_cur, remaining_pixels * 2);
                pixels_0 = _mm_load_si128((__m128i*)buffer);
                pixels_1 = _mm_load_si128((__m128i*)(buffer + 16));
            }
        }
        src_ptr_cur += 32;
    }
}

#define PROCESS_PIXELS(prefix, dither_func, row, column, clamp_high_add, clamp_high_sub, clamp_low) \
{ \
    if (need_shifting) \
    { \
        prefix##_0 = _mm_sll_epi16(prefix##_0, pixel_shift_vector); \
        prefix##_1 = _mm_sll_epi16(prefix##_1, pixel_shift_vector); \
    } \
    prefix##_0 = dither_high::dither_func<mode+2>(context_buffer, prefix##_0, row, column); \
    prefix##_1 = dither_high::dither_func<mode+2>(context_buffer, prefix##_1, row, column + 8); \
    \
    prefix##_0 = _mm_srli_epi16(prefix##_0, INTERNAL_BIT_DEPTH - 8); \
    prefix##_1 = _mm_srli_epi16(prefix##_1, INTERNAL_BIT_DEPTH - 8); \
    \
    prefix##_out = _mm_packus_epi16(prefix##_0, prefix##_1); \
    if (need_clamping) \
    { \
        prefix##_out = low_bit_depth_pixels_clamp(prefix##_out, clamp_high_add, clamp_high_sub, clamp_low); \
    } \
}

template<int mode>
static void process_plane_impl(const unsigned char* src_ptr, const int src_pitch, unsigned char* dst_ptr, const int dst_pitch, const int target_width, const int target_height, const bool stacked, const int input_depth, const unsigned char pixel_min, const unsigned char pixel_max)
{
    char context_buffer[CONTEXT_BUFFER_SIZE];
    dither_high::init<mode+2>(context_buffer, target_width, 8);

    __m128i pixel_shift_vector = _mm_setzero_si128();
    bool need_shifting = input_depth < INTERNAL_BIT_DEPTH;
    if (need_shifting)
    {
        pixel_shift_vector = _mm_set_epi32(0, 0, 0, INTERNAL_BIT_DEPTH - input_depth);
    }

    __m128i clamp_high_add = _mm_setzero_si128();
    __m128i clamp_high_sub = _mm_setzero_si128();
    __m128i clamp_low = _mm_setzero_si128();

    bool need_clamping = pixel_min > 0 || pixel_max < 0xff;
    if (need_clamping)
    {
        clamp_high_add = _mm_set1_epi8((char)(0xff - pixel_max));
        clamp_high_sub = _mm_set1_epi8((char)(0xff - pixel_max + pixel_min));
        clamp_low = _mm_set1_epi8((char)(pixel_min));
    }

    bool aligned = (((POINTER_INT)src_ptr & 15) == 0) && ((src_pitch & 15) == 0);

    for (int row = 0; row < target_height; row++)
    {
        const unsigned char* src_ptr_cur = src_ptr + src_pitch * row;
        unsigned char* dst_ptr_cur = dst_ptr + dst_pitch * row;
        for (int column = 0; column < target_width; column += 16)
        {
            __m128i pixels_0, pixels_1;
            int remaining_pixels = target_width - column;
            bool need_special_read = ((src_pitch & 15) != 0) && (row == target_height - 1) && (remaining_pixels < 16);
            read_pixels(src_ptr_cur, src_pitch, target_height, aligned, stacked, need_special_read, remaining_pixels, pixels_0, pixels_1);
            __m128i pixels_out;
            
            PROCESS_PIXELS(pixels, dither, row, column, clamp_high_add, clamp_high_sub, clamp_low);

            _mm_store_si128((__m128i*)dst_ptr_cur, pixels_out);
            dst_ptr_cur += 16;
        }
        dither_high::next_row<mode+2>(context_buffer);
    }
    dither_high::complete<mode+2>(context_buffer);
}


template<int mode>
static void process_plane_impl_yuy2(const unsigned char* src_ptr, const int src_pitch, unsigned char* dst_ptr, const int dst_pitch, const int target_width, const int target_height, const bool stacked, const int input_depth, const bool need_clamping)
{
    char context_buffer[3][CONTEXT_BUFFER_SIZE];
    dither_high::init<mode+2>(context_buffer[0], target_width, 8);
    dither_high::init<mode+2>(context_buffer[1], target_width >> 1, 8);
    dither_high::init<mode+2>(context_buffer[2], target_width >> 1, 8);

    __m128i pixel_shift_vector = _mm_setzero_si128();
    bool need_shifting = input_depth < INTERNAL_BIT_DEPTH;
    if (need_shifting)
    {
        pixel_shift_vector = _mm_set_epi32(0, 0, 0, INTERNAL_BIT_DEPTH - input_depth);
    }
    
    __m128i clamp_high_add = _mm_setzero_si128();
    __m128i clamp_high_sub = _mm_setzero_si128();
    __m128i clamp_low = _mm_setzero_si128();

    if (need_clamping)
    {
        clamp_low = _mm_set1_epi16((short)( VALUE_8BIT(TV_RANGE_Y_MIN) | ( VALUE_8BIT(TV_RANGE_C_MIN) << 8 ) ) );
        __m128i clamp_high = _mm_set1_epi16((short)( VALUE_8BIT(TV_RANGE_Y_MAX) | ( VALUE_8BIT(TV_RANGE_C_MAX) << 8 ) ) );
        clamp_high_add = _mm_sub_epi8(_mm_set1_epi8((char)0xff), clamp_high);
        clamp_high_sub = _mm_add_epi8(clamp_high_add, clamp_low);
    }

    bool aligned = (((POINTER_INT)src_ptr & 15) == 0) && ((src_pitch & 15) == 0);
    
    for (int row = 0; row < target_height; row++)
    {
        const unsigned char* src_ptr_cur = src_ptr + src_pitch * row;
        unsigned char* dst_ptr_cur = dst_ptr + dst_pitch * row;
        for (int column = 0; column < (target_width * 2); column += 16)
        {
            __m128i pixels_0, pixels_1;
            int remaining_pixels = target_width - column;
            bool need_special_read = ((src_pitch & 15) != 0) && (row == target_height - 1) && (remaining_pixels < 16);
            read_pixels(src_ptr_cur, src_pitch, target_height, aligned, stacked, need_special_read, remaining_pixels, pixels_0, pixels_1);
            __m128i pixels_out;

            PROCESS_PIXELS(pixels, dither_yuy2, row, column, clamp_high_add, clamp_high_sub, clamp_low);

            _mm_store_si128((__m128i*)dst_ptr_cur, pixels_out);
            dst_ptr_cur += 16;
        }
        dither_high::next_row<mode+2>(context_buffer[0]);
        dither_high::next_row<mode+2>(context_buffer[1]);
        dither_high::next_row<mode+2>(context_buffer[2]);
    }

    dither_high::complete<mode+2>(context_buffer[0]);
    dither_high::complete<mode+2>(context_buffer[1]);
    dither_high::complete<mode+2>(context_buffer[2]);
}

void dither_avs::get_pixel_limits(int plane, unsigned char &pixel_min, unsigned char &pixel_max)
{
    if (_keep_tv_range)
    {
        pixel_min = plane == PLANAR_Y ? VALUE_8BIT(TV_RANGE_Y_MIN) : VALUE_8BIT(TV_RANGE_C_MIN);
        pixel_max = plane == PLANAR_Y ? VALUE_8BIT(TV_RANGE_Y_MAX) : VALUE_8BIT(TV_RANGE_C_MAX);
    } else {
        pixel_min = plane == PLANAR_Y ? VALUE_8BIT(FULL_RANGE_Y_MIN) : VALUE_8BIT(FULL_RANGE_C_MIN);
        pixel_max = plane == PLANAR_Y ? VALUE_8BIT(FULL_RANGE_Y_MAX) : VALUE_8BIT(FULL_RANGE_C_MAX);
    }
}
void dither_avs::process_plane(PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane, IScriptEnvironment* env)
{
    unsigned char pixel_min, pixel_max;
    get_pixel_limits(plane, pixel_min, pixel_max);
#define CALL_IMPL(mode) \
    process_plane_impl<mode>( \
        src->GetReadPtr(plane), \
        src->GetPitch(plane), \
        dstp, \
        dst->GetPitch(plane), \
        vi.width >> vi.GetPlaneWidthSubsampling(plane), \
        vi.height >> vi.GetPlaneHeightSubsampling(plane), \
        _stacked, \
        _input_depth, \
        pixel_min, \
        pixel_max \
    )


    if (_mode == 0)
    {
        CALL_IMPL(0);
    } else if (_mode == 1) {
        CALL_IMPL(1);
    } else {
        abort();
    }
#undef CALL_IMPL
}

void dither_avs::process_plane_yuy2(PVideoFrame src, PVideoFrame dst, unsigned char *dstp, IScriptEnvironment* env)
{
#define CALL_IMPL(mode) \
    process_plane_impl_yuy2<mode>( \
        src->GetReadPtr(), \
        src->GetPitch(), \
        dstp, \
        dst->GetPitch(), \
        vi.width, \
        vi.height, \
        _stacked, \
        _input_depth, \
        _keep_tv_range \
    )

    if (_mode == 0)
    {
        CALL_IMPL(0);
    } else if (_mode == 1) {
        CALL_IMPL(1);
    } else {
        abort();
    }
    #undef CALL_IMPL
}

PVideoFrame __stdcall dither_avs::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src = child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi, vi.IsYUY2() ? PLANE_ALIGNMENT * 4 : PLANE_ALIGNMENT);

    if (vi.IsY8())
    {
        process_plane(src, dst, dst->GetWritePtr(), PLANAR_Y, env);
    } else if (vi.IsPlanar()) {
        process_plane(src, dst, dst->GetWritePtr(PLANAR_Y), PLANAR_Y, env);
        process_plane(src, dst, dst->GetWritePtr(PLANAR_U), PLANAR_U, env);
        process_plane(src, dst, dst->GetWritePtr(PLANAR_V), PLANAR_V, env);
    } else {
        // YUY2
        process_plane_yuy2(src, dst, dst->GetWritePtr(), env);
    }

    return dst;
}