Basic usage
-----------

.. highlight:: python

* Avisynth::

    # Note: Only planar YUV colorspaces are supported

    # Load plugins, source video etc...
    f3kdb(...)

* Vapoursynth::

    import vapoursynth as vs

    core = vs.get_core()

    # Load plugins, source video etc...

    last = core.f3kdb.Deband(last, ...)

Parameters
----------

.. option:: range

    Banding detection range.

    **Default:** 15

.. option:: Y, Cb, Cr

    Banding detection threshold for respective plane. If difference between current pixel and
    reference pixel is less than threshold, it will be considered as banded.

    **Default:** 64

.. option:: grainY, grainC

    Specifies amount of grains added in the last debanding stage.

    **Default:** 64

.. option:: sample_mode

    Valid modes are:

    * 1: Take 2 pixels as reference pixel. Reference pixels are in the same
      column of current pixel. Add grain after processing.

    * 2: Take 4 pixels as reference pixel. Reference pixels are in the square
      around current pixel. Add grain after processing.

    **Default:** 2

.. option:: seed

    Seed for random number generation.

.. option:: blur_first

    * true: Current pixel is compared with average value of all pixels.
    * false: Current pixel is compared with all pixels. The pixel is considered as
      banded pixel only if all differences are less than threshold.

    **Default:** true

.. option:: dynamic_grain

    Use different grain pattern for each frame.

    **Default:** false

.. option:: opt

    Specifies optimization mode.

    * -1: Use highest optimization mode that is supported by host CPU
    
    * 0: No optimization (Intended for testing only)
    
    * 1: SSE2 (Pentium 4, AMD K8)
    
    * 2: SSSE3 (Core 2)
    
    * 3: SSE4.1 (Core 2 45nm)

    **Default:** -1

.. option:: mt

    *Only available in Avisynth*

    Multi-threaded processing. If set to true, U and V plane will be proccessed
    in parallel with Y plane to speed up processing.

    **Default:** true if host has more than 1 CPU/cores, false otherwise.

.. option:: dither_algo

    * 1: No dithering, LSB is truncated
    
    * 2: Ordered dithering
    
    * 3: Floyd-Steinberg dithering

    Notes:

    1. Visual quality of mode 3 is the best, but the debanded pixels may
       easily be destroyed by x264, you need to carefully tweak the settings
       to get better result.

    2. Mode 1 and mode 2 don't look the best, but if you are encoding at low
       bitrate, they may be better choice since the debanded pixels is easier
       to survive encoding, mode 3 may look worse than 1/2 after encoding in
       this situation.
    
       (Thanks sneaker_ger @ doom9 for pointing this out!)

    3. This parameter is ignored if :option:`output_depth` = 16.

    4.
        10bit x264 command-line example::
    
            avs2yuv -raw "script.avs" -o - | x264-10bit --demuxer raw --input-depth 16 --input-res 1280x720 --fps 24 --output "out.mp4" -

        Or compile x264 with the patch on https://gist.github.com/1117711, and
        specify the script directly::

            x264-10bit --input-depth 16 --output "out.mp4" script.avs

    **Default:** 3

.. option:: keep_tv_range

    If set to true, all processed pixels will be clamped to TV range
    (luma: 16 ~ 235, chroma: 16 ~ 240).

    * It is recommended to set this to true for TV-range videos, since pixel
      values may overflow/underflow after dithering.

    * DON'T set this to true for full-range videos, as all out-of-range pixels
      will be clamped to TV range.

    **Default:** false

.. option:: input_mode

    *Not available in Vapoursynth, since this can be inferred from clip properties*

    Specify source video type.

    * 0: Regular 8 bit video
    
    * 1: 9 ~ 16 bit high bit-depth video, stacked format
    
    * 2: 9 ~ 16 bit high bit-depth video, interleaved format

    **Default:** 0 (:option:`input_depth` = 8 or not specified) / 1 (:option:`input_depth` > 8)

.. option:: input_depth

    *Not available in Vapoursynth, since this can be inferred from clip properties*

    Specify bit-depth of source video.

    **Range:** 8 ~ 16

    **Default:** 8 (:option:`input_mode` = 0 or not specified) / 16 (:option:`input_mode` = 1 or 2)

.. option:: output_mode

    *Not available in Vapoursynth, f3kdb will only output high bitdepth clip in native format*

    Specify output video type. Meaning of values are the same as :option:`input_mode`.

    When :option:`output_mode` = 2, frames will be 2x wider and look garbled on preview,
    it will return to normal after correctly encoded by high bit-depth x264)

    **Default:** 0 (:option:`output_depth` = 8 or not specified) / 1 (:option:`output_depth` > 8)

.. option:: output_depth

    Specify output bit-depth.

    If :option:`output_depth` = 16, dither algorithm specified by :option:`dither_algo` won't be
    applied.

    **Range:** 8 ~ 16

    **Default:** 8 (:option:`output_mode` = 0 or not specified) / 16 (:option:`output_mode` = 1 or 2)

.. option:: random_algo_ref, random_algo_grain

    Choose random number algorithm for reference positions / grains.

    * 0: Algorithm in old versions
    
    * 1: Uniform distribution
    
    * 2: Gaussian distribution

      (StdDev (sigma) is settable through :option:`random_param_ref` / :option:`random_param_grain`,
      Only values in [-1.0, 1.0] is used for multiplication, numbers outside
      this range are simply ignored)

    **Default:** 1 / 1

.. option:: random_param_ref, random_param_grain

    Parameter for the respective random number generator. Currently only have
    effect for the Gaussian generator.

    **Default:** 1.0

.. option:: preset

    Use preset parameters. Preset will be applied before other parameters so that you can easily override individual parameter.

    :doc:`See available presets <presets>`
