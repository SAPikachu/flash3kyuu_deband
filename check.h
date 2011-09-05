#pragma once

#include "avisynth.h"

void check_parameter_range(const char* name, int value, int lower_bound, int upper_bound, char* param_name, IScriptEnvironment* env);

void check_video_format(const char* name, const VideoInfo& vi, IScriptEnvironment* env);