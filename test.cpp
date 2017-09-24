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

int32_t WriteWavePCM(
    s16_stereo_sample * data, 
    size_t samples, 
    const char * file_name)
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

    file_format_header hdr;

    for (size_t i = 0; i < 4; ++i) {
        hdr.fChunkID[i]     = fChunkID[i];
        hdr.fFormat[i]      = fFormat[i];
        hdr.fSubchunk1ID[i] = fSubchunk1ID[i];
        hdr.fSubchunk2ID[i] = fSubchunk2ID[i];
    }

    static const uint32_t SAMPLE_RATE   = 44100;
    static const uint16_t BITS_PER_BYTE = 8;

    const uint32_t fByteRate = SAMPLE_RATE * N_CHANNELS * fBitsPerSample / 
                               BITS_PER_BYTE;

    const uint16_t fBlockAlign = N_CHANNELS * fBitsPerSample / BITS_PER_BYTE;

    const uint32_t fSubchunk2Size = samples * N_CHANNELS * fBitsPerSample / 
                                    BITS_PER_BYTE;

    const uint32_t fChunkSize = fRIFFChunkDescriptorLength + 
                                fFmtSubChunkDescriptorLength + fSubchunk2Size;

    hdr.fAudioFormat   = fAudioFormat;
    hdr.fBitsPerSample = fBitsPerSample;
    hdr.fBlockAlign    = fBlockAlign;
    hdr.fByteRate      = fByteRate;
    hdr.fChunkSize     = fChunkSize;
    hdr.fNumChannels   = N_CHANNELS;
    hdr.fSampleRate    = SAMPLE_RATE;
    hdr.fSubchunk1Size = fSubchunk1Size;
    hdr.fSubchunk2Size = fSubchunk2Size;

    bool little_endian = is_little_endian();

    if (!little_endian) {
        hdr.fAudioFormat   = little_endian_uint16_t(hdr.fAudioFormat);
        hdr.fBitsPerSample = little_endian_uint16_t(hdr.fBitsPerSample);
        hdr.fBlockAlign    = little_endian_uint16_t(hdr.fBlockAlign);
        hdr.fByteRate      = little_endian_uint32_t(hdr.fByteRate);
        hdr.fChunkSize     = little_endian_uint32_t(hdr.fChunkSize);
        hdr.fNumChannels   = little_endian_uint16_t(hdr.fNumChannels);
        hdr.fSampleRate    = little_endian_uint32_t(hdr.fSampleRate);
        hdr.fSubchunk1Size = little_endian_uint32_t(hdr.fSubchunk1Size);
        hdr.fSubchunk2Size = little_endian_uint32_t(hdr.fSubchunk2Size);
    }

    size_t ws = fwrite(&hdr, sizeof(hdr), 1, file);

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
    return ws == samples * N_CHANNELS;
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

    int32_t written = WriteWavePCM(  
        reinterpret_cast<s16_stereo_sample *>(sample_data),
        samples_per_channel,
        "test_out.wav");
    if (written < samples_per_channel) {
        printf("Error - written less than expected to test out wav file\n");
        return -1;
    }

    printf("test successful\n");

    drwav_free(sample_data);

    return 0;
}