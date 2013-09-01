#include "stdafx.h"
#include "filter.h"
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

	return "f3kdb";
}