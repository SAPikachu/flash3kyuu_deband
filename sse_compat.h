
#if !defined(SSE_LIMIT) || SSE_LIMIT >= 41

#define _cmm_blendv_by_cmp_mask_epi8 _mm_blendv_epi8

#define _cmm_mullo_limit16_epi32 _mm_mullo_epi32

#else

static __m128i __inline _cmm_blendv_by_cmp_mask_epi8( 
   __m128i a,
   __m128i b,
   __m128i mask 
)
{
	// mask is always returned from cmp functions, so it can be used directly
	__m128i b_mask = _mm_and_si128(mask, b);
	__m128i a_mask = _mm_andnot_si128(mask, a);

	return _mm_or_si128(a_mask, b_mask);
}

static __m128i __inline _cmm_mullo_limit16_epi32(__m128i a, __m128i b)
{
	// input can always be fit in 16bit

	a = _mm_packs_epi32(a, a);

	b = _mm_packs_epi32(b, b);
	
	__m128i lo_part = _mm_mullo_epi16(a, b);

	__m128i hi_part = _mm_mulhi_epi16(a, b);

	return _mm_unpacklo_epi16(lo_part, hi_part);
}

static __m128i __inline _mm_min_epu16(__m128i val1, __m128i val2)
{
    __m128i sign_convert = _mm_set1_epi16((short)0x8000);
    val1 = _mm_sub_epi16(val1, sign_convert);
    val2 = _mm_sub_epi16(val2, sign_convert);

    return _mm_add_epi16(_mm_min_epi16(val1, val2), sign_convert);
}
#endif



#if !defined(SSE_LIMIT) || SSE_LIMIT >= 31

template <int ref_part_index>
static __m128i __inline process_change_part(__m128i current_value, __m128i extracted_part)
{
	__m128i change_temp = extracted_part;
	switch (ref_part_index)
	{
	case 0:
		// right-shift 16 bits
		change_temp = _mm_srli_epi32(change_temp, 16);
		// first value, no need to merge
		return change_temp;
		break;
	case 1:
		// right-shift 8 bits
		change_temp = _mm_srli_epi32(change_temp, 8);
		break;
	case 2:
		// already in correct place, do nothing
		break;
	case 3:
		// left-shift 8 bits
		change_temp = _mm_slli_epi32(change_temp, 8);
		break;
	default:
		// CODING ERROR!
		abort();
	}
	
	// combine them
	return _mm_or_si128(current_value, change_temp);
}


static __m128i __inline postprocess_change_value(__m128i change, __m128i change_shuffle_mask)
{
	// shuffle delta values to correct place
	return _mm_shuffle_epi8(change, change_shuffle_mask);
}

static __m128i __inline _cmm_negate_all_epi32(__m128i a, __m128i minus_one)
{
	return _mm_sign_epi32(a, minus_one);
}

#else

template <int ref_part_index>
static __m128i __inline process_change_part(__m128i current_value, __m128i extracted_part)
{
	__m128i zero = _mm_setzero_si128();
	__m128i change_temp;
	// shift to pos 0
	change_temp = _mm_srli_epi32(extracted_part, 16);
	// [0, 0, 0, d, 0, 0, 0, c, 0, 0, 0, b, 0, 0, 0, a]

	change_temp = _mm_packus_epi16(change_temp, zero);
	// [0, 0, 0, 0, 0, 0, 0, 0, 0, d, 0, c, 0, b, 0, a]

	change_temp = _mm_packus_epi16(change_temp, zero);
	// [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, d, c, b, a]

	switch (ref_part_index)
	{
	case 0:
		// first value, no need to merge
		return change_temp;
		break;
	case 1:
		change_temp = _mm_slli_si128(change_temp, 4);
		break;
	case 2:
		change_temp = _mm_slli_si128(change_temp, 8);
		break;
	case 3:
		change_temp = _mm_slli_si128(change_temp, 12);
		break;
	default:
		// CODING ERROR!
		abort();
	}
	
	// combine them
	return _mm_or_si128(current_value, change_temp);
}


static __m128i __inline postprocess_change_value(__m128i change, __m128i change_shuffle_mask)
{
	// no need to postprocess
	return change;
}

static __m128i __inline _cmm_negate_all_epi32(__m128i a, __m128i minus_one)
{
	return _mm_sub_epi32(_mm_setzero_si128(), a);
}

static __m128i __inline _mm_abs_epi32 (__m128i a)
{
    __m128i mask = _mm_srai_epi32(a, 31);
    __m128i fix = _mm_srli_epi32(a, 31);
    a = _mm_xor_si128(a, mask);
    return _mm_add_epi32(a, fix);
}
#endif
