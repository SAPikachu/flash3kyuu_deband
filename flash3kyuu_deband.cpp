// flash3kyuu_deband.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "flash3kyuu_deband.h"

void check_parameter_range(int value, int lower_bound, int upper_bound, char* param_name, IScriptEnvironment* env) {
	if (value < lower_bound || value > upper_bound) {
		char msg[1024];
		sprintf_s(msg, "flash3kyuu_deband: Invalid value for parameter %s: %d, must be %d ~ %d.",
			param_name, value, lower_bound, upper_bound);
		env->ThrowError(_strdup(msg));
	}
}

AVSValue __cdecl Create_flash3kyuu_deband(AVSValue args, void* user_data, IScriptEnvironment* env){
	PClip child = args[0].AsClip();
	const VideoInfo& vi = child->GetVideoInfo();
	if (!vi.IsYV12()) {
		env->ThrowError("flash3kyuu_deband: Only YV12 clip is supported.");
	}
	if (vi.IsFieldBased()) {
		env->ThrowError("flash3kyuu_deband: Field-based clip is not supported.");
	}

	int range = args[1].AsInt(15);
	int Y = args[2].AsInt(1);
	int Cb = args[3].AsInt(1);
	int Cr = args[4].AsInt(1);
	int ditherY = args[5].AsInt(1);
	int ditherC = args[6].AsInt(1);
	int sample_mode = args[7].AsInt(1);
	int seed = args[8].AsInt(0);
	bool blur_first = args[9].AsBool(true);
	bool diff_seed_for_each_frame = args[10].AsBool(false);

#define CHECK_PARAM(value, lower_bound, upper_bound) \
	check_parameter_range(value, lower_bound, upper_bound, #value, env);

	CHECK_PARAM(range, 0, 31);
	CHECK_PARAM(Y, 0, 31);
	CHECK_PARAM(Cb, 0, 31);
	CHECK_PARAM(Cr, 0, 31);
	CHECK_PARAM(ditherY, 0, 31);
	CHECK_PARAM(ditherC, 0, 31);
	CHECK_PARAM(sample_mode, 0, 2);
	CHECK_PARAM(seed, 0, 127);

	return new flash3kyuu_deband(child, range, Y, Cb, Cr, 
		ditherY, ditherC, sample_mode, seed, 
		blur_first, diff_seed_for_each_frame);
}

FLASH3KYUU_DEBAND_API const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->AddFunction("flash3kyuu_deband", 
		"c[range]i[Y]i[Cb]i[Cr]i[ditherY]i[ditherC]i[sample_mode]i[seed]i[blur_first]b[diff_seed]b", 
		Create_flash3kyuu_deband, 
		NULL);

	return "flash3kyuu_deband";
}

flash3kyuu_deband::flash3kyuu_deband(PClip child, int range, int Y, int Cb, int Cr, 
		int ditherY, int ditherC, int sample_mode, int seed,
		bool blur_first, bool diff_seed_for_each_frame) :
			GenericVideoFilter(child),
			_range_raw(range),
			_range(range),
			_Y(Y),
			_Cb(Cb),
			_Cr(Cr),
			_ditherY(ditherY),
			_ditherC(ditherC),
			_sample_mode(sample_mode),
			_seed(seed),
			_blur_first(blur_first),
			_diff_seed_for_each_frame(diff_seed_for_each_frame)
{
	this->init();
}

void flash3kyuu_deband::init(void) 
{
	int new_Y, new_Cb, new_Cr;
	int yCbCr_coeff;
	if (_sample_mode > 0 && _blur_first) {
		yCbCr_coeff = 2;
	} else {
		yCbCr_coeff = 4;
	}

	new_Y = _Y * yCbCr_coeff;
	new_Cb = _Cb * yCbCr_coeff;
	new_Cr = _Cr * yCbCr_coeff;

	int new_range = _range * 2 + 1;
	int new_ditherY = _ditherY * 2 + 1;
	int new_ditherC = _ditherC * 2 + 1;

	for (int i = 0; i < 64; i++)
	{
		_range_lut[i] = i % new_range - _range;
		_ditherY_lut[i] = i % new_ditherY - _ditherY;
		_ditherC_lut[i] = i % new_ditherC - _ditherC;
	}

	_h_ind_masks = new signed char[vi.height];

	for (int i = 0; i < vi.height; i++)
	{
		_h_ind_masks[i] = (i >= _range && vi.height - i >= _range) ? -1 : 0;
	}

	_Y = new_Y;
	_Cb = new_Cb;
	_Cr = new_Cr;

	_range = new_range;
	_ditherY = new_ditherY;
	_ditherC = new_ditherC;
}

void flash3kyuu_deband::process_pixel_out_of_range_mode0(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	*dst_px = *src_px;
}

void flash3kyuu_deband::process_pixel_out_of_range_mode12(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	*dst_px = *src_px + dither_lut[(seed >> shift_bits) & 0x3F];
}

void flash3kyuu_deband::process_pixel_mode0(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	int ref_px = src_pitch * (ind_mask & _range_lut[(seed >> 10) & 0x3F]
                + _range_lut[(seed >> 5) & 0x3F]);

	if (!is_full_plane) 
	{
		ref_px >>= 2;
	}

	int diff = *src_px - src_px[ref_px];

	if ( (diff ^ (diff >> 31)) - (diff >> 31) >= threshold )
	{
		*dst_px = *src_px;
	} else {
		*dst_px = src_px[ref_px];
	}

}

void flash3kyuu_deband::process_pixel_mode1_noblur(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	int ref_px = src_pitch * (ind_mask & _range_lut[(seed >> 10) & 0x3F]
                + _range_lut[(seed >> 5) & 0x3F]);

	if (!is_full_plane) 
	{
		ref_px >>= 2;
	}
	int diff = *src_px - src_px[ref_px];
	int diff_n = *src_px - src_px[-ref_px];

	int change = dither_lut[(seed >> shift_bits) & 0x3F];

	if ( (diff ^ (diff >> 31)) - (diff >> 31) >= threshold ||
		 (diff_n ^ (diff_n >> 31)) - (diff_n >> 31) >= threshold)
	{
		*dst_px = *src_px + change;
	} else {
		*dst_px = change + (char)(((int)*src_px + (int)src_px[ref_px]) >> 1);
	}

}

void flash3kyuu_deband::process_pixel_mode1_blur(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	int ref_px = src_pitch * (ind_mask & _range_lut[(seed >> 10) & 0x3F]
                + _range_lut[(seed >> 5) & 0x3F]);
				
	if (!is_full_plane) 
	{
		ref_px >>= 2;
	}

	int avg = ((int)src_px[ref_px] + (int)src_px[-ref_px]) >> 1;
	int diff = avg - *src_px;

	int change = dither_lut[(seed >> shift_bits) & 0x3F];

	if ( (diff ^ (diff >> 31)) - (diff >> 31) >= threshold )
	{
		*dst_px = *src_px + change;
	} else {
		*dst_px = (char)((change + avg) & 0xFF);
	}

}

void flash3kyuu_deband::process_pixel_mode2_noblur(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	int ref_ind_x1 = _range_lut[(seed >> 5) & 0x3F];
	int ref_ind_x2 = _range_lut[(seed >> 10) & 0x3F];

	int ref_px = src_pitch * (ind_mask & ref_ind_x2) + ref_ind_x1;
	int ref_px_2 = ref_ind_x2 - src_pitch * (ref_ind_x1 & ind_mask);
	
	if (!is_full_plane) 
	{
		ref_px >>= 2;
		ref_px_2 >>= 2;
	}
	
	int diff1 = src_px[ref_px] - *src_px;
	int diff2 = src_px[-ref_px] - *src_px;
	int diff3 = src_px[ref_px_2] - *src_px;
	int diff4 = src_px[-ref_px_2] - *src_px;

	int change = dither_lut[(seed >> shift_bits) & 0x3F];

	if ( (diff1 ^ (diff1 >> 31)) - (diff1 >> 31) >= threshold ||
		 (diff2 ^ (diff2 >> 31)) - (diff2 >> 31) >= threshold ||
		 (diff3 ^ (diff3 >> 31)) - (diff3 >> 31) >= threshold ||
		 (diff4 ^ (diff4 >> 31)) - (diff4 >> 31) >= threshold )
	{
		*dst_px = *src_px + change;
	} else {
		*dst_px = (char)(((((int)src_px[ref_px] + (int)src_px[-ref_px] + (int)src_px[ref_px_2] + (int)src_px[-ref_px_2]) >> 2) + change) & 0xFF);
	}

}

void flash3kyuu_deband::process_pixel_mode2_blur(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	int ref_ind_x1 = _range_lut[(seed >> 5) & 0x3F];
	int ref_ind_x2 = _range_lut[(seed >> 10) & 0x3F];

	int ref_px = src_pitch * (ind_mask & ref_ind_x2) + ref_ind_x1;
	int ref_px_2 = ref_ind_x2 - src_pitch * (ref_ind_x1 & ind_mask);
	
	if (!is_full_plane) 
	{
		ref_px >>= 2;
		ref_px_2 >>= 2;
	}

	int avg = ((int)src_px[ref_px] + (int)src_px[-ref_px] + (int)src_px[ref_px_2] + (int)src_px[-ref_px_2]) >> 2;

	int diff = avg - *src_px;

	int change = dither_lut[(seed >> shift_bits) & 0x3F];

	if ( (diff ^ (diff >> 31)) - (diff >> 31) >= threshold )
	{
		*dst_px = *src_px + change;
	} else {
		*dst_px = (char)((avg + change) & 0xFF);
	}

}

void flash3kyuu_deband::process_plane(int n, PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane)
{
	const unsigned char* srcp = src->GetReadPtr(plane);
	const int src_pitch = src->GetPitch(plane);
	const int src_width = src->GetRowSize(plane);
	const int src_height = src->GetHeight(plane);
	
	int dst_pitch;
	int dst_width;
	int dst_height;

	dst_pitch = dst->GetPitch(plane);
	dst_width = dst->GetRowSize(plane);
	dst_height = dst->GetHeight(plane);

	int seed = 0x92D68CA2 - _seed;
	if (_diff_seed_for_each_frame) 
	{
		seed -= n;
	}

	int* dither_lut;
	int shift_bits;
	int threshold;
	bool is_full_plane = (plane == PLANAR_Y);

	switch (plane & 7)
	{
	case PLANAR_Y:
		dither_lut = _ditherY_lut;
		shift_bits = 15;
		threshold = _Y;
		break;
	case PLANAR_U:
		dither_lut = _ditherC_lut;
		shift_bits = 20;
		threshold = _Cb;
		break;
	case PLANAR_V:
		dither_lut = _ditherC_lut;
		shift_bits = 25;
		threshold = _Cr;
		break;
	}

	int range = is_full_plane ? _range_raw : _range_raw >> 2;
	for (int i = 0; i < src_height; i++)
	{
		const unsigned char* src_px = srcp + src_pitch * i;
		unsigned char* dst_px = dstp + dst_pitch * i;
		int ind_mask = is_full_plane ? _h_ind_masks[i] : _h_ind_masks[i << 2];

		for (int j = 0; j < src_width; j++)
		{
			if (j < range || src_width - j <= range) 
			{
				if (_sample_mode == 0) 
				{
					process_pixel_out_of_range_mode0(src_px, src_pitch,dst_px, seed, dither_lut, shift_bits, ind_mask, threshold, is_full_plane);
				} else {
					for (int k = 0; k < (is_full_plane ? 1 : 4); k++)
					{
						int seed_tmp = (((seed << 13) ^ (unsigned int)seed) >> 17) ^ (seed << 13) ^ seed;
						seed = 32 * seed_tmp ^ seed_tmp;
					}
					process_pixel_out_of_range_mode12(src_px, src_pitch,dst_px, seed, dither_lut, shift_bits, ind_mask, threshold, is_full_plane);
				}
			} else {
				for (int k = 0; k < (is_full_plane ? 1 : 4); k++)
				{
					int seed_tmp = (((seed << 13) ^ (unsigned int)seed) >> 17) ^ (seed << 13) ^ seed;
					seed = 32 * seed_tmp ^ seed_tmp;
				}
				switch (_sample_mode)
				{
				case 0:
					process_pixel_mode0(src_px, src_pitch,dst_px, seed, dither_lut, shift_bits, ind_mask, threshold, is_full_plane);
					break;
				case 1:
					if (_blur_first) 
					{
						process_pixel_mode1_blur(src_px, src_pitch,dst_px, seed, dither_lut, shift_bits, ind_mask, threshold, is_full_plane);
					} else {
						process_pixel_mode1_noblur(src_px, src_pitch,dst_px, seed, dither_lut, shift_bits, ind_mask, threshold, is_full_plane);
					}
					break;
				case 2:
					if (_blur_first) 
					{
						process_pixel_mode2_blur(src_px, src_pitch,dst_px, seed, dither_lut, shift_bits, ind_mask, threshold, is_full_plane);
					} else {
						process_pixel_mode2_noblur(src_px, src_pitch,dst_px, seed, dither_lut, shift_bits, ind_mask, threshold, is_full_plane);
					}
					break;
				}
			}
			src_px++;
			dst_px++;
		}
	}
}

PVideoFrame __stdcall flash3kyuu_deband::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi);
	
	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_Y), PLANAR_Y);
	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_U), PLANAR_U);
	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_V), PLANAR_V);
	return dst;
}