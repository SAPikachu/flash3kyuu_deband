// flash3kyuu_deband.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "flash3kyuu_deband.h"
#include "impl_dispatch.h"

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
	unsigned char Y = (unsigned char)args[2].AsInt(1);
	unsigned char Cb = (unsigned char)args[3].AsInt(1);
	unsigned char Cr = (unsigned char)args[4].AsInt(1);
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

flash3kyuu_deband::flash3kyuu_deband(PClip child, int range, unsigned char Y, unsigned char Cb, unsigned char Cr, 
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
			_y_info(NULL),
			_cb_info(NULL),
			_cr_info(NULL),
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

	int y_stride = FRAME_LUT_STRIDE(vi.width);
	int y_size = sizeof(pixel_dither_info) * y_stride * vi.height;
	_y_info = (pixel_dither_info*)_aligned_malloc(y_size, FRAME_LUT_ALIGNMENT);

	int c_stride = FRAME_LUT_STRIDE(vi.width / 2);
	int c_size = sizeof(pixel_dither_info) * c_stride * (vi.height / 2);
	_cb_info = (pixel_dither_info*)_aligned_malloc(c_size, FRAME_LUT_ALIGNMENT);
	_cr_info = (pixel_dither_info*)_aligned_malloc(c_size, FRAME_LUT_ALIGNMENT);

	// ensure unused items are also initialized
	memset(_y_info, 0, y_size);
	memset(_cb_info, 0, c_size);
	memset(_cr_info, 0, c_size);

	pixel_dither_info *y_info_ptr, *cb_info_ptr, *cr_info_ptr;

	for (int y = 0; y < vi.height; y++)
	{
		y_info_ptr = _y_info + y * y_stride;
		cb_info_ptr = _cb_info + (y / 2) * c_stride;
		cr_info_ptr = _cr_info + (y / 2) * c_stride;
		for (int x = 0; x < vi.width; x++)
		{
			int seed_tmp = (((seed << 13) ^ (unsigned int)seed) >> 17) ^ (seed << 13) ^ seed;
			seed = 32 * seed_tmp ^ seed_tmp;

			if (x < _range_raw || vi.width - x <= _range_raw) {
				continue;
			}

			pixel_dither_info info_y = {0, 0, 0, 0};

			if (_sample_mode < 2)
			{
				info_y.ref1 = (signed char)(_h_ind_masks[y] & _range_lut[(seed >> 10) & 0x3F]);
			} else {
				info_y.ref1 = (signed char)(_h_ind_masks[y] & _range_lut[(seed >> 5) & 0x3F]);
				info_y.ref2 = (signed char)(_h_ind_masks[y] & _range_lut[(seed >> 10) & 0x3F]);
			}

			info_y.change = (signed char)_ditherY_lut[(seed >> 15) & 0x3F];
			*y_info_ptr = info_y;

			if ((x & 1) == 0 && (y & 1) == 0) {
				pixel_dither_info info_c = {0, 0, 0, 0};
				// use absolute value to calulate correct ref, and restore sign
				info_c.ref1 = (abs(info_y.ref1) >> 1) * (info_y.ref1 >> 7);
				info_c.ref2 = (abs(info_y.ref2) >> 1) * (info_y.ref2 >> 7);

				info_c.change = _ditherC_lut[(seed >> 20) & 0x3F];
				*cb_info_ptr = info_c;

				info_c.change = _ditherC_lut[(seed >> 25) & 0x3F];
				*cr_info_ptr = info_c;
				
				cb_info_ptr++;
				cr_info_ptr++;
			}
			y_info_ptr++;
		}
	}
}

flash3kyuu_deband::~flash3kyuu_deband()
{
	delete [] _h_ind_masks;
	destroy_frame_luts();
}

static process_plane_impl_t get_process_plane_impl(int sample_mode, bool blur_first)
{
	const process_plane_impl_t* impl_table = process_plane_impl_c;
	if (sample_mode == 0)
	{
		return impl_table[0];
	}
	return impl_table[sample_mode * 2 + (blur_first ? 0 : 1) - 1];
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

	_process_plane_impl = get_process_plane_impl(_sample_mode, _blur_first);
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

	unsigned char threshold;
	pixel_dither_info* info_ptr_base;
	int info_stride;

	switch (plane & 7)
	{
	case PLANAR_Y:
		info_ptr_base = _y_info;
		info_stride = FRAME_LUT_STRIDE(vi.width);
		threshold = _Y;
		break;
	case PLANAR_U:
		info_ptr_base = _cb_info;
		info_stride = FRAME_LUT_STRIDE(vi.width / 2);
		threshold = _Cb;
		break;
	case PLANAR_V:
		info_ptr_base = _cr_info;
		info_stride = FRAME_LUT_STRIDE(vi.width / 2);
		threshold = _Cr;
		break;
	default:
		abort();
	}

	int range = (plane == PLANAR_Y ? _range_raw : _range_raw >> 1);
	_process_plane_impl(srcp, src_width, src_height, src_pitch, dstp, dst_pitch, threshold, info_ptr_base, info_stride, range);
}

PVideoFrame __stdcall flash3kyuu_deband::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi, 16);
	
	if (_diff_seed_for_each_frame)
	{
		init_frame_luts(n);
	}

	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_Y), PLANAR_Y);
	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_U), PLANAR_U);
	process_plane(n, src, dst, dst->GetWritePtr(PLANAR_V), PLANAR_V);
	return dst;
}