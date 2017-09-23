/*  lib_simple_raw_audio_converter (lsrac) - alpha - https://github.com/torkeldanielsson/lib_simple_raw_audio_converter

    Single file header only library for converting raw audio formats and sample rates. 
    Uses code from libsamplerate/"Simple Rabbit Code".


USAGE
  
    Before #including,
        #define LSRAC_IMPLEMENTATION
    in the file that you want to have the implementation - just like STB.

    dst_data must be allocated by user and large enough.

    dst_samples and src_samples are assumed to be covering the whole segment
    (so sample rates will be calculated based off them)


LICENSE

    LGPL.

    (libsamplerate/"Simple Rabbit Code" is also LGPL)

*/

#ifndef INCLUDE_SIMPLE_RAW_AUDIO_CONVERTER_H
#define INCLUDE_SIMPLE_RAW_AUDIO_CONVERTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LSRAC_TYPE_INT_16   1
// #define LSRAC_TYPE_INT_32   2
#define LSRAC_TYPE_FLOAT_32 3
// #define LSRAC_TYPE_FLOAT_64 4

extern int32_t lsrac_convert_audio(
        void *  dst_data,     void *  src_data,
        int32_t dst_type,     int32_t src_type,
        int64_t dst_samples,  int64_t src_samples,
        int32_t dst_stride,   int32_t src_stride,                /* default = 1 */
                              int32_t src_extra_samples_before,  /* default = 0 */
                              int32_t src_extra_samples_after);  /* default = 0 */

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_SIMPLE_RAW_AUDIO_CONVERTER_H

#ifdef LSRAC_IMPLEMENTATION

#define LSRAC_RET_VAL_ARGUMENT_ERROR  -2
#define LSRAC_RET_VAL_ERROR           -1
#define LSRAC_RET_VAL_OK               1

#include <cstddef>

extern int32_t convert_audio(
        void *  dst_data,       void *  src_data,
        int32_t dst_type,       int32_t src_type,
        int64_t dst_samples,    int64_t src_samples,
        int32_t dst_stride = 1, int32_t src_stride = 1,
                                int32_t src_extra_samples_before = 0, 
                                int32_t src_extra_samples_after = 0)
{
    if (dst_data == nullptr ||
        src_data == nullptr) {
        return LSRAC_RET_VAL_ARGUMENT_ERROR;
    }

    double input_index = 0;
    double src_ratio = ((double)src_samples) / ((double)dst_samples);

    return LSRAC_RET_VAL_OK;
}

#endif // LSRAC_IMPLEMENTATION
