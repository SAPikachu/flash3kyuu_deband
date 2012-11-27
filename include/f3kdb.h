#pragma once

#include "f3kdb_enums.h"
#include "f3kdb_params.h"

static const int F3KDB_INTERFACE_VERSION = 1;

// All planes need to be aligned to 16-byte boundary
static const int PLANE_ALIGNMENT = 16;

enum {
    // Supposed to be the same as respecting values in avisynth
    PLANE_Y = 1<<0,
    PLANE_CB = 1<<1,
    PLANE_CR = 1<<2
};
typedef struct _f3kdb_video_info_t
{
    // width and height are in actual pixels, NOT pixels after applying hacks
    int width;
    int height;

    int chroma_width_subsampling;
    int chroma_height_subsampling;

    // Can be set to an estimated number if unknown when initializing
    int num_frames;

    inline int get_plane_width(int plane)
    {
        return plane == PLANE_Y ? width : width >> chroma_width_subsampling;
    }

    inline int get_plane_height(int plane)
    {
        return plane == PLANE_Y ? height : height >> chroma_height_subsampling;
    }
} f3kdb_video_info_t;

class f3kdb_core_t;

enum
{
    F3KDB_SUCCESS = 0,
    F3KDB_ERROR_INVALID_INTERFACE_VERSION,
    F3KDB_INSUFFICIENT_MEMORY,
    F3KDB_ERROR_INVALID_PARAMETER,
    F3KDB_ERROR_INVALID_STATE,

    F3KDB_ERROR_MAX
};

extern "C"
{

int f3kdb_params_init_defaults(f3kdb_params_t* params, int interface_version = F3KDB_INTERFACE_VERSION);
int f3kdb_params_sanitize(f3kdb_params_t* params, int interface_version = F3KDB_INTERFACE_VERSION);
int f3kdb_create(const f3kdb_video_info_t* video_info, const f3kdb_params_t* params, f3kdb_core_t** core_out, char* error_msg = nullptr, size_t error_msg_size = 0, int interface_version = F3KDB_INTERFACE_VERSION);
int f3kdb_destroy(f3kdb_core_t* context);
int f3kdb_process_plane(f3kdb_core_t* core, int frame_index, int plane, unsigned char* dst_frame_ptr, int dst_pitch, const unsigned char* src_frame_ptr, int src_pitch);

}