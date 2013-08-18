#pragma once

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

#ifndef _WIN32
#include <cstring>
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define alignas(x) __declspec(align(x))
#endif