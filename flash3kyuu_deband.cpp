// flash3kyuu_deband.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <intrin.h>
#include <process.h>
#include <windows.h>

#include "flash3kyuu_deband.h"
#include "impl_dispatch.h"
#include "icc_override.h"

static void check_parameter_range(int value, int lower_bound, int upper_bound, char* param_name, IScriptEnvironment* env) {
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

	SYSTEM_INFO si;
	memset(&si, 0, sizeof(si));
	GetSystemInfo(&si);

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
	int opt = args[11].AsInt(-1);
	bool mt = args[12].AsBool(si.dwNumberOfProcessors > 1);

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
	CHECK_PARAM(opt, -1, (sizeof(process_plane_impls) - 1) );

	return new flash3kyuu_deband(child, range, Y, Cb, Cr, 
		ditherY, ditherC, sample_mode, seed, 
		blur_first, diff_seed_for_each_frame, opt, mt);
}

flash3kyuu_deband::flash3kyuu_deband(PClip child, int range, unsigned char Y, unsigned char Cb, unsigned char Cr, 
		int ditherY, int ditherC, int sample_mode, int seed,
		bool blur_first, bool diff_seed_for_each_frame, int opt, bool mt) :
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

void inline rand_next(int &seed)
{
	int seed_tmp = (((seed << 13) ^ (unsigned int)seed) >> 17) ^ (seed << 13) ^ seed;
	seed = 32 * seed_tmp ^ seed_tmp;
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
	
	int range_limit = _range * 2 + 1;
	int ditherY_limit = _ditherY * 2 + 1;
	int ditherC_limit = _ditherC * 2 + 1;

	for (int y = 0; y < vi.height; y++)
	{
		y_info_ptr = _y_info + y * y_stride;
		cb_info_ptr = _cb_info + (y / 2) * c_stride;
		cr_info_ptr = _cr_info + (y / 2) * c_stride;
		for (int x = 0; x < vi.width; x++)
		{
			rand_next(seed);

			pixel_dither_info info_y = {0, 0, 0, 0};
			info_y.change = (signed char)((seed & 0x7fffffff) % ditherY_limit - _ditherY);

			if (!(x < _range || vi.width - x <= _range || y < _range || vi.height - y <= _range)) {
				rand_next(seed);
				info_y.ref1 = (signed char)((seed & 0x7fffffff) % range_limit - _range);
				if (_sample_mode = 2)
				{
					rand_next(seed);
					info_y.ref2 = (signed char)((seed & 0x7fffffff) % range_limit - _range);
				}
			}

			*y_info_ptr = info_y;

			if ((x & 1) == 0 && (y & 1) == 0) {
				pixel_dither_info info_c = {0, 0, 0, 0};
				// use absolute value to calulate correct ref, and restore sign
				info_c.ref1 = (abs(info_y.ref1) >> 1) * (info_y.ref1 >> 7);
				info_c.ref2 = (abs(info_y.ref2) >> 1) * (info_y.ref2 >> 7);
				
				rand_next(seed);
				info_c.change = (signed char)((seed & 0x7fffffff) % ditherC_limit - _ditherC);
				*cb_info_ptr = info_c;
				
				rand_next(seed);
				info_c.change = (signed char)((seed & 0x7fffffff) % ditherC_limit - _ditherC);
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
	if (_mt_info) {
		_mt_info->exit = true;
		SetEvent(_mt_info->work_event);
		WaitForSingleObject(_mt_info->thread_handle, INFINITE);
		mt_info_destroy(_mt_info);
	}

	destroy_frame_luts();
}

static process_plane_impl_t get_process_plane_impl(int sample_mode, bool blur_first, int opt)
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
	const process_plane_impl_t* impl_table = process_plane_impls[opt];
	return impl_table[select_impl_index(sample_mode, blur_first)];
}

void flash3kyuu_deband::init(void) 
{
	___intel_cpu_indicator_init();

	int yCbCr_coeff;
	if (_sample_mode > 0 && _blur_first) {
		yCbCr_coeff = 2;
	} else {
		yCbCr_coeff = 4;
	}
	
	_Y = _Y * yCbCr_coeff;
	_Cb = _Cb * yCbCr_coeff;
	_Cr = _Cr * yCbCr_coeff;

	init_context(&_y_context);
	init_context(&_cb_context);
	init_context(&_cr_context);

	init_frame_luts(0);

	_process_plane_impl = get_process_plane_impl(_sample_mode, _blur_first, _opt);

}

void flash3kyuu_deband::process_plane(PVideoFrame src, PVideoFrame dst, unsigned char *dstp, int plane, IScriptEnvironment* env)
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
	process_plane_context* context;

	switch (plane & 7)
	{
	case PLANAR_Y:
		info_ptr_base = _y_info;
		info_stride = FRAME_LUT_STRIDE(vi.width);
		threshold = _Y;
		context = &_y_context;
		break;
	case PLANAR_U:
		info_ptr_base = _cb_info;
		info_stride = FRAME_LUT_STRIDE(vi.width / 2);
		threshold = _Cb;
		context = &_cb_context;
		break;
	case PLANAR_V:
		info_ptr_base = _cr_info;
		info_stride = FRAME_LUT_STRIDE(vi.width / 2);
		threshold = _Cr;
		context = &_cr_context;
		break;
	default:
		abort();
	}

	if (threshold == 0) {
		// no need to process
        env->BitBlt(dstp, dst_pitch, srcp, src_pitch, src_width, src_height);
		return;
	}

	int range = (plane == PLANAR_Y ? _range : _range >> 1);
	_process_plane_impl(srcp, src_width, src_height, src_pitch, dstp, dst_pitch, threshold, info_ptr_base, info_stride, range, context);
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
	PVideoFrame dst = env->NewVideoFrame(vi, PLANE_ALIGNMENT);

	if (_diff_seed_for_each_frame)
	{
		init_frame_luts(n);
	}

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
	return dst;
}