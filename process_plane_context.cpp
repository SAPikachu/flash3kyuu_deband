#include "stdafx.h"

#include "process_plane_context.h"

#include <assert.h>


void destroy_context(process_plane_context* context)
{
	assert(context);

	if (context->data) {
		assert(context->destroy);
		context->destroy(context->data);
		memset(context, 0, sizeof(process_plane_context));
	}
}

void init_context(process_plane_context* context)
{
	assert(context);
	memset(context, 0, sizeof(process_plane_context));
}
