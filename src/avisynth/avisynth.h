#pragma once

#if USE_AVISYNTH_INTERFACE == 5

#ifdef __INTEL_COMPILER
#pragma warning( push )
#pragma warning( disable: 693 )
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4521 )
#pragma warning( disable: 4522 )
#endif

#include "include/interface_v5/avisynth.h"

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#pragma warning( pop )
#endif

#elif USE_AVISYNTH_INTERFACE == 3
#include "include/interface_legacy_26/avisynth.h"
#else
#error "USE_AVISYNTH_INTERFACE must be either 3 or 5"
#endif

