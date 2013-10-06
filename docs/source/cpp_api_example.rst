C++ API
-------

.. highlight:: c

*Note: Due to the use of some C++ feature in header files, they can't be compiled
under pure C without hacking. This will be change in future.*

Please check the following example to see how to use f3kdb in your program:: 

    // Compile with: g++ -std=c++11 -Wall -Wextra example.cpp -lf3kdb

    #include <stdio.h>
    #include <stdint.h>
    #include <f3kdb/f3kdb.h>

    int main()
    {
        static const int FRAME_WIDTH = 640;
        static const int FRAME_HEIGHT = 480;

        f3kdb_video_info_t vi;
        vi.width = FRAME_WIDTH;
        vi.height = FRAME_HEIGHT;

        // YUV 4:2:0
        vi.chroma_width_subsampling = 1;
        vi.chroma_height_subsampling = 1;

        // 8-bit video
        vi.pixel_mode = LOW_BIT_DEPTH;
        vi.depth = 8;

        // Set this to estimated value if frame count is unknown
        vi.num_frames = 42;

        f3kdb_params_t params;
        int result;
        result = f3kdb_params_init_defaults(&params); // Always call this to initialize parameters to correct default value
        if (result != F3KDB_SUCCESS) {
            printf("f3kdb_params_init_defaults code = %d\n", result);
            return result;
        }

        // Fill parameters. You can also directly modify params
        result = f3kdb_params_fill_preset(&params, "medium/nograin");
        if (result != F3KDB_SUCCESS) {
            printf("f3kdb_params_fill_preset code = %d\n", result);
            return result;
        }
        result = f3kdb_params_fill_by_string(&params, "seed=42/dither_algo=2");
        if (result != F3KDB_SUCCESS) {
            printf("f3kdb_params_fill_by_string code = %d\n", result);
            return result;
        }

        // No need to call f3kdb_params_sanitize or f3kdb_video_info_sanitize,
        // unless you want to inspect how the parameter object look like after
        // replacing default values with inferred values

        f3kdb_core_t* core;
        char extra_error[1024];
        result = f3kdb_create(&vi, &params, &core, extra_error, sizeof(extra_error));
        if (result != F3KDB_SUCCESS) {
            printf("f3kdb_create code = %d, msg=%s\n", result, extra_error);
            return result;
        }

        // For demonstration purpose, we just try to deband some garbage data
        static const int buffer_size = FRAME_WIDTH * 480 + 15; // Keep room for 16-byte alignment
        unsigned char src_buffer[buffer_size]; 
        unsigned char dst_buffer[buffer_size]; 

        // Align the buffers
        // Note: Source buffer can be unaligned, but performance may be degraded for unaligned buffer
        // Destination buffer MUST be aligned, otherwise bad things may happen
        // Use posix_memalign / _aligned_malloc to allocate aligned buffer in real world
        unsigned char* src_buffer_ptr = (unsigned char*)(((uintptr_t)src_buffer + 15) & ~15);
        unsigned char* dst_buffer_ptr = (unsigned char*)(((uintptr_t)dst_buffer + 15) & ~15);

        // Y plane
        result = f3kdb_process_plane(core, 0, PLANE_Y, dst_buffer_ptr, FRAME_WIDTH, src_buffer_ptr, FRAME_WIDTH);
        if (result != F3KDB_SUCCESS) {
            printf("f3kdb_process_plane code = %d\n", result);
            return result;
        }
        // Cb plane
        result = f3kdb_process_plane(core, 0, PLANE_CB, dst_buffer_ptr, FRAME_WIDTH, src_buffer_ptr, FRAME_WIDTH);
        if (result != F3KDB_SUCCESS) {
            printf("f3kdb_process_plane code = %d\n", result);
            return result;
        }

        // Clean up
        f3kdb_destroy(core);
        return 0;
    }