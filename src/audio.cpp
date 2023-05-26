#include "audio.h"

#include "utils.h"


#define AUDIO_FMT_PCM_INT 1
#define AUDIO_FMT_PCM_FLOAT 3


typedef struct {
    uint32_t frequency;
    uint8_t sample_width;
    uint8_t nb_channels;
    uint32_t nb_samples;
    uint16_t format;
    void* data;
} raw_music_t;


static int load_raw_wav_file(const char* filename, raw_music_t *music) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return 1;
    }

    #pragma pack(push,1)

    struct {
        char fingerprint[4];
        uint32_t file_size;
        char file_format_id[4];
    } hdr_file;

    struct {
        uint32_t block_size;
        uint16_t audio_format;
        uint16_t nb_channels;
        uint32_t frequency;
        uint32_t bytes_per_sec;
        uint16_t bytes_per_block;
        uint16_t bits_per_sample;
    } hdr_fmt;

    struct {
        uint32_t data_size;
        void* data;
    } hdr_data;

    #pragma pack(pop)

    if (fread(&hdr_file, 1, sizeof(hdr_file), file) != sizeof(hdr_file)) {
        return 2;
    }

    char* fgp = hdr_file.fingerprint;
    if (fgp[0] != 'R' || fgp[1] != 'I' || fgp[2] != 'F' || fgp[3] != 'F') {
        return 3;
    }

    char* ffid = hdr_file.file_format_id;
    if (ffid[0] != 'W' || ffid[1] != 'A' || ffid[2] != 'V' || ffid[3] != 'E') {
        return 4;
    }

    // For each header block
    char blkid[4];
    while(1) {
        if (fread(&blkid[0], 1, 4, file) != 4) {
            return 5;
        }

        //printf("Chunk ID : %c%c%c%c\n", blkid[0], blkid[1], blkid[2], blkid[3]);

        if (blkid[0] == 'J' && blkid[1] == 'U' && blkid[2] == 'N' && blkid[3] == 'K') {
            if (fseek(file, 32, SEEK_CUR)) {
                return 7;
            }
        }

        else if (blkid[0] == 'f' && blkid[1] == 'm' && blkid[2] == 't' && blkid[3] == ' ') {
            if (fread(&hdr_fmt, 1, sizeof(hdr_fmt), file) != sizeof(hdr_fmt)) {
                return 8;
            }
        }

        else if (blkid[0] == 'd' && blkid[1] == 'a' && blkid[2] == 't' && blkid[3] == 'a') {
            if (fread(&hdr_data, 1, 4, file) != 4) {
                return 9;
            }
            hdr_data.data_size = ltohi(hdr_data.data_size);
            hdr_data.data = malloc(hdr_data.data_size);
            if (fread(hdr_data.data, 1, hdr_data.data_size, file) != hdr_data.data_size) {
                return 10;
            }
            break;
        }

        else if (blkid[0] == 'L' && blkid[1] == 'I' && blkid[2] == 'S' && blkid[3] == 'T') {
            uint32_t sub_chunk_size = 0;
            if (fread(&sub_chunk_size, 1, 4, file) != 4) {
                return 11;
            }
            sub_chunk_size = ltohi(sub_chunk_size);
            if (fseek(file, sub_chunk_size, SEEK_CUR)) {
                return 12;
            }
        }

        else {
            return 6;
        }
    }

    fclose(file);

    music->nb_channels = ltohs(hdr_fmt.nb_channels);
    music->sample_width = ltohs(hdr_fmt.bits_per_sample);
    music->nb_samples = (hdr_data.data_size * 8) / music->sample_width;
    music->frequency = ltohi(hdr_fmt.frequency);
    music->format = ltohs(hdr_fmt.audio_format);
    music->data = hdr_data.data;

    printf("Data size : %i\n", hdr_data.data_size);
    printf("Nb channels : %i\n", music->nb_channels);
    printf("Sample width : %i\n", music->sample_width);
    printf("Nb samples : %i\n", music->nb_samples);
    printf("Frequency : %i\n", music->frequency);

    fflush(stdout);

    return 0;
}


bool Audio::load_wav_file(std::string filename) {
    raw_music_t raw;

    int r = load_raw_wav_file(filename.c_str(), &raw);
    if (r) {
        return r;
    }

    uint8_t nbchnl = raw.nb_channels;
    uint32_t length = raw.nb_samples / nbchnl;

    data = (float**) malloc(nbchnl * sizeof(float*));
    float* out_data = (float*) malloc(length * nbchnl * sizeof(float));
    all_data = out_data;

    for (uint32_t j = nbchnl; j--;)
        data[j] = &out_data[length * j];

    if (raw.format == AUDIO_FMT_PCM_FLOAT) {
        float* raw_data = (float*) raw.data;
        if (raw.sample_width == 32) {
            for (uint32_t i = 0; i < length; i++) {
                for (uint32_t j = nbchnl; j--;)
                    out_data[length * j + i] = raw_data[i * nbchnl + j];
            }
        } else {
            return 34;
        }
    }
    else if (raw.format == AUDIO_FMT_PCM_INT) {
        int16_t* raw_data = (int16_t*) raw.data;
        const float k = 1.f / 32767.f;
        if (raw.sample_width == 16) {
            for (uint32_t i = 0; i < length; i++) {
                for (uint32_t j = nbchnl; j--;)
                    out_data[length * j + i] = clamp((float) raw_data[i * nbchnl + j] * k, -1.f, 1.f);
            }
        } else {
            return 33;
        }
    }
    else {
        return 32;
    }

    free(raw.data);

    length = length;
    rate = raw.frequency;
    nb_channels = raw.nb_channels;

    return 0;
}
