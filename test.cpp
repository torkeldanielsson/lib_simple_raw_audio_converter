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

int32_t write_wav(
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

int main()
{
    unsigned int channels;
    unsigned int sample_rate;
    drwav_uint64 total_sample_count;

    int16_t * sample_data = drwav_open_and_read_file_s16(
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

    {
        /*
         *  TEST 1: no change in sample rate
         */

        int16_t * dst_data = reinterpret_cast<int16_t *>(malloc(total_sample_count * sizeof(int16_t)));

        int32_t conversion_result = -1;
        bool test_ok = true;

        conversion_result = lsrac_convert_audio(
                dst_data,            sample_data,
                samples_per_channel, samples_per_channel,
                2,                   2);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        conversion_result = lsrac_convert_audio(
                dst_data + 1,            sample_data + 1,
                samples_per_channel,     samples_per_channel,
                2,                       2);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        for (size_t i = 0; i < total_sample_count; ++i) {
            if (sample_data[i] != dst_data[i]) {
                test_ok = false;
            }
        }

        if (test_ok) {
            printf("Test 1: successful\n");
        } else {
            printf("Test 1: FAIL\n");
        }

        int32_t written = write_wav(  
            reinterpret_cast<s16_stereo_sample *>(dst_data),
            samples_per_channel,
            "test_1.wav",
            sample_rate);
        if (written < samples_per_channel) {
            printf("Error - written less than expected to test out wav file (%d < %d)\n", written, samples_per_channel);
            return -1;
        }

        free(dst_data);
    }

    {
        /*
         *  TEST 2: downsampling
         */

        float resampling_factor = 0.5f;

        int64_t new_samples_per_channel = static_cast<int64_t>(resampling_factor * static_cast<float>(samples_per_channel));
        int64_t new_sample_rate = static_cast<uint32_t>(resampling_factor * static_cast<float>(sample_rate));

        int16_t * dst_data = reinterpret_cast<int16_t *>(malloc(new_samples_per_channel * channels * sizeof(int16_t)));

        int32_t conversion_result = -1;
        bool test_ok = true;

        conversion_result = lsrac_convert_audio(
                dst_data,                sample_data,
                new_samples_per_channel, samples_per_channel,
                2,                       2);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        conversion_result = lsrac_convert_audio(
                dst_data + 1,            sample_data + 1,
                new_samples_per_channel, samples_per_channel,
                2,                       2);
        if (conversion_result != LSRAC_RET_VAL_OK) {
            test_ok = false;
        }

        if (test_ok) {
            printf("Test 2: successful\n");
        } else {
            printf("Test 2: FAIL\n");
        }

        int32_t written = write_wav(  
            reinterpret_cast<s16_stereo_sample *>(dst_data),
            new_samples_per_channel,
            "test_2.wav",
            new_sample_rate);
        if (written < samples_per_channel) {
            printf("Error - written less than expected to test out wav file (%d < %d)\n", written, samples_per_channel);
            return -1;
        }

        free(dst_data);
    }

    drwav_free(sample_data);

    return 0;
}
