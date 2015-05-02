#pragma once

#ifndef AVS_NS
#error AVS_NS is not defined
#endif

namespace AVS_NS {
#if USE_AVISYNTH_INTERFACE == 6
#if defined(__INTEL_COMPILER)
#pragma warning(push)
// #693: calling convention specified here is ignored
#pragma warning(disable: 693)
#endif

#include "include/interface_v6/avisynth.h"

#if defined(__INTEL_COMPILER)
#pragma warning(pop)
#endif
#elif USE_AVISYNTH_INTERFACE == 3
#include "include/interface_legacy_26/avisynth.h"
#else
#error "USE_AVISYNTH_INTERFACE must be either 3 or 6"
#endif
}
