#pragma once

#include "process_plane_context.h"
#include "include/f3kdb.h"

#ifdef FLASH3KYUU_DEBAND_EXPORTS
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllexport)
#else
#define FLASH3KYUU_DEBAND_API extern "C" __declspec(dllimport)
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define LIKELY(x)       __builtin_expect((x),1)
#define UNLIKELY(x)     __builtin_expect((x),0)
#define EXPECT(x, val)  __builtin_expect((x),val)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#define EXPECT(x, val)  (x)
#endif

#ifdef __INTEL_COMPILER
#define __PRAGMA_NOUNROLL__ __pragma(nounroll)
#else
#define __PRAGMA_NOUNROLL__
#endif

