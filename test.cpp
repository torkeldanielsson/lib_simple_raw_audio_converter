/*  lsrac tests

    Test wav file downloaded from http://www.kozco.com/tech/soundtests.html
    
*/

#define LSRAC_IMPLEMENTATION
#include "simple_raw_audio_converter.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <stdio.h>

int main()
{
    unsigned int channels;
    unsigned int sample_rate;
    drwav_uint64 total_sample_count;

    float * sample_data = drwav_open_and_read_file_f32("test.wav", &channels, &sample_rate, &total_sample_count);

    if (sample_data == nullptr) {
        printf("Error opening and reading test wav file\n");
        return -1;
    }

    drwav_free(sample_data);

    printf("hello world");

    return 0;
}