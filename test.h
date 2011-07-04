
#include "flash3kyuu_deband.h"

#include "impl_dispatch.h"

#include "intrin.h"

#include "asmlib.h"

#define GUARD_CONST 0xDEADBEEF

#define READ_TSC(ret_var) do { ret_var = ReadTSC(); } while(0)

static unsigned char * create_guarded_buffer(int const height, int const pitch, unsigned char * &data_start)
{
	unsigned char* buffer = (unsigned char*)_aligned_malloc( (height + 4) * pitch, PLANE_ALIGNMENT);

	for (int i = 0; i < pitch * 2; i += 4)
	{
		*(int*)(buffer + i) = GUARD_CONST;
	}
	for (int i = 0; i < pitch * 2; i += 4)
	{
		*(int*)(buffer + (height + 2) * pitch + i) = GUARD_CONST;
	}
	data_start = buffer + pitch * 2;
	return buffer;
}

static void check_guard_bytes(unsigned char *buffer, int const height, int const pitch)
{
	for (int i = 0; i < pitch * 2; i += 4)
	{
		if (*(int*)(buffer + i) != GUARD_CONST) {
			printf("ERROR: Leading guard bytes was overwritten.\n");
			break;
		}
	}
	for (int i = 0; i < pitch * 2; i += 4)
	{
		if (*(int*)(buffer + (height + 2) * pitch + i) != GUARD_CONST) {
			printf("ERROR: Trailing guard bytes was overwritten.\n");
			break;
		}
	}
}
