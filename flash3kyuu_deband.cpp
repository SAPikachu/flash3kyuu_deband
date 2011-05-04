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
			_diff_seed_for_each_frame(diff_seed_for_each_frame),
			_h_ind_masks(NULL),
			_ref_px_y_lut(NULL),
			_ref_px_y_2_lut(NULL),
			_ref_px_c_lut(NULL),
			_ref_px_c_2_lut(NULL),
			_change_y_lut(NULL),
			_change_cb_lut(NULL),
			_change_cr_lut(NULL)
{
	this->init();
}

void flash3kyuu_deband::destroy_frame_luts(void)
{
	delete [] _ref_px_y_lut;
	delete [] _ref_px_y_2_lut;
	delete [] _ref_px_c_lut;
	delete [] _ref_px_c_2_lut;
	delete [] _change_y_lut;
	delete [] _change_cb_lut;
	delete [] _change_cr_lut;
	
	_ref_px_y_lut = NULL;
	_ref_px_y_2_lut = NULL;
    _ref_px_c_lut = NULL;
    _ref_px_c_2_lut = NULL;
	_change_y_lut = NULL;
	_change_cb_lut = NULL;
	_change_cr_lut = NULL;
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
	_ref_px_y_lut = new int[vi.width * vi.height];
	_ref_px_y_2_lut = new int[vi.width * vi.height];
	_ref_px_c_lut = new int[vi.width * vi.height / 4];
	_ref_px_c_2_lut = new int[vi.width * vi.height / 4];
	_change_y_lut = new int[vi.width * vi.height];
	_change_cb_lut = new int[vi.width * vi.height / 4];
	_change_cr_lut = new int[vi.width * vi.height / 4];
	
	int* ref_px_y_lut = _ref_px_y_lut;
	int* ref_px_y_2_lut = _ref_px_y_2_lut;
	int* ref_px_c_lut = _ref_px_c_lut;
	int* ref_px_c_2_lut = _ref_px_c_2_lut;
	int* change_y_lut = _change_y_lut;
	int* change_cb_lut = _change_cb_lut;
	int* change_cr_lut = _change_cr_lut;

	for (int y = 0; y < vi.height; y++)
	{
		for (int x = 0; x < vi.width; x++)
		{
			int seed_tmp = (((seed << 13) ^ (unsigned int)seed) >> 17) ^ (seed << 13) ^ seed;
			seed = 32 * seed_tmp ^ seed_tmp;

			if (_sample_mode < 2)
			{
				*ref_px_y_lut = (_h_ind_masks[y] & _range_lut[(seed >> 10) & 0x3F]);
			} else {
				*ref_px_y_lut = _range_lut[(seed >> 5) & 0x3F];
				*ref_px_y_2_lut = _range_lut[(seed >> 10) & 0x3F];
			}

			*change_y_lut = _range_lut[(seed >> 15) & 0x3F];

			if ((x & 1) == 0 && (y & 1) == 0) {
				*ref_px_c_lut = *ref_px_y_lut >> 1;
				*ref_px_c_2_lut = *ref_px_y_2_lut >> 1;
				*change_cb_lut = _range_lut[(seed >> 20) & 0x3F];
				*change_cr_lut = _range_lut[(seed >> 25) & 0x3F];
				ref_px_c_lut++;
				ref_px_c_2_lut++;
				change_cb_lut++;
				change_cr_lut++;
			}
			ref_px_y_lut++;
			ref_px_y_2_lut++;
			change_y_lut++;
		}
	}
}

flash3kyuu_deband::~flash3kyuu_deband()
{
	delete [] _h_ind_masks;
	destroy_frame_luts();
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
		_h_ind_masks[i] = (i >= _range && vi.height - i > _range) ? -1 : 0;
	}

	_Y = new_Y;
	_Cb = new_Cb;
	_Cr = new_Cr;

	_range = new_range;
	_ditherY = new_ditherY;
	_ditherC = new_ditherC;

	init_frame_luts(0);
}

inline unsigned char sadd8(unsigned char a, int b)
{
    int s = (int)a+b;
    return (unsigned char)(s < 0 ? 0 : s > 0xFF ? 0xFF : s);
}


void flash3kyuu_deband::process_pixel_out_of_range_mode0(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	*dst_px = *src_px;
}

void flash3kyuu_deband::process_pixel_out_of_range_mode12(const unsigned char* src_px, int src_pitch, unsigned char* dst_px, int seed, int* dither_lut, int shift_bits, int ind_mask, int threshold, bool is_full_plane)
{
	*dst_px = sadd8(*src_px, dither_lut[(seed >> shift_bits) & 0x3F]);
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

	int threshold;
	int* ref_lut_1;
	int* ref_lut_2;
	int* change_lut;

	switch (plane & 7)
	{
	case PLANAR_Y:
		ref_lut_1 = _ref_px_y_lut;
        ref_lut_2 = _ref_px_y_2_lut;
		change_lut = _change_y_lut;
		threshold = _Y;
		break;
	case PLANAR_U:
		ref_lut_1 = _ref_px_c_lut;
		ref_lut_2 = _ref_px_c_2_lut;
		change_lut = _change_cb_lut;
		threshold = _Cb;
		break;
	case PLANAR_V:
		ref_lut_1 = _ref_px_c_lut;
		ref_lut_2 = _ref_px_c_2_lut;
		change_lut = _change_cr_lut;
		threshold = _Cr;
		break;
	}

	int range = (plane == PLANAR_Y ? _range_raw : _range_raw >> 1);
	for (int i = 0; i < src_height; i++)
	{
		const unsigned char* src_px = srcp + src_pitch * i;
		unsigned char* dst_px = dstp + dst_pitch * i;

		for (int j = 0; j < src_width; j++)
		{
			if (j < range || src_width - j <= range) 
			{
				if (_sample_mode == 0) 
				{
					*dst_px = *src_px;
				} else {
					*dst_px = sadd8(*src_px, *change_lut);
				}
			} else {
#define IS_ABOVE_THRESHOLD(diff) ( (diff ^ (diff >> 31)) - (diff >> 31) >= threshold )
				assert(abs(*ref_lut_1) <= i && abs(*ref_lut_1) + i < src_height);
				if (_sample_mode == 0) 
				{
					int ref_px = *ref_lut_1 * src_pitch;
					int diff = *src_px - src_px[ref_px];
					if (IS_ABOVE_THRESHOLD(diff)) {
						*dst_px = *src_px;
					} else {
						*dst_px = src_px[ref_px];
					}
				} else {
					int avg;
					bool use_org_px_as_base;
					int ref_px, ref_px_2;
					if (_sample_mode == 1)
					{
						ref_px = *ref_lut_1 * src_pitch;
						avg = ((int)src_px[ref_px] + (int)src_px[-ref_px]) >> 1;
						if (_blur_first)
						{
							int diff = avg - *src_px;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff);
						} else {
							int diff = *src_px - src_px[ref_px];
							int diff_n = *src_px - src_px[-ref_px];
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff) && IS_ABOVE_THRESHOLD(diff_n);
						}
					} else {
						assert(abs(*ref_lut_1) <= j && abs(*ref_lut_1) + j < src_width);
						assert(abs(*ref_lut_2) <= i && abs(*ref_lut_2) + i < src_height);
						assert(abs(*ref_lut_2) <= j && abs(*ref_lut_2) + j < src_width);
						
						ref_px = src_pitch * *ref_lut_1 + *ref_lut_2;
						ref_px_2 = *ref_lut_2 - src_pitch * *ref_lut_1;

						avg = (((int)src_px[ref_px] + 
							    (int)src_px[-ref_px] + 
								(int)src_px[ref_px_2] + 
								(int)src_px[-ref_px_2]) >> 2);
						if (_blur_first)
						{
							int diff = avg - *src_px;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff);
						} else {
							int diff1 = src_px[ref_px] - *src_px;
							int diff2 = src_px[-ref_px] - *src_px;
							int diff3 = src_px[ref_px_2] - *src_px;
							int diff4 = src_px[-ref_px_2] - *src_px;
							use_org_px_as_base = IS_ABOVE_THRESHOLD(diff1) && 
												 IS_ABOVE_THRESHOLD(diff2) &&
												 IS_ABOVE_THRESHOLD(diff3) && 
												 IS_ABOVE_THRESHOLD(diff4);
						}
					}
					if (use_org_px_as_base) {
						*dst_px = sadd8(*src_px, *change_lut);
					} else {
						*dst_px = sadd8(avg, *change_lut);
					}
				}
			}
			src_px++;
			dst_px++;
			ref_lut_1++;
			ref_lut_2++;
			change_lut++;
		}
	}
}

PVideoFrame __stdcall flash3kyuu_deband::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi);
	
	if (_diff_seed_for_each_frame)
	{
		init_frame_luts(n);
	}

	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_Y), PLANAR_Y);
	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_U), PLANAR_U);
	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_V), PLANAR_V);
	return dst;
}