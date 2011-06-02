

namespace pixel_proc_16bit_f_s_dithering {
	
	static const int BIT_DEPTH = 16;

	typedef unsigned short ERROR_TYPE;

	typedef struct _context_t
	{
		ERROR_TYPE* error_buffer;
		bool buffer_needs_dealloc;
		ERROR_TYPE* current_px_error;
		int row_pitch;
	} context_t;

	static inline void init_context(char context_buffer[CONTEXT_BUFFER_SIZE], int frame_width)
	{
		context_t* ctx = (context_t*)context_buffer;
		int ctx_size = sizeof(context_t);
		memset(ctx, 0, ctx_size);

		// additional 2 bytes are placed at the beginning and the end
		int size_needed = (frame_width + 2) * 2 * sizeof(ERROR_TYPE);
		if (CONTEXT_BUFFER_SIZE - ctx_size < size_needed)
		{
			ctx->error_buffer = (ERROR_TYPE*)malloc(size_needed);
			ctx->buffer_needs_dealloc = true;
		} else {
			ctx->error_buffer = (ERROR_TYPE*)(context_buffer + ctx_size);
		}
		memset(ctx->error_buffer, 0, size_needed);
		ctx->current_px_error = ctx->error_buffer + 1;
		ctx->row_pitch = frame_width + 2;
	}

	static inline void destroy_context(void* context)
	{
		context_t* ctx = (context_t*)context;
		if (ctx->buffer_needs_dealloc)
		{
			free(ctx->error_buffer);
			ctx->error_buffer = NULL;
		}
	}

	static inline void next_pixel(void* context)
	{
		context_t* ctx = (context_t*)context;
		ctx->current_px_error++;
	}

	static inline void next_row(void* context)
	{
		context_t* ctx = (context_t*)context;
		ctx->row_pitch = -ctx->row_pitch;
		ctx->current_px_error = ctx->error_buffer + (ctx->row_pitch >> 31) * ctx->row_pitch;
		memset(ctx->current_px_error + ctx->row_pitch, 0, abs(ctx->row_pitch) * sizeof(ERROR_TYPE));
		ctx->current_px_error++;
	}

	static inline int dither(void* context, int pixel, int row, int column)
	{
		context_t* ctx = (context_t*)context;
		pixel += *(ctx->current_px_error);
		int new_error = pixel & ( ( 1 << (BIT_DEPTH - 8) ) - 1 );
		*(ctx->current_px_error + 1) += (new_error * 7) >> 4;
		*(ctx->current_px_error + ctx->row_pitch - 1) += (new_error * 3) >> 4;
		*(ctx->current_px_error + ctx->row_pitch) += (new_error * 5) >> 4;
		*(ctx->current_px_error + ctx->row_pitch + 1) += (new_error * 1) >> 4;
		return pixel;
	}

	#include "pixel_proc_c_high_bit_depth_common.h"

};