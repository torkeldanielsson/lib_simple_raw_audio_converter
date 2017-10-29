/*  lsrac tests

    Test wav file downloaded from http://www.kozco.com/tech/soundtests.html

    Wav header writing code from:
    https://codereview.stackexchange.com/questions/106137/writing-computer-generated-music-to-a-wav-file-in-c-follow-up-2
    https://stackoverflow.com/questions/23030980/creating-a-stereo-wav-file-using-c

*/

#define LSRAC_IMPLEMENTATION
#include "simple_raw_audio_converter.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <stdio.h>

static const float f32_to_s16 = 32767.0f;      // cast<r32>(SHRT_MAX);

static bool is_little_endian() 
{
    volatile uint32_t i = 0x01234567;
    return (*((uint8_t*)(&i))) == 0x67;
}

static uint16_t little_endian_uint16_t(uint16_t num) 
{
    return (((0xff00 & num) >> 8) | ((0xff & num) << 8));
}

static uint32_t little_endian_uint32_t(uint32_t num) {
    return ((((0xff000000 & num) >> 24) | 
             ((0xff & num) << 24)       | 
             ((0xff0000 & num) >> 8))   | 
             ((0xff00 & num) << 8));
}

#pragma pack(0)
struct file_format_header {
    char fChunkID[4];
    uint32_t fChunkSize;
    char fFormat[4];

    char fSubchunk1ID[4];
    uint32_t fSubchunk1Size;
    uint16_t fAudioFormat;
    uint16_t fNumChannels;
    uint32_t fSampleRate;
    uint32_t fByteRate;
    uint16_t fBlockAlign;
    uint16_t fBitsPerSample;

    char fSubchunk2ID[4];
    uint32_t fSubchunk2Size;
};

struct s16_stereo_sample {
    uint16_t left;
    uint16_t right;
};

int32_t write_wav_s16(
    s16_stereo_sample * data, 
    size_t samples, 
    const char * file_name,
    const uint32_t sample_rate = 44100)
{
    if (data == NULL || file_name == NULL) {
        return false;
    }

    FILE * file = fopen(file_name, "wb");
    if (file == NULL) {
        return false;
    }

    static const char fChunkID[]     = {'R', 'I', 'F', 'F'};
    static const char fFormat[]      = {'W', 'A', 'V', 'E'};
    static const char fSubchunk1ID[] = {'f', 'm', 't', ' '};
    static const char fSubchunk2ID[] = {'d', 'a', 't', 'a'};

    static const uint16_t N_CHANNELS                   = 2;
    static const uint32_t fSubchunk1Size               = 16;
    static const uint16_t fAudioFormat                 = 1;
    static const uint16_t fBitsPerSample               = 16;
    static const uint32_t fRIFFChunkDescriptorLength   = 12;
    static const uint32_t fFmtSubChunkDescriptorLength = 24;

    file_format_header wav_header;

    for (size_t i = 0; i < 4; ++i) {
        wav_header.fChunkID[i]     = fChunkID[i];
        wav_header.fFormat[i]      = fFormat[i];
        wav_header.fSubchunk1ID[i] = fSubchunk1ID[i];
        wav_header.fSubchunk2ID[i] = fSubchunk2ID[i];
    }

    static const uint16_t BITS_PER_BYTE = 8;

    const uint32_t fByteRate = sample_rate * N_CHANNELS * fBitsPerSample / 
                               BITS_PER_BYTE;

    const uint16_t fBlockAlign = N_CHANNELS * fBitsPerSample / BITS_PER_BYTE;

    const uint32_t fSubchunk2Size = samples * N_CHANNELS * fBitsPerSample / 
                                    BITS_PER_BYTE;

    const uint32_t fChunkSize = fRIFFChunkDescriptorLength + 
                                fFmtSubChunkDescriptorLength + fSubchunk2Size;

    wav_header.fAudioFormat   = fAudioFormat;
    wav_header.fBitsPerSample = fBitsPerSample;
    wav_header.fBlockAlign    = fBlockAlign;
    wav_header.fByteRate      = fByteRate;
    wav_header.fChunkSize     = fChunkSize;
    wav_header.fNumChannels   = N_CHANNELS;
    wav_header.fSampleRate    = sample_rate;
    wav_header.fSubchunk1Size = fSubchunk1Size;
    wav_header.fSubchunk2Size = fSubchunk2Size;

    bool little_endian = is_little_endian();

    if (!little_endian) {
        wav_header.fAudioFormat   = little_endian_uint16_t(wav_header.fAudioFormat);
        wav_header.fBitsPerSample = little_endian_uint16_t(wav_header.fBitsPerSample);
        wav_header.fBlockAlign    = little_endian_uint16_t(wav_header.fBlockAlign);
        wav_header.fByteRate      = little_endian_uint32_t(wav_header.fByteRate);
        wav_header.fChunkSize     = little_endian_uint32_t(wav_header.fChunkSize);
        wav_header.fNumChannels   = little_endian_uint16_t(wav_header.fNumChannels);
        wav_header.fSampleRate    = little_endian_uint32_t(wav_header.fSampleRate);
        wav_header.fSubchunk1Size = little_endian_uint32_t(wav_header.fSubchunk1Size);
        wav_header.fSubchunk2Size = little_endian_uint32_t(wav_header.fSubchunk2Size);
    }

    size_t ws = fwrite(&wav_header, sizeof(wav_header), 1, file);

    if (ws != 1) {
        fclose(file);
        return false;
    }

    if (!little_endian) {
        for (size_t i = 0; i < samples; ++i) {
            s16_stereo_sample * sample = &data[i];
            sample->left = little_endian_uint16_t(sample->left);
            sample->right = little_endian_uint16_t(sample->right);
        }
    }

    ws = fwrite(data, sizeof(uint16_t), samples * N_CHANNELS, file);
    fclose(file);
    return ws;
}

struct f32_stereo_sample {
    float left;
    float right;
};

int32_t write_wav_f32(
    f32_stereo_sample * data, 
    size_t samples, 
    const char * file_name,
    const uint32_t sample_rate = 44100)
{
    int16_t * converted_data = reinterpret_cast<int16_t *>(malloc(samples * sizeof(s16_stereo_sample)));

    float * data_samples = reinterpret_cast<float *>(data);

    for (size_t i = 0; i < 2*samples; ++i) {
        converted_data[i] = static_cast<int16_t>(data_samples[i] * f32_to_s16);
    }

    int32_t res = write_wav_s16(
        reinterpret_cast<s16_stereo_sample *>(converted_data), 
        samples, 
        file_name, 
        sample_rate);

    free(converted_data);

    return res;
}


int main()
{
    unsigned int channels;
    unsigned int sample_rate;
    drwav_uint64 total_sample_count;

    float * sample_data = drwav_open_and_read_file_f32(
            "test.wav", 
            &channels, 
            &sample_rate, 
            &total_sample_count);
    if (sample_data == NULL) {
        printf("Error opening and reading test wav file\n");
        return -1;
    }
    if (channels != 2) {
        printf("Not 2 channels = error");
         return -1;
   }

    int32_t samples_per_channel = total_sample_count / channels;

    int32_t test_number = 1;

    {
        /*
         *  TEST: no change in sample rate
         */

        float * dst_data = reinterpret_cast<float *>(malloc(total_sample_count * sizeof(float)));

        int32_t conversion_result = -1;
        bool test_ok = true;

        conversion_result = lsrac_convert_audio(
                dst_data,            sample_data,
                samples_per_channel, samples_per_channel,
                2*sizeof(float),     2*sizeof(float),
                0,                   0);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        conversion_result = lsrac_convert_audio(
                dst_data + 1,        sample_data + 1,
                samples_per_channel, samples_per_channel,
                2*sizeof(float),     2*sizeof(float),
                0,                   0);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        for (size_t i = 0; i < total_sample_count; ++i) {
            if (sample_data[i] != dst_data[i]) {
                test_ok = false;
            }
        }

        if (test_ok) {
            printf("Test %d: successful\n", test_number);
        } else {
            printf("Test %d: FAIL (%d)\n", test_number, conversion_result);
        }

        char out_file_name[256];

        snprintf(out_file_name, ARRAY_COUNT(out_file_name), "test_%d.wav", test_number);

        int32_t written = write_wav_f32(  
            reinterpret_cast<f32_stereo_sample *>(dst_data),
            samples_per_channel,
            out_file_name,
            sample_rate);
        if (written < samples_per_channel) {
            printf("Error - written less than expected to test out wav file (%d < %d)\n", written, samples_per_channel);
            return -1;
        }

        test_number++;

        free(dst_data);
    }

    {
        /*
         *  TEST: downsampling
         */

        float resampling_factor = 0.57256f;

        int64_t new_samples_per_channel = static_cast<int64_t>(resampling_factor * static_cast<float>(samples_per_channel));
        int64_t new_sample_rate = static_cast<uint32_t>(resampling_factor * static_cast<float>(sample_rate));

        float * dst_data = reinterpret_cast<float *>(malloc(new_samples_per_channel * channels * sizeof(float)));

        int32_t conversion_result = -1;
        bool test_ok = true;

        conversion_result = lsrac_convert_audio(
                dst_data,                sample_data,
                new_samples_per_channel, samples_per_channel,
                2*sizeof(float),         2*sizeof(float),
                0,                       0);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        conversion_result = lsrac_convert_audio(
                dst_data + 1,            sample_data + 1,
                new_samples_per_channel, samples_per_channel,
                2*sizeof(float),         2*sizeof(float),
                0,                       0);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        if (test_ok) {
            printf("Test %d: successful\n", test_number);
        } else {
            printf("Test %d: FAIL (%d)\n", test_number, conversion_result);
        }

        char out_file_name[256];

        snprintf(out_file_name, ARRAY_COUNT(out_file_name), "test_%d.wav", test_number);

        int32_t written = write_wav_f32(  
            reinterpret_cast<f32_stereo_sample *>(dst_data),
            new_samples_per_channel,
            out_file_name,
            new_sample_rate);
        if (written < samples_per_channel) {
            printf("Error - written less than expected to test out wav file (%d < %d)\n", written, samples_per_channel);
            return -1;
        }

        test_number++;

        free(dst_data);
    }

    {
        /*
         *  TEST: upsampling
         */

        float resampling_factor = 2.0f;

        int64_t new_samples_per_channel = static_cast<int64_t>(resampling_factor * static_cast<float>(samples_per_channel));
        int64_t new_sample_rate = static_cast<uint32_t>(resampling_factor * static_cast<float>(sample_rate));

        float * dst_data = reinterpret_cast<float *>(malloc(new_samples_per_channel * channels * sizeof(float)));

        int32_t conversion_result = -1;
        bool test_ok = true;

        conversion_result = lsrac_convert_audio(
                dst_data,                sample_data,
                new_samples_per_channel, samples_per_channel,
                2*sizeof(float),         2*sizeof(float),
                0,                       0);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        conversion_result = lsrac_convert_audio(
                dst_data + 1,            sample_data + 1,
                new_samples_per_channel, samples_per_channel,
                2*sizeof(float),         2*sizeof(float),
                0,                       0);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        if (test_ok) {
            printf("Test %d: successful\n", test_number);
        } else {
            printf("Test %d: FAIL (%d)\n", test_number, conversion_result);
        }

        char out_file_name[256];

        snprintf(out_file_name, ARRAY_COUNT(out_file_name), "test_%d.wav", test_number);

        int32_t written = write_wav_f32(  
            reinterpret_cast<f32_stereo_sample *>(dst_data),
            new_samples_per_channel,
            out_file_name,
            new_sample_rate);
        if (written < samples_per_channel) {
            printf("Error - written less than expected to test out wav file (%d < %d)\n", written, samples_per_channel);
            return -1;
        }

        test_number++;

        free(dst_data);
    }

    {
        /*
         *  TEST: small sample test
         */

        float src_data[]          = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, };
        float dst_data[]          = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, };
        float expected_dst_data[] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, };

        int32_t conversion_result = -1;
        bool test_ok = true;

        if (ARRAY_COUNT(dst_data) != ARRAY_COUNT(expected_dst_data)) {

            printf("ERROR IN TEST %d, aborting tests \n", test_number);
            return -1;
        }

        int32_t edge_avoid = 4;

        conversion_result = lsrac_convert_audio(
                dst_data,                src_data + edge_avoid,
                ARRAY_COUNT(dst_data),   ARRAY_COUNT(src_data) - 2*edge_avoid,
                sizeof(float),           sizeof(float),
                edge_avoid,              edge_avoid);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        printf("src:");
        for (size_t i = 0; i < ARRAY_COUNT(src_data); ++i) {
            printf(" %f", src_data[i]);
        }
        printf("\n");

        printf("dst:");
        for (size_t i = 0; i < ARRAY_COUNT(dst_data); ++i) {
            printf(" %f", dst_data[i]);
        }
        printf("\n");

        printf("expected_dst:");
        for (size_t i = 0; i < ARRAY_COUNT(expected_dst_data); ++i) {
            printf(" %f", expected_dst_data[i]);
        }
        printf("\n");

        for (size_t i = 0; i < ARRAY_COUNT(dst_data); ++i) {
            if (abs(dst_data[i] - expected_dst_data[i]) > 2) {
                test_ok = false;
            }
        }

        if (test_ok) {
            printf("Test %d: successful\n", test_number);
        } else {
            printf("Test %d: FAIL (%d)\n", test_number, conversion_result);
        }

        test_number++;
    }

    {
        /*
         *  TEST: small sample test
         */

        float src_data[]          = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, };
        float dst_data[]          = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, };
        float expected_dst_data[] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, };

        int32_t conversion_result = -1;
        bool test_ok = true;

        if (ARRAY_COUNT(dst_data) != ARRAY_COUNT(expected_dst_data)) {

            printf("ERROR IN TEST %d, aborting tests \n", test_number);
            return -1;
        }

        int32_t edge_avoid = 4;

        conversion_result = lsrac_convert_audio(
                dst_data,                src_data + edge_avoid,
                ARRAY_COUNT(dst_data),   ARRAY_COUNT(src_data) - 2*edge_avoid,
                sizeof(float),           sizeof(float),
                edge_avoid,              edge_avoid);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        printf("src:");
        for (size_t i = 0; i < ARRAY_COUNT(src_data); ++i) {
            printf(" %f", src_data[i]);
        }
        printf("\n");

        printf("dst:");
        for (size_t i = 0; i < ARRAY_COUNT(dst_data); ++i) {
            printf(" %f", dst_data[i]);
        }
        printf("\n");

        printf("expected_dst:");
        for (size_t i = 0; i < ARRAY_COUNT(expected_dst_data); ++i) {
            printf(" %f", expected_dst_data[i]);
        }
        printf("\n");

        for (size_t i = 0; i < ARRAY_COUNT(dst_data); ++i) {
            if (abs(dst_data[i] - expected_dst_data[i]) > 2) {
                test_ok = false;
            }
        }

        if (test_ok) {
            printf("Test %d: successful\n", test_number);
        } else {
            printf("Test %d: FAIL (%d)\n", test_number, conversion_result);
        }

        test_number++;
    }

    drwav_free(sample_data);

    return 0;
}
