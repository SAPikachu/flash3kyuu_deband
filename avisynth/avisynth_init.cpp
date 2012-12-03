#include "stdafx.h"
#include "filter.h"
#include "dither_avs.h"
#include "../compiler_compat.h"

F3KDB_API(const char*) AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->AddFunction("flash3kyuu_deband", 
		F3KDB_AVS_PARAMS, 
		Create_flash3kyuu_deband, 
		NULL);
	env->AddFunction("f3kdb", 
		F3KDB_AVS_PARAMS, 
		Create_flash3kyuu_deband, 
		NULL);
	env->AddFunction("f3kdb_dither", 
		"c[mode]i[stacked]b[input_depth]i[keep_tv_range]b", 
		Create_dither, 
		NULL);

	return "f3kdb";
}