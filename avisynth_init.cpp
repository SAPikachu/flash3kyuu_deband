#include "stdafx.h"

#include "flash3kyuu_deband.h"

#include "dither_avs.h"


FLASH3KYUU_DEBAND_API const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->AddFunction("flash3kyuu_deband", 
		"c[range]i[Y]i[Cb]i[Cr]i[ditherY]i[ditherC]i[sample_mode]i[seed]i[blur_first]b[diff_seed]b[opt]i[mt]b[precision_mode]i[keep_tv_range]b", 
		Create_flash3kyuu_deband, 
		NULL);
	env->AddFunction("f3kdb_dither", 
		"c[mode]i[stacked]b[input_depth]i[keep_tv_range]b", 
		Create_dither, 
		NULL);

	return "flash3kyuu_deband";
}