#include "flash3kyuu_deband.h"

#include "impl_dispatch.h"

#include "sse_compat.h"

#include "dither_high.h"

/****************************************************************************
 * NOTE: DON'T remove static from any function in this file, it is required *
 *       for generating code in multiple SSE versions.                      *
 ****************************************************************************/

typedef struct _info_cache
{
    int pitch;
    char* data_stream;
} info_cache;

static void destroy_cache(void* data)
{
    assert(data);

    info_cache* cache = (info_cache*) data;
    _aligned_free(cache->data_stream);
    free(data);
}

static __inline __m128i clamped_absolute_difference(__m128i a, __m128i b, __m128i difference_limit)
{
    // we need to clamp the result for 2 reasons:
    // 1. there is no integer >= operator in SSE
    // 2. comparison instructions accept only signed integers,
    //    so if difference is bigger than 0x7f, the compare result will be invalid
    __m128i diff = _mm_sub_epi8(_mm_max_epu8(a, b), _mm_min_epu8(a, b));
    return _mm_min_epu8(diff, difference_limit);
}

template <int sample_mode, int ref_part_index>
static __forceinline void process_plane_info_block(
    pixel_dither_info *&info_ptr, 
    __m128i &src_addrs, 
    const __m128i &src_pitch_vector, 
    const __m128i &src_addr_increment_vector, 
    __m128i &change_1, 
    __m128i &change_2, 
    const __m128i &minus_one, 
    const __m128i &width_subsample_vector,
    const __m128i &height_subsample_vector,
    unsigned char ref_pixels_1_components[16], 
    unsigned char ref_pixels_2_components[16], 
    unsigned char ref_pixels_3_components[16], 
    unsigned char ref_pixels_4_components[16],
    char*& info_data_stream)
{
    __m128i info_block = _mm_load_si128((__m128i*)info_ptr);

    if (sample_mode > 0) {
        // change: bit 16-31
        __m128i change_temp;
        change_temp = info_block;
        change_temp = _mm_srai_epi32(change_temp, 16);

        switch (ref_part_index)
        {
        case 0:
            change_1 = change_temp;
            break;
        case 1:
            change_1 = _mm_packs_epi32(change_1, change_temp);
            break;
        case 2:
            change_2 = change_temp;
            break;
        case 3:
            change_2 = _mm_packs_epi32(change_2, change_temp);
            break;
        }
    
    }

    // ref1: bit 0-7
    // left-shift & right-shift 24bits to remove other elements and preserve sign
    __m128i ref1 = info_block;
    ref1 = _mm_slli_epi32(ref1, 24); // << 24
    ref1 = _mm_srai_epi32(ref1, 24); // >> 24


    // temporarily store computed addresses
    __declspec(align(16))
    unsigned char* address_buffer_1[4];

    __declspec(align(16))
    unsigned char* address_buffer_2[4];

    __m128i ref_offset1;
    __m128i ref_offset2;

    __m128i temp_ref1;
    switch (sample_mode)
    {
    case 0:
        // ref1 = (abs(ref1) >> height_subsampling) * (sign(ref1))
        temp_ref1 = _mm_abs_epi32(ref1);
        temp_ref1 = _mm_sra_epi32(temp_ref1, height_subsample_vector);
        temp_ref1 = _cmm_mullo_limit16_epi32(temp_ref1, _mm_srai_epi32(ref1, 31));
        ref_offset1 = _cmm_mullo_limit16_epi32(src_pitch_vector, temp_ref1); // packed DWORD multiplication
        break;
    case 1:
        // ref1 is guarenteed to be postive
        temp_ref1 = _mm_sra_epi32(ref1, height_subsample_vector);
        ref_offset1 = _cmm_mullo_limit16_epi32(src_pitch_vector, temp_ref1); // packed DWORD multiplication

        ref_offset2 = _cmm_negate_all_epi32(ref_offset1, minus_one); // negates all offsets
        break;
    case 2:
        // ref2: bit 8-15
        // similar to above
        __m128i ref2;
        ref2 = info_block;
        ref2 = _mm_slli_epi32(ref2, 16); // << 16
        ref2 = _mm_srai_epi32(ref2, 24); // >> 24

        __m128i ref1_fix, ref2_fix;
        // ref_px = src_pitch * info.ref2 + info.ref1;
        ref1_fix = _mm_sra_epi32(ref1, width_subsample_vector);
        ref2_fix = _mm_sra_epi32(ref2, height_subsample_vector);
        ref_offset1 = _cmm_mullo_limit16_epi32(src_pitch_vector, ref2_fix); // packed DWORD multiplication
        ref_offset1 = _mm_add_epi32(ref_offset1, ref1_fix);

        // ref_px_2 = info.ref2 - src_pitch * info.ref1;
        ref1_fix = _mm_sra_epi32(ref1, height_subsample_vector);
        ref2_fix = _mm_sra_epi32(ref2, width_subsample_vector);
        ref_offset2 = _cmm_mullo_limit16_epi32(src_pitch_vector, ref1_fix); // packed DWORD multiplication
        ref_offset2 = _mm_sub_epi32(ref2_fix, ref_offset2);

        break;
    default:
        abort();
    }
    _mm_store_si128((__m128i*)address_buffer_1, _mm_add_epi32(src_addrs, ref_offset1));

    if (sample_mode > 0) {
        _mm_store_si128((__m128i*)address_buffer_2, _mm_add_epi32(src_addrs, ref_offset2));
    }

    if (info_data_stream){
        _mm_store_si128((__m128i*)info_data_stream, ref_offset1);
        info_data_stream += 16;

        if (sample_mode == 2) {
            _mm_store_si128((__m128i*)info_data_stream, ref_offset2);
            info_data_stream += 16;
        }
    }
    // load ref bytes
    ref_pixels_1_components[4 * ref_part_index + 0] = *(address_buffer_1[0]);
    ref_pixels_1_components[4 * ref_part_index + 1] = *(address_buffer_1[1]);
    ref_pixels_1_components[4 * ref_part_index + 2] = *(address_buffer_1[2]);
    ref_pixels_1_components[4 * ref_part_index + 3] = *(address_buffer_1[3]);

    if (sample_mode > 0) {
        ref_pixels_2_components[4 * ref_part_index + 0] = *(address_buffer_2[0]);
        ref_pixels_2_components[4 * ref_part_index + 1] = *(address_buffer_2[1]);
        ref_pixels_2_components[4 * ref_part_index + 2] = *(address_buffer_2[2]);
        ref_pixels_2_components[4 * ref_part_index + 3] = *(address_buffer_2[3]);
    }

    if (sample_mode == 2) {

        // another direction, negates all offsets
        ref_offset1 = _cmm_negate_all_epi32(ref_offset1, minus_one);
        _mm_store_si128((__m128i*)address_buffer_1, _mm_add_epi32(src_addrs, ref_offset1));

        ref_offset2 = _cmm_negate_all_epi32(ref_offset2, minus_one);
        _mm_store_si128((__m128i*)address_buffer_2, _mm_add_epi32(src_addrs, ref_offset2));

        ref_pixels_3_components[4 * ref_part_index + 0] = *(address_buffer_1[0]);
        ref_pixels_3_components[4 * ref_part_index + 1] = *(address_buffer_1[1]);
        ref_pixels_3_components[4 * ref_part_index + 2] = *(address_buffer_1[2]);
        ref_pixels_3_components[4 * ref_part_index + 3] = *(address_buffer_1[3]);

        ref_pixels_4_components[4 * ref_part_index + 0] = *(address_buffer_2[0]);
        ref_pixels_4_components[4 * ref_part_index + 1] = *(address_buffer_2[1]);
        ref_pixels_4_components[4 * ref_part_index + 2] = *(address_buffer_2[2]);
        ref_pixels_4_components[4 * ref_part_index + 3] = *(address_buffer_2[3]);

    }
    info_ptr += 4;
    src_addrs = _mm_add_epi32(src_addrs, src_addr_increment_vector);
}


static __m128i __inline process_pixels_mode0(__m128i src_pixels, __m128i threshold_vector, __m128i& ref_pixels)
{
    __m128i dst_pixels;
    __m128i blend_mask;

    __m128i difference;

    difference = clamped_absolute_difference(src_pixels, ref_pixels, threshold_vector);
    // mask: if difference >= threshold, set to 0xff, otherwise 0x00
    // difference is already clamped to threshold, so we compare for equality here
    blend_mask = _mm_cmpeq_epi8(difference, threshold_vector);

    // if mask is 0xff (over threshold), select second operand, otherwise select first
    dst_pixels = _cmm_blendv_by_cmp_mask_epi8(ref_pixels, src_pixels, blend_mask);

    return dst_pixels;
}

template<int sample_mode, bool blur_first>
static __m128i __inline process_pixels_mode12(__m128i src_pixels, __m128i threshold_vector, __m128i sign_convert_vector, __m128i& one_i8, __m128i& change, __m128i& ref_pixels_1, __m128i& ref_pixels_2, __m128i& ref_pixels_3, __m128i& ref_pixels_4)
{
    __m128i dst_pixels;
    __m128i use_orig_pixel_blend_mask;
    __m128i avg;

    __m128i difference;

    if (!blur_first)
    {
        difference = clamped_absolute_difference(src_pixels, ref_pixels_1, threshold_vector);
        // mask: if difference >= threshold, set to 0xff, otherwise 0x00
        // difference is already clamped to threshold, so we compare for equality here
        use_orig_pixel_blend_mask = _mm_cmpeq_epi8(difference, threshold_vector);

        difference = clamped_absolute_difference(src_pixels, ref_pixels_2, threshold_vector);
        // use OR to combine the masks
        use_orig_pixel_blend_mask = _mm_or_si128(_mm_cmpeq_epi8(difference, threshold_vector), use_orig_pixel_blend_mask);
    }

    avg = _mm_avg_epu8(ref_pixels_1, ref_pixels_2);

    if (sample_mode == 2)
    {

        if (!blur_first)
        {
            difference = clamped_absolute_difference(src_pixels, ref_pixels_3, threshold_vector);
            use_orig_pixel_blend_mask = _mm_or_si128(_mm_cmpeq_epi8(difference, threshold_vector), use_orig_pixel_blend_mask);

            difference = clamped_absolute_difference(src_pixels, ref_pixels_4, threshold_vector);
            use_orig_pixel_blend_mask = _mm_or_si128(_mm_cmpeq_epi8(difference, threshold_vector), use_orig_pixel_blend_mask);
        }
        // PAVGB adds 1 before calculating average, so we subtract 1 here to be consistent with c version

        __m128i avg2_tmp = _mm_avg_epu8(ref_pixels_3, ref_pixels_4);
        __m128i avg2 = _mm_min_epu8(avg, avg2_tmp);

        avg = _mm_max_epu8(avg, avg2_tmp);
        avg = _mm_subs_epu8(avg, one_i8);

        avg = _mm_avg_epu8(avg, avg2);

    }

    if (blur_first)
    {
        difference = clamped_absolute_difference(src_pixels, avg, threshold_vector);
        use_orig_pixel_blend_mask = _mm_cmpeq_epi8(difference, threshold_vector);
    }

    // if mask is 0xff (over threshold), select second operand, otherwise select first
    src_pixels = _cmm_blendv_by_cmp_mask_epi8(avg, src_pixels, use_orig_pixel_blend_mask);

    // convert to signed form, since change is signed
    src_pixels = _mm_sub_epi8(src_pixels, sign_convert_vector);

    // saturated add
    src_pixels = _mm_adds_epi8(src_pixels, change);

    // convert back to unsigned
    dst_pixels = _mm_add_epi8(src_pixels, sign_convert_vector);
    return dst_pixels;
}

static __forceinline __m128i generate_blend_mask_high(__m128i a, __m128i b, __m128i threshold)
{
    __m128i diff1 = _mm_subs_epu16(a, b);
    __m128i diff2 = _mm_subs_epu16(b, a);

    __m128i abs_diff = _mm_or_si128(diff1, diff2);

    __m128i sign_convert_vector = _mm_set1_epi16( (short)0x8000 );

    __m128i converted_diff = _mm_sub_epi16(abs_diff, sign_convert_vector);

    __m128i converted_threshold = _mm_sub_epi16(threshold, sign_convert_vector);

    // mask: if threshold >= diff, set to 0xff, otherwise 0x00
    // note that this is the opposite of low bitdepth implementation
    return _mm_cmpgt_epi16(converted_threshold, converted_diff);
}

static __m128i __forceinline upsample_pixels(__m128i pixels)
{
    return _mm_slli_epi16(pixels, 6);
}

template<int sample_mode, bool blur_first>
static __m128i __forceinline process_pixels_mode12_high_part(__m128i src_pixels, __m128i threshold_vector, __m128i change, __m128i& ref_pixels_1, __m128i& ref_pixels_2, __m128i& ref_pixels_3, __m128i& ref_pixels_4)
{	
    __m128i src_pixels_part = upsample_pixels(src_pixels);
    
    __m128i ref_part_1 = upsample_pixels(ref_pixels_1);
    __m128i ref_part_2 = upsample_pixels(ref_pixels_2);

    __m128i use_orig_pixel_blend_mask;
    __m128i avg;

    if (!blur_first)
    {
        use_orig_pixel_blend_mask = generate_blend_mask_high(src_pixels_part, ref_part_1, threshold_vector);

        // note: use AND instead of OR, because two operands are reversed
        // (different from low bit-depth mode!)
        use_orig_pixel_blend_mask = _mm_and_si128(
            use_orig_pixel_blend_mask, 
            generate_blend_mask_high(src_pixels_part, ref_part_2, threshold_vector) );
    }

    avg = _mm_avg_epu16(ref_part_1, ref_part_2);

    if (sample_mode == 2)
    {
        __m128i ref_part_3 = upsample_pixels(ref_pixels_3);
        __m128i ref_part_4 = upsample_pixels(ref_pixels_4);

        if (!blur_first)
        {
            use_orig_pixel_blend_mask = _mm_and_si128(
                use_orig_pixel_blend_mask, 
                generate_blend_mask_high(src_pixels_part, ref_part_3, threshold_vector) );

            use_orig_pixel_blend_mask = _mm_and_si128(
                use_orig_pixel_blend_mask, 
                generate_blend_mask_high(src_pixels_part, ref_part_4, threshold_vector) );
        }

        avg = _mm_avg_epu16(avg, _mm_avg_epu16(ref_part_3, ref_part_4));

    }

    if (blur_first)
    {
        use_orig_pixel_blend_mask = generate_blend_mask_high(src_pixels_part, avg, threshold_vector);
    }

    // if mask is 0xff (NOT over threshold), select second operand, otherwise select first
    // note this is different from low bitdepth code
    __m128i dst_pixels;

    dst_pixels = _cmm_blendv_by_cmp_mask_epi8(src_pixels_part, avg, use_orig_pixel_blend_mask);
    
    __m128i sign_convert_vector = _mm_set1_epi16((short)0x8000);

    // convert to signed form, since change is signed
    dst_pixels = _mm_sub_epi16(dst_pixels, sign_convert_vector);

    // saturated add
    dst_pixels = _mm_adds_epi16(dst_pixels, change);

    // convert back to unsigned
    dst_pixels = _mm_add_epi16(dst_pixels, sign_convert_vector);
    return dst_pixels;
}


template<int sample_mode, bool blur_first, int precision_mode>
static __m128i __forceinline process_pixels_mode12_high(__m128i src_pixels, __m128i threshold_vector, __m128i& change_1, __m128i& change_2, __m128i& ref_pixels_1, __m128i& ref_pixels_2, __m128i& ref_pixels_3, __m128i& ref_pixels_4, int row, int column, int height, int dst_pitch, __m128i* dst_px, void* dither_context)
{
    __m128i zero = _mm_setzero_si128();
    
    __m128i lo = process_pixels_mode12_high_part<sample_mode, blur_first>
        (_mm_unpacklo_epi8(src_pixels, zero), 
         threshold_vector, 
         change_1, 
         _mm_unpacklo_epi8(ref_pixels_1, zero), 
         _mm_unpacklo_epi8(ref_pixels_2, zero), 
         _mm_unpacklo_epi8(ref_pixels_3, zero), 
         _mm_unpacklo_epi8(ref_pixels_4, zero) );

    __m128i hi = process_pixels_mode12_high_part<sample_mode, blur_first>
        (_mm_unpackhi_epi8(src_pixels, zero), 
         threshold_vector, 
         change_2, 
         _mm_unpackhi_epi8(ref_pixels_1, zero), 
         _mm_unpackhi_epi8(ref_pixels_2, zero), 
         _mm_unpackhi_epi8(ref_pixels_3, zero), 
         _mm_unpackhi_epi8(ref_pixels_4, zero) );
    
    switch (precision_mode)
    {
    case PRECISION_LOW:
    case PRECISION_HIGH_NO_DITHERING:
    case PRECISION_HIGH_ORDERED_DITHERING:
    case PRECISION_HIGH_FLOYD_STEINBERG_DITHERING:
        lo = dither_high::dither<precision_mode>(dither_context, lo, row, column);
        hi = dither_high::dither<precision_mode>(dither_context, hi, row, column + 8);

        lo = _mm_srli_epi16(lo, 6);
        hi = _mm_srli_epi16(hi, 6);

        return _mm_packus_epi16(lo, hi);
        break;
    case PRECISION_16BIT_STACKED:
        {
            __m128i max_pixel_value = _mm_set1_epi16(0x3fff);
            lo = _mm_min_epu16(lo, max_pixel_value);
            hi = _mm_min_epu16(hi, max_pixel_value);
            __m128i msb_lo = _mm_srli_epi16(lo, 6);
            __m128i msb_hi = _mm_srli_epi16(hi, 6);

            __m128i msb = _mm_packus_epi16(msb_lo, msb_hi);
            _mm_store_si128(dst_px, msb);

            __m128i mask = _mm_set1_epi16(0x00ff);
            __m128i lsb_lo = _mm_slli_epi16(lo, 2);
            lsb_lo = _mm_and_si128(lsb_lo, mask);
            __m128i lsb_hi = _mm_slli_epi16(hi, 2);
            lsb_hi = _mm_and_si128(lsb_hi, mask);
        
            __m128i lsb = _mm_packus_epi16(lsb_lo, lsb_hi);
            _mm_store_si128((__m128i*)(((char*)dst_px) + dst_pitch * height), lsb);
        }
        break;
    case PRECISION_16BIT_INTERLEAVED:
        {
            __m128i max_pixel_value = _mm_set1_epi16(0x3fff);
            lo = _mm_min_epu16(lo, max_pixel_value);
            hi = _mm_min_epu16(hi, max_pixel_value);

            lo = _mm_slli_epi16(lo, 2);
            hi = _mm_slli_epi16(hi, 2);
        
            _mm_store_si128(dst_px, lo);
            _mm_store_si128(dst_px + 1, hi);
        }
        break;
    default:
        abort();
    }

    return zero;
}

template<int sample_mode, bool blur_first, int precision_mode>
static __m128i __forceinline process_pixels(__m128i src_pixels, __m128i threshold_vector, __m128i sign_convert_vector, __m128i& one_i8, __m128i& change_1, __m128i& change_2, __m128i& ref_pixels_1, __m128i& ref_pixels_2, __m128i& ref_pixels_3, __m128i& ref_pixels_4, int row, int column, int height, int dst_pitch, __m128i* dst_px, void* dither_context)
{
    switch (sample_mode)
    {
    case 0:
        return process_pixels_mode0(src_pixels, threshold_vector, ref_pixels_1);
        break;
    case 1:
    case 2:
        if (precision_mode == PRECISION_LOW)
        {
            return process_pixels_mode12<sample_mode, blur_first>(src_pixels, threshold_vector, sign_convert_vector, one_i8, change_1, ref_pixels_1, ref_pixels_2, ref_pixels_3, ref_pixels_4);
        } else {
            return process_pixels_mode12_high<sample_mode, blur_first, precision_mode>(src_pixels, threshold_vector, change_1, change_2, ref_pixels_1, ref_pixels_2, ref_pixels_3, ref_pixels_4, row, column, height, dst_pitch, dst_px, dither_context);
        }
        break;
    default:
        abort();
    }
    return src_pixels;
}

template<int sample_mode, bool blur_first, int precision_mode, bool aligned>
static void __cdecl _process_plane_sse_impl(const process_plane_params& params, process_plane_context* context)
{
    pixel_dither_info* info_ptr = params.info_ptr_base;

    // used to compute 4 consecutive addresses
    __m128i src_addr_offset_vector = _mm_set_epi32(3, 2, 1, 0);

    __m128i src_addr_increment_vector = _mm_set1_epi32(4);

    __m128i src_pitch_vector = _mm_set1_epi32(params.src_pitch);
           
    __m128i threshold_vector;
    
    if (precision_mode == PRECISION_LOW || sample_mode == 0)
    {
        threshold_vector = _mm_set1_epi8((unsigned char)params.threshold);
    } else {
        threshold_vector = _mm_set1_epi16(params.threshold);
    }

    __m128i sign_convert_vector = _mm_set1_epi8(0x80u);

    // general-purpose constant
    __m128i minus_one = _mm_set1_epi32(-1);

    __m128i one_i8 = _mm_set1_epi8(1);
    
    bool use_cached_info = false;

    char* info_data_stream = NULL;

    char context_buffer[DITHER_CONTEXT_BUFFER_SIZE];

    dither_high::init<precision_mode>(context_buffer, params.src_width);

    info_cache *cache = NULL;
    
    __m128i width_subsample_vector = _mm_set_epi32(0, 0, 0, params.width_subsampling);
    __m128i height_subsample_vector = _mm_set_epi32(0, 0, 0, params.height_subsampling);

    // initialize storage for pre-calculated pixel offsets
    if (context->data) {
        cache = (info_cache*) context->data;
        // we need to ensure src_pitch is the same, otherwise offsets will be completely wrong
        // also, if pitch changes, don't waste time to update the cache since it is likely to change again
        if (cache->pitch == params.src_pitch) {
            info_data_stream = cache->data_stream;
            use_cached_info = true;
        }
    } else {
        // set up buffer for cache
        cache = (info_cache*)malloc(sizeof(info_cache));
        // 4 offsets (2 bytes per item) + 2-byte change
        info_data_stream = (char*)_aligned_malloc(params.info_stride * (4 * 2 + 2) * params.src_height, FRAME_LUT_ALIGNMENT);
        cache->data_stream = info_data_stream;
        cache->pitch = params.src_pitch;
    }

    for (int row = 0; row < params.src_height; row++)
    {
        const unsigned char* src_px = params.src_plane_ptr + params.src_pitch * row;
        unsigned char* dst_px = params.dst_plane_ptr + params.dst_pitch * row;

        // info_ptr = info_ptr_base + info_stride * row;
        // doesn't need here, since info_stride equals to count of pixels that are needed to process in each row

        int processed_pixels = 0;

        // 4 addresses at a time
        __m128i src_addrs = _mm_set1_epi32((int)src_px);
        // add offset
        src_addrs = _mm_add_epi32(src_addrs, src_addr_offset_vector);

        while (processed_pixels < params.src_width)
        {
            __m128i change_1, change_2;

            __m128i ref_pixels_1;
            __m128i ref_pixels_2;
            __m128i ref_pixels_3;
            __m128i ref_pixels_4;

            if (LIKELY(use_cached_info)) {
                // cache layout: 16 offset groups (1 or 2 offsets / group depending on sample mode) in a pack, 
                //               followed by 32 bytes of change values
                // in the case of 2 offsets / group, offsets are stored like this:
                // [1 1 1 1 
                //  2 2 2 2
                //  1 1 1 1
                //  2 2 2 2
                //  .. snip
                //  1 1 1 1
                //  2 2 2 2]
                for (int i = 0; i < 16; i++)
                {

                    switch (sample_mode)
                    {
                    case 0:
                        ref_pixels_1.m128i_u8[i] = *(src_px + i + *(int*)(info_data_stream + 4 * i));
                        break;
                    case 1:
                        ref_pixels_1.m128i_u8[i] = *(src_px + i + *(int*)(info_data_stream + 4 * i));
                        ref_pixels_2.m128i_u8[i] = *(src_px + i + -*(int*)(info_data_stream + 4 * i));
                        break;
                    case 2:
                        ref_pixels_1.m128i_u8[i] = *(src_px + i + *(int*)(info_data_stream + 4 * (i + i / 4 * 4)));
                        ref_pixels_2.m128i_u8[i] = *(src_px + i + *(int*)(info_data_stream + 4 * (i + i / 4 * 4 + 4)));
                        ref_pixels_3.m128i_u8[i] = *(src_px + i + -*(int*)(info_data_stream + 4 * (i + i / 4 * 4)));
                        ref_pixels_4.m128i_u8[i] = *(src_px + i + -*(int*)(info_data_stream + 4 * (i + i / 4 * 4 + 4)));
                        break;
                    }
                }
                info_data_stream += (sample_mode == 2 ? 128 : 64);
                if (sample_mode > 0) {
                    change_1 = _mm_load_si128((__m128i*)info_data_stream);
                    info_data_stream += 16;
                    if (precision_mode != PRECISION_LOW)
                    {
                        change_2 = _mm_load_si128((__m128i*)info_data_stream);
                        info_data_stream += 16;
                    }
                }
            } else {
                // we need to process the info block
                change_1 = _mm_setzero_si128();
                change_2 = _mm_setzero_si128();

            
    #define PROCESS_INFO_BLOCK(n) \
                process_plane_info_block<sample_mode, n>(info_ptr, src_addrs, src_pitch_vector, src_addr_increment_vector, change_1, change_2, minus_one, width_subsample_vector, height_subsample_vector, ref_pixels_1.m128i_u8, ref_pixels_2.m128i_u8, ref_pixels_3.m128i_u8, ref_pixels_4.m128i_u8, info_data_stream);
            
                PROCESS_INFO_BLOCK(0);
                PROCESS_INFO_BLOCK(1);
                PROCESS_INFO_BLOCK(2);
                PROCESS_INFO_BLOCK(3);

    #undef PROCESS_INFO_BLOCK

                if (precision_mode == PRECISION_LOW)
                {
                    change_1 = _mm_packs_epi16(change_1, change_2);
                }

                if (sample_mode > 0) {

                    if (info_data_stream) {
                        _mm_store_si128((__m128i*)info_data_stream, change_1);
                        info_data_stream += 16;
                        if (precision_mode != PRECISION_LOW)
                        {
                            _mm_store_si128((__m128i*)info_data_stream, change_2);
                            info_data_stream += 16;
                        }
                    }
                }
            }

            __m128i src_pixels;
            if (aligned)
            {
                src_pixels = _mm_load_si128((__m128i*)src_px);
            } else {
                if (UNLIKELY(row == params.src_height - 1 && (params.src_pitch - processed_pixels) < 16))
                {
                    // prevent segfault
                    // doesn't need to initialize since garbage data won't cause error
                    __declspec(align(16)) unsigned char buffer[16];
                    memcpy(buffer, src_px, params.src_width - processed_pixels);
                    src_pixels = _mm_load_si128((__m128i*)buffer);
                } else {
                    src_pixels = _mm_loadu_si128((__m128i*)src_px);
                }
            }
            __m128i dst_pixels = process_pixels<sample_mode, blur_first, precision_mode>(src_pixels, threshold_vector, sign_convert_vector, one_i8, change_1, change_2, ref_pixels_1, ref_pixels_2, ref_pixels_3, ref_pixels_4, row, processed_pixels, params.src_height, params.dst_pitch, (__m128i*)dst_px, context_buffer);

            
            switch (precision_mode)
            {
            case PRECISION_LOW:
            case PRECISION_HIGH_NO_DITHERING:
            case PRECISION_HIGH_ORDERED_DITHERING:
            case PRECISION_HIGH_FLOYD_STEINBERG_DITHERING:
                _mm_store_si128((__m128i*)dst_px, dst_pixels);
                dst_px += 16;
                break;
            case PRECISION_16BIT_STACKED:
                // already written in process_pixels_mode12_high
                dst_px += 16;
                break;
            case PRECISION_16BIT_INTERLEAVED:
                // same as above
                dst_px += 32;
                break;
            default:
                abort();
            }

            src_px += 16;
            processed_pixels += 16;
        }
        dither_high::next_row<precision_mode>(context_buffer);
    }
    
    dither_high::complete<precision_mode>(context_buffer);

    // for thread-safety, save context after all data is processed
    if (!use_cached_info && !context->data && cache)
    {
        context->destroy = destroy_cache;
        if (InterlockedCompareExchangePointer(&context->data, cache, NULL) != NULL)
        {
            // other thread has completed first, so we can destroy our copy
            destroy_cache(cache);
        }
    }
}


template<int sample_mode, bool blur_first, int precision_mode>
static void __cdecl process_plane_sse_impl(const process_plane_params& params, process_plane_context* context)
{
    if ( ( (int)params.src_plane_ptr & (PLANE_ALIGNMENT - 1) ) == 0 && (params.src_pitch & (PLANE_ALIGNMENT - 1) ) == 0 )
    {
        _process_plane_sse_impl<sample_mode, blur_first, precision_mode, true>(params, context);
    } else {
        _process_plane_sse_impl<sample_mode, blur_first, precision_mode, false>(params, context);
    }
}