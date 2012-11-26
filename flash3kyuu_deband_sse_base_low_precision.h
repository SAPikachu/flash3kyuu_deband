#pragma once

#include <stdlib.h>
#include "include/f3kdb.h"
#include "constants.h"

namespace sse_low_dither_algo
{
    static __inline __m128i clamped_absolute_difference_low(__m128i a, __m128i b, __m128i difference_limit)
    {
        // we need to clamp the result for 2 reasons:
        // 1. there is no integer >= operator in SSE
        // 2. comparison instructions accept only signed integers,
        //    so if difference is bigger than 0x7f, the compare result will be invalid
        __m128i diff = _mm_sub_epi8(_mm_max_epu8(a, b), _mm_min_epu8(a, b));
        return _mm_min_epu8(diff, difference_limit);
    }

    #ifdef ENABLE_DEBUG_DUMP

    static void __forceinline _dump_value_group_low(const TCHAR* name, __m128i part1, bool is_signed=false)
    {
        DUMP_VALUE_S(name, part1, 1, is_signed);
    }

    #define DUMP_VALUE_GROUP(name, ...) _dump_value_group_low(TEXT(name), __VA_ARGS__)

    #else

    #define DUMP_VALUE_GROUP(name, ...) ((void)0)

    #endif


    template <int sample_mode, int ref_part_index>
    static __forceinline void process_plane_info_block(
        pixel_dither_info *&info_ptr, 
        const unsigned char* src_addr_start, 
        const __m128i &src_pitch_vector, 
        __m128i &change, 
        const __m128i &minus_one, 
        const __m128i &width_subsample_vector,
        const __m128i &height_subsample_vector,
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
                change = change_temp;
                // [@ @ @ d @ @ @ c @ @ @ b @ @ @ a]
                break;
            case 1:
                change = _mm_packs_epi32(change, change_temp);
                // [@ h @ g @ f @ e @ d @ c @ b @ a]
                break;
            case 2:
                change = _mm_packs_epi16(change, change_temp);
                // [@ l @ k @ j @ i h g f e d c b a]
                break;
            case 3:
                {
                    __m128i zero = _mm_setzero_si128();

                    __m128i part3 = _mm_srli_si128(change, 8);
                    // [@ @ @ @ @ @ @ @ @ l @ k @ j @ i]

                    __m128i part4 = _mm_packs_epi32(zero, change_temp);
                    // [@ p @ o @ n @ m @ @ @ @ @ @ @ @]

                    __m128i high_part = _mm_packs_epi16(zero, _mm_or_si128(part3, part4));
                    // [p o n m l k j i @ @ @ @ @ @ @ @]

                    __m128i low_mask = _mm_set_epi32(0, 0, -1, -1);
                    change = _cmm_blendv_by_cmp_mask_epi8(high_part, change, low_mask);
                }
                break;
            }
    
        }

        // ref1: bit 0-7
        // left-shift & right-shift 24bits to remove other elements and preserve sign
        __m128i ref1 = info_block;
        ref1 = _mm_slli_epi32(ref1, 24); // << 24
        ref1 = _mm_srai_epi32(ref1, 24); // >> 24

        DUMP_VALUE("ref1", ref1, 4, true);

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
            DUMP_VALUE("ref_pos", ref_offset1, 4, true);
            break;
        case 1:
            // ref1 is guarenteed to be postive
            temp_ref1 = _mm_sra_epi32(ref1, height_subsample_vector);
            ref_offset1 = _cmm_mullo_limit16_epi32(src_pitch_vector, temp_ref1); // packed DWORD multiplication
            DUMP_VALUE("ref_pos", ref_offset1, 4, true);

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
            DUMP_VALUE("ref_pos", ref_offset1, 4, true);

            // ref_px_2 = info.ref2 - src_pitch * info.ref1;
            ref1_fix = _mm_sra_epi32(ref1, height_subsample_vector);
            ref2_fix = _mm_sra_epi32(ref2, width_subsample_vector);
            ref_offset2 = _cmm_mullo_limit16_epi32(src_pitch_vector, ref1_fix); // packed DWORD multiplication
            ref_offset2 = _mm_sub_epi32(ref2_fix, ref_offset2);
            DUMP_VALUE("ref_pos_2", ref_offset2, 4, true);
            break;
        default:
            abort();
        }

        if (info_data_stream){
            _mm_store_si128((__m128i*)info_data_stream, ref_offset1);
            info_data_stream += 16;

            if (sample_mode == 2) {
                _mm_store_si128((__m128i*)info_data_stream, ref_offset2);
                info_data_stream += 16;
            }
        }

        info_ptr += 4;
    }


    static __m128i __inline process_pixels_mode0(__m128i src_pixels, __m128i threshold_vector, const __m128i& ref_pixels)
    {
        __m128i dst_pixels;
        __m128i blend_mask;

        __m128i difference;

        difference = clamped_absolute_difference_low(src_pixels, ref_pixels, threshold_vector);
        // mask: if difference >= threshold, set to 0xff, otherwise 0x00
        // difference is already clamped to threshold, so we compare for equality here
        blend_mask = _mm_cmpeq_epi8(difference, threshold_vector);

        // if mask is 0xff (over threshold), select second operand, otherwise select first
        dst_pixels = _cmm_blendv_by_cmp_mask_epi8(ref_pixels, src_pixels, blend_mask);

        return dst_pixels;
    }

    template<int sample_mode, bool blur_first>
    static __m128i __inline process_pixels_mode12(
        __m128i src_pixels, 
        __m128i threshold_vector, 
        __m128i sign_convert_vector, 
        const __m128i& one_i8, 
        const __m128i& change, 
        const __m128i& ref_pixels_1, 
        const __m128i& ref_pixels_2, 
        const __m128i& ref_pixels_3, 
        const __m128i& ref_pixels_4,
        const __m128i& clamp_high_add,
        const __m128i& clamp_high_sub,
        const __m128i& clamp_low,
        bool need_clamping)
    {
        __m128i dst_pixels;
        __m128i use_orig_pixel_blend_mask;
        __m128i avg;

        __m128i difference;

        if (!blur_first)
        {
            difference = clamped_absolute_difference_low(src_pixels, ref_pixels_1, threshold_vector);
            // mask: if difference >= threshold, set to 0xff, otherwise 0x00
            // difference is already clamped to threshold, so we compare for equality here
            use_orig_pixel_blend_mask = _mm_cmpeq_epi8(difference, threshold_vector);

            difference = clamped_absolute_difference_low(src_pixels, ref_pixels_2, threshold_vector);
            // use OR to combine the masks
            use_orig_pixel_blend_mask = _mm_or_si128(_mm_cmpeq_epi8(difference, threshold_vector), use_orig_pixel_blend_mask);
        }

        avg = _mm_avg_epu8(ref_pixels_1, ref_pixels_2);

        if (sample_mode == 2)
        {

            if (!blur_first)
            {
                difference = clamped_absolute_difference_low(src_pixels, ref_pixels_3, threshold_vector);
                use_orig_pixel_blend_mask = _mm_or_si128(_mm_cmpeq_epi8(difference, threshold_vector), use_orig_pixel_blend_mask);

                difference = clamped_absolute_difference_low(src_pixels, ref_pixels_4, threshold_vector);
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
            difference = clamped_absolute_difference_low(src_pixels, avg, threshold_vector);
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

        if (need_clamping)
        {
            dst_pixels = low_bit_depth_pixels_clamp(dst_pixels, clamp_high_add, clamp_high_sub, clamp_low);
        }
        return dst_pixels;
    }

    template<int sample_mode, bool blur_first>
    static __m128i __forceinline process_pixels(
        __m128i src_pixels_0, 
        __m128i threshold_vector, 
        const __m128i& sign_convert_vector, 
        const __m128i& one_i8, 
        const __m128i& change_1, 
        const __m128i& ref_pixels_1_0,
        const __m128i& ref_pixels_2_0,
        const __m128i& ref_pixels_3_0,
        const __m128i& ref_pixels_4_0,
        const __m128i& clamp_high_add,
        const __m128i& clamp_high_sub,
        const __m128i& clamp_low,
        bool need_clamping)
    {
        switch (sample_mode)
        {
        case 0:
            return sse_low_dither_algo::process_pixels_mode0(src_pixels_0, threshold_vector, ref_pixels_1_0);
            break;
        case 1:
        case 2:
            return sse_low_dither_algo::process_pixels_mode12<sample_mode, blur_first>(
                        src_pixels_0, 
                        threshold_vector, 
                        sign_convert_vector, 
                        one_i8, 
                        change_1, 
                        ref_pixels_1_0, 
                        ref_pixels_2_0, 
                        ref_pixels_3_0, 
                        ref_pixels_4_0, 
                        clamp_high_add, 
                        clamp_high_sub, 
                        clamp_low, 
                        need_clamping);
            break;
        default:
            abort();
        }
        return _mm_setzero_si128();
    }

    template<bool aligned>
    static __m128i load_m128(const unsigned char *ptr)
    {
        if (aligned)
        {
            return _mm_load_si128((const __m128i*)ptr);
        } else {
            return _mm_loadu_si128((const __m128i*)ptr);
        }
    }

    template<int sample_mode>
    static void __forceinline read_reference_pixels(
        const unsigned char* src_px_start,
        const char* info_data_start,
        unsigned char ref_pixels_1[16],
        unsigned char ref_pixels_2[16],
        unsigned char ref_pixels_3[16],
        unsigned char ref_pixels_4[16])
    {

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
    
        switch (sample_mode)
        {
        case 0:
            for (int i = 0; i < 16; i++)
            {
                ref_pixels_1[i] = *(src_px_start + i + *(int*)(info_data_start + 4 * i));
            }
            break;
        case 1:
            for (int i = 0; i < 16; i++)
            {
                ref_pixels_1[i] = *(src_px_start + i + *(int*)(info_data_start + 4 * i));
                ref_pixels_2[i] = *(src_px_start + i + -*(int*)(info_data_start + 4 * i));
            }
            break;
        case 2:
            for (int i = 0; i < 16; i++)
            {
                ref_pixels_1[i] = *(src_px_start + i + *(int*)(info_data_start + 4 * (i + i / 4 * 4)));
                ref_pixels_2[i] = *(src_px_start + i + *(int*)(info_data_start + 4 * (i + i / 4 * 4 + 4)));
                ref_pixels_3[i] = *(src_px_start + i + -*(int*)(info_data_start + 4 * (i + i / 4 * 4)));
                ref_pixels_4[i] = *(src_px_start + i + -*(int*)(info_data_start + 4 * (i + i / 4 * 4 + 4)));
            }
            break;
        }
    }


    template<int sample_mode, bool blur_first, bool aligned>
    static void __cdecl _process_plane_sse_impl(const process_plane_params& params, process_plane_context* context)
    {

        DUMP_INIT("sse", params.plane, params.plane_width_in_pixels);

        pixel_dither_info* info_ptr = params.info_ptr_base;

        __m128i src_pitch_vector = _mm_set1_epi32(params.src_pitch);
           
        __m128i threshold_vector;
    
        threshold_vector = _mm_set1_epi8((unsigned char)params.threshold);

        __m128i sign_convert_vector = _mm_set1_epi8(0x80u);

        // general-purpose constant
        __m128i minus_one = _mm_set1_epi32(-1);

        __m128i one_i8 = _mm_set1_epi8(1);
    
        bool use_cached_info = false;

        char* info_data_stream = NULL;

        info_cache *cache = NULL;
   
        bool need_clamping = (params.pixel_min > 0 || 
                              params.pixel_max < 0xffff);
        __m128i clamp_high_add = _mm_setzero_si128();
        __m128i clamp_high_sub = _mm_setzero_si128();
        __m128i clamp_low = _mm_setzero_si128();
        if (need_clamping)
        {
            clamp_low = _mm_set1_epi8((unsigned char)(params.pixel_min >> 8));
            clamp_high_add = _mm_sub_epi8(_mm_set1_epi8((unsigned char)0xff), _mm_set1_epi8((unsigned char)(params.pixel_max >> 8)));
            clamp_high_sub = _mm_add_epi8(clamp_high_add, clamp_low);
        }

        __m128i width_subsample_vector = _mm_set_epi32(0, 0, 0, params.width_subsampling);
        __m128i height_subsample_vector = _mm_set_epi32(0, 0, 0, params.height_subsampling);


        __declspec(align(16))
        char dummy_info_buffer[128];

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

        int info_cache_block_size = (sample_mode == 2 ? 128 : 64);

        for (int row = 0; row < params.plane_height_in_pixels; row++)
        {
            const unsigned char* src_px = params.src_plane_ptr + params.src_pitch * row;
            unsigned char* dst_px = params.dst_plane_ptr + params.dst_pitch * row;

            // info_ptr = info_ptr_base + info_stride * row;
            // doesn't need here, since info_stride equals to count of pixels that are needed to process in each row

            int processed_pixels = 0;

            while (processed_pixels < params.plane_width_in_pixels)
            {
                __m128i change;
            
                __m128i ref_pixels_1_0;
                __m128i ref_pixels_2_0;
                __m128i ref_pixels_3_0;
                __m128i ref_pixels_4_0;

    #define READ_REFS(data_stream) read_reference_pixels<sample_mode>( \
                        src_px, \
                        data_stream, \
                        ref_pixels_1_0.m128i_u8, \
                        ref_pixels_2_0.m128i_u8, \
                        ref_pixels_3_0.m128i_u8, \
                        ref_pixels_4_0.m128i_u8)

                char * data_stream_block_start;

                if (LIKELY(use_cached_info)) {
                    data_stream_block_start = info_data_stream;
                    info_data_stream += info_cache_block_size;
                    if (sample_mode > 0) {
                        change = _mm_load_si128((__m128i*)info_data_stream);
                        info_data_stream += 16;
                    }
                } else {
                    // we need to process the info block
                    change = _mm_setzero_si128();

                    char * data_stream_ptr = info_data_stream;
                    if (!data_stream_ptr)
                    {
                        data_stream_ptr = dummy_info_buffer;
                    }

                    data_stream_block_start = data_stream_ptr;
            
        #define PROCESS_INFO_BLOCK(n) \
                    process_plane_info_block<sample_mode, n>(info_ptr, src_px, src_pitch_vector, change, minus_one, width_subsample_vector, height_subsample_vector, data_stream_ptr);
            
                    PROCESS_INFO_BLOCK(0);
                    PROCESS_INFO_BLOCK(1);
                    PROCESS_INFO_BLOCK(2);
                    PROCESS_INFO_BLOCK(3);

        #undef PROCESS_INFO_BLOCK

                
                    if (info_data_stream) {
                        info_data_stream += info_cache_block_size;
                        assert(info_data_stream == data_stream_ptr);
                    }

                    if (sample_mode > 0) {

                        if (info_data_stream) {

                            _mm_store_si128((__m128i*)info_data_stream, change);
                            info_data_stream += 16;
                        }
                    }
                }

                READ_REFS(data_stream_block_start);

                #undef READ_REFS
            
            
                DUMP_VALUE_GROUP("change", change, true);
                DUMP_VALUE_GROUP("ref_1_up", ref_pixels_1_0);
                DUMP_VALUE_GROUP("ref_2_up", ref_pixels_2_0);
                DUMP_VALUE_GROUP("ref_3_up", ref_pixels_3_0);
                DUMP_VALUE_GROUP("ref_4_up", ref_pixels_4_0);

                __m128i src_pixels;
                // abuse the guard bytes on the end of frame, as long as they are present there won't be segfault
                // garbage data is not an problem
                src_pixels = load_m128<aligned>(src_px);
                DUMP_VALUE_GROUP("src_px_up", src_pixels);

                __m128i dst_pixels = process_pixels<sample_mode, blur_first>(
                                         src_pixels, 
                                         threshold_vector, 
                                         sign_convert_vector, 
                                         one_i8, 
                                         change, 
                                         ref_pixels_1_0, 
                                         ref_pixels_2_0, 
                                         ref_pixels_3_0, 
                                         ref_pixels_4_0, 
                                         clamp_high_add, 
                                         clamp_high_sub, 
                                         clamp_low, 
                                         need_clamping);

            
                _mm_store_si128((__m128i*)dst_px, dst_pixels);
                dst_px += 16;

                processed_pixels += 16;
                src_px += 16;
            }
            DUMP_NEXT_LINE();
        }
    
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

        DUMP_FINISH();
    }


    template<int sample_mode, bool blur_first>
    static void __cdecl process_plane_sse_impl(const process_plane_params& params, process_plane_context* context)
    {
        if ( ( (POINTER_INT)params.src_plane_ptr & (PLANE_ALIGNMENT - 1) ) == 0 && (params.src_pitch & (PLANE_ALIGNMENT - 1) ) == 0 )
        {
            _process_plane_sse_impl<sample_mode, blur_first, true>(params, context);
        } else {
            _process_plane_sse_impl<sample_mode, blur_first, false>(params, context);
        }
    }
};

#undef DUMP_VALUE_GROUP
