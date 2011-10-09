
#include <Windows.h>

#include <stdio.h>

#include <emmintrin.h>

void dump_init(const TCHAR* dump_base_name, int plane);

void dump_value(const TCHAR* dump_name, int value);

void dump_value(const TCHAR* dump_name, __m128i value, int word_size_in_bytes, bool is_signed);

void dump_finish();

#ifdef ENABLE_DEBUG_DUMP

#define DUMP_INIT(name, plane) dump_init( TEXT(name), plane )

#define DUMP_VALUE(name, ...) dump_value(TEXT(name), __VA_ARGS__)

#define DUMP_VALUE_S(name, ...) dump_value(name, __VA_ARGS__)

#define DUMP_FINISH() dump_finish()

#else

#define DUMP_INIT(name, plane) ((void)0)

#define DUMP_VALUE(name, ...) ((void)0)

#define DUMP_VALUE_S(name, ...) ((void)0)

#define DUMP_FINISH() ((void)0)

#endif