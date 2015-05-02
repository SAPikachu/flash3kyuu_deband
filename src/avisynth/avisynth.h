#pragma once

#ifndef AVS_NS
#error AVS_NS is not defined
#endif

namespace AVS_NS {
#if USE_AVISYNTH_INTERFACE == 6
#include "include/interface_v6/avisynth.h"
#elif USE_AVISYNTH_INTERFACE == 3
#include "include/interface_legacy_26/avisynth.h"
#else
#error "USE_AVISYNTH_INTERFACE must be either 3 or 6"
#endif
}
