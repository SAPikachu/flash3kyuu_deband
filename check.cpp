#include "check.h"
#include <stdio.h>

void check_parameter_range(const char* name, int value, int lower_bound, int upper_bound, char* param_name, IScriptEnvironment* env) {
    if (value < lower_bound || value > upper_bound) {
        char msg[1024];
        sprintf_s(msg, "%s: Invalid value for parameter %s: %d, must be %d ~ %d.",
            name, param_name, value, lower_bound, upper_bound);
        env->ThrowError(_strdup(msg));
    }
}

void check_video_format(const char* name, const VideoInfo& vi, IScriptEnvironment* env)
{
    char name_buffer[256];
    if (!vi.IsYUV() || !(vi.IsYUY2() || vi.IsPlanar())) {
        sprintf(name_buffer, "%s: Only YUY2 and planar YUV clips are supported.", name);
        env->ThrowError(strdup(name_buffer));
    }
    if (vi.IsFieldBased()) {
        sprintf(name_buffer, "%s: Field-based clip is not supported.", name);
        env->ThrowError(strdup(name_buffer));
    }
}