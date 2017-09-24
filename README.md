# lib_simple_raw_audio_converter / lsrac
Single file, header only, library for converting raw audio formats and sample rates.

It uses a sinc filter to make what is hopefully an ok sample rate conversion without distortions. (The filter coefficients are the same as in libsamplerate.)

# Use

Before #including, #define LSRAC_IMPLEMENTATION in the file that you want to have the implementation - just like STB.

lsrac_convert_audio(..) is the only function, it will convert one stream of samples from one sample rate to another.

*Note:*

- dst_data must be allocated by user and large enough.
- dst_samples and src_samples are assumed to be covering the whole segment and sample rates will be calculated based off them. This poses the limitation that you must chose proper start and end times for the conversions.
-  Converstions are only for one channel at a time. Use the stride parameters to support interleaved formats.

See test.cpp for a working example of how to use lsrac.
