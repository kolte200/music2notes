#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define PI (3.14159265)

//#define MINIMAL



typedef struct {
    float frequency;
    float amplitude;
    float duration;
    float timestamp;
} note_t;


typedef struct {
    int nb_notes;
    note_t *notes;
} partition_t;


typedef struct {
    uint32_t frequency;
    uint8_t sample_width;
    uint8_t nb_channels;
    uint32_t nb_samples;
    uint16_t format;
    void* data;
} raw_music_t;


typedef struct {
    int rate;
    int length;
    float* data;
} music_t;



uint16_t ltohs(uint16_t v) {
    return (((uint8_t*)&v)[1] << 8) | ((uint8_t*)&v)[0];
}

uint32_t ltohi(uint32_t v) {
    uint8_t* x = (uint8_t*) &v;
    return  (x[3] << 24) | (x[2] << 16) | (x[1] << 8) | x[0];
}

int16_t utoss(uint16_t v) {
    return *((int16_t*)&v);
}



#ifndef MINIMAL

#include <portaudio.h>
#include <SDL2/SDL.h>



typedef struct {
    PaStream* stream;
    PaStreamParameters parameters;
    int frequency;
    float* sample;
    int sample_len;
    bool opened;
    int sample_offset;
} tone_player_t;


typedef struct {
    PaStream* stream;
    PaStreamParameters parameters;
    music_t* music;
    int offset;
    bool finished;
} music_player_t;


typedef struct {
    SDL_Window* window;
    SDL_Surface* surface;
    bool drawable;
} graph_t;



const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = SAMPLE_RATE / 32;

static int _tone_player_callback( const void *input_buffer, void *output_buffer,
                            unsigned long frames_per_buffer,
                            const PaStreamCallbackTimeInfo* time_info,
                            PaStreamCallbackFlags status_flags,
                            void *user_data )
{
    tone_player_t* player = (tone_player_t*) user_data;
    if (player->sample != NULL) {
        float* out = (float*) output_buffer;
        int j = player->sample_offset;
        for (int i = 0; i < frames_per_buffer;) {
            for (; j < player->sample_len && i < frames_per_buffer; j++, i++) {
                out[i] = player->sample[j];
            }
            if (i < frames_per_buffer) j = 0;
        }
        player->sample_offset = j;
    }
    return paContinue;
}

int play_tone(tone_player_t* player, int frequency, float volume) {
    // Generate sample

    if (player->sample != NULL) {
        free(player->sample);
        player->sample = NULL;
    }

    if (frequency < 20 || frequency * 2 > SAMPLE_RATE) {
        player->sample_len = FRAMES_PER_BUFFER;
        player->sample = (float*) malloc(player->sample_len * sizeof(float));
        player->sample_offset = 0;

        for (int i = 0; i < player->sample_len; i++) {
            player->sample[i] = 0;
        }

        return 0;
    }

    player->sample_len = SAMPLE_RATE / frequency;
    player->sample = (float*) malloc(player->sample_len * sizeof(float));
    if (player->frequency != frequency)
        player->sample_offset = 0;

    for (int i = 0; i < player->sample_len; i++) {
        player->sample[i] = sin(PI * 2 * i / player->sample_len) * volume;
    }

    player->frequency = frequency;

    return 0;
}

int open_tone_player(tone_player_t* player)
{
    player->frequency = 0;
    player->opened = false;
    player->sample_len = 0;
    player->sample_offset = 0;
    player->sample = NULL;

    PaError err;

    player->parameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (player->parameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default output device.\n");
        goto error;
    }
    player->parameters.channelCount = 1;
    player->parameters.sampleFormat = paFloat32;
    player->parameters.suggestedLatency = Pa_GetDeviceInfo( player->parameters.device )->defaultLowOutputLatency;
    player->parameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &player->stream,
              NULL, /* no input */
              &player->parameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              _tone_player_callback,
              player );
    if( err != paNoError ) goto error;

    play_tone(player, 0, 0);

    err = Pa_StartStream( player->stream );
    if( err != paNoError ) goto error;

    player->opened = true;

    return paNoError;

error:
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}

int close_tone_player(tone_player_t* player) {
    PaError err;

    err = Pa_StopStream( player->stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( player->stream );
    if( err != paNoError ) goto error;

    return paNoError;

error:
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}



static int _music_player_callback( const void *input_buffer, void *output_buffer,
                            unsigned long frames_per_buffer,
                            const PaStreamCallbackTimeInfo* time_info,
                            PaStreamCallbackFlags status_flags,
                            void *user_data )
{
    music_player_t* player = (music_player_t*) user_data;
    float* out = (float*) output_buffer;
    if (!player->finished) {
        int i = player->offset;
        for (int j = 0; i < player->music->length && j < frames_per_buffer; i++, j++) {
            out[j] = player->music->data[i];
        }
        player->offset = i;
        if (i >= player->music->length) {
            player->finished = true;
            return paAbort;
        }
        return paContinue;
    }
    return paAbort;
}

static void _music_player_finish_callback( void *user_data )
{
    music_player_t* player = (music_player_t*) user_data;
    player->finished = true;
}

int play_music(music_player_t* player, music_t* music)
{
    PaError err;

    player->parameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (player->parameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default output device.\n");
        goto error;
    }
    player->parameters.channelCount = 1;
    player->parameters.sampleFormat = paFloat32;
    player->parameters.suggestedLatency = Pa_GetDeviceInfo( player->parameters.device )->defaultLowOutputLatency;
    player->parameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &player->stream,
              NULL, /* no input */
              &player->parameters,
              music->rate,
              music->rate / 20,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              _music_player_callback,
              player );
    if( err != paNoError ) goto error;

    err = Pa_SetStreamFinishedCallback( player->stream, &_music_player_finish_callback );
    if( err != paNoError ) goto error;

    player->finished = false;
    player->music = music;
    player->offset = 0;

    err = Pa_StartStream( player->stream );
    if( err != paNoError ) goto error;

    return paNoError;

error:
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    fflush( stderr );
    return err;
}

int wait_music(music_player_t* player) {
    while (!player->finished) {
        Pa_Sleep(1);
    }
    return paNoError;
}

int stop_music(music_player_t* player) {
    PaError err;

    err = Pa_StopStream( player->stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( player->stream );
    if( err != paNoError ) goto error;

    return paNoError;

error:
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    fflush( stderr );
    return err;
}



int graph_create(graph_t* graph, int width, int height) {
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    graph->window = SDL_CreateWindow("Graphic", 100, 100, width, height, SDL_WINDOW_SHOWN);
    if (graph->window == NULL) {
        fprintf(stderr, "SDL_CreateWindow Error: %s", SDL_GetError());
        return 1;
    }
    graph->surface = SDL_GetWindowSurface(graph->window);
    graph->drawable = true;
    return 0;
}

int graph_destroy(graph_t* graph) {
    graph->drawable = false;
    if (graph->window != NULL)
        SDL_DestroyWindow(graph->window);
    SDL_Quit();
    return 0;
}

int graph_render(graph_t* graph, float* vals, int nvals, float yscale) {
    if (!graph->drawable) return 2;

    int w = graph->surface->w;
    int h = graph->surface->h;
    SDL_FillRect(graph->surface, NULL, 0);
    SDL_LockSurface(graph->surface);
    uint8_t* pixels = (uint8_t*) graph->surface->pixels;
    float k = ((float) nvals) / w;
    for (int x = 0; x < w; x++) {
        int vx = x * k;
        int val = vals[vx] * h * yscale;
        if (val < 0) val = 0;
        if (val > h) val = h;
        for (int y = 0; y < val; y++) {
            int i = ((h - y - 1) * w + x) * 4;
            pixels[i + 0] = 0xFF;
            pixels[i + 1] = 0xFF;
            pixels[i + 2] = 0xFF;
        }
    }
    SDL_UnlockSurface(graph->surface);

    SDL_Event event;
    while(SDL_PollEvent(&event) > 0) {
        switch(event.type) {
            case SDL_QUIT:
                graph_destroy(graph);
                return 1;
        }
    }

    SDL_UpdateWindowSurface(graph->window);
    return 0;
}

#endif


static inline float absf(float a) {
    return a > 0 ? a : -a;
}

static inline float maxf(float a, float b) {
    return a > b ? a : b;
}

static inline float minf(float a, float b) {
    return a > b ? b : a;
}

static inline float clampf(float a, float minv, float maxv) {
    return minf(maxf(a, minv), maxv);
}

static inline float cyclef(float from, float to, float modulus) {
    const float s = modulus * .5f;
    float r = fmodf(to - from + s, modulus);
    if (r < 0) r += modulus;
    return r - s;
}


#define AUDIO_FMT_PCM_INT 1
#define AUDIO_FMT_PCM_FLOAT 3


int load_wave_from_filename(const char* filename, raw_music_t *music) {
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

        if (blkid[0] == 'J' && blkid[1] == 'U' & blkid[2] == 'N' && blkid[3] == 'K') {
            if (fseek(file, 32, SEEK_CUR)) {
                return 7;
            }
        }

        else if (blkid[0] == 'f' && blkid[1] == 'm' & blkid[2] == 't' && blkid[3] == ' ') {
            if (fread(&hdr_fmt, 1, sizeof(hdr_fmt), file) != sizeof(hdr_fmt)) {
                return 8;
            }
        }

        else if (blkid[0] == 'd' && blkid[1] == 'a' & blkid[2] == 't' && blkid[3] == 'a') {
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

        else if (blkid[0] == 'L' && blkid[1] == 'I' & blkid[2] == 'S' && blkid[3] == 'T') {
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


int load_music_from_filename(const char* filename, music_t* music) {
    raw_music_t raw;

    int r = load_wave_from_filename(filename, &raw);
    if (r) {
        return r;
    }

    uint8_t nbchnl = raw.nb_channels;
    int length = raw.nb_samples / nbchnl;

    if (raw.format == AUDIO_FMT_PCM_FLOAT) {
        if (raw.sample_width == 32) {
            float* raw_data = (float*) raw.data;
            if (nbchnl > 1) {
                float* data = (float*) malloc(length * sizeof(float));
                for (uint32_t i = 0; i < length; i++) {
                    float s = 0;
                    uint32_t x = i * nbchnl;
                    for (uint32_t j = nbchnl; j--;)
                        s += raw_data[x + j];
                    data[i] = s / nbchnl;
                }
                free(raw.data);
                music->data = data;
            } else {
                music->data = raw_data;
            }
        }

        else {
            return 34;
        }
    }

    else if (raw.format == AUDIO_FMT_PCM_INT) {
        float* data = (float*) malloc(length * sizeof(float));

        if (raw.sample_width == 16) {
            int16_t* raw_data = (int16_t*) raw.data;
            if (nbchnl > 1) {
                for (uint32_t i = 0; i < length; i++) {
                    int32_t s = 0;
                    uint32_t x = i * nbchnl;
                    for (uint32_t j = nbchnl; j--;)
                        s += raw_data[x + j];
                    data[i] = ((float) s) / (nbchnl * 32767.0);
                }
            } else {
                for (uint32_t i = 0; i < raw.nb_samples; i++) {
                    data[i] = ((float) raw_data[i]) / 32767.0;
                }
            }
        }

        else {
            return 33;
        }

        free(raw.data);
        music->data = data;
    }

    else {
        return 32;
    }

    music->length = length;
    music->rate = raw.frequency;

    return 0;
}



#define NOTE_FREQ_RATIO (1.0594630943592953) // 2**(1/12)
#define HALF_NOTE_FREQ_RATIO (1.029302236643492) // 2**(1/24)


float* compute_sample_freqs_1(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Create frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 10e10;

    // Copy data sample
    float* src = &music->data[offset];
    float* data = (float*) malloc(sizeof(float) * length);
    for (int i = length; i--;) data[i] = src[i];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        float d = music->rate / (2 * freq); // Half the period length in number of samples
        // Split sample in area of length d
        int m = 0, j = 0;
        // For each phase
        float step = d / 10;
        for (float phase = 0; phase < d; phase += step) {
            float score1 = 0, score2 = 0; // Represent how much this frequency is present in the sample for this phase
            int lastmin = 0, lastmax = 0;
            uint8_t first = 1;
            int i = 0;
            for (; m < length - phase; i++) {
                // Get min and max of current area
                j = d * i + phase;
                m = d * (i + 1) + phase;
                if (m > length) m = length;
                float minv = data[j];
                float maxv = minv;
                for (j++; j < m; j++) {
                    float v = data[j];
                    if (v > maxv) maxv = v;
                    if (v < minv) minv = v;
                }
                if (first) {
                    first = 0;
                } else {
                    if (i & 1) {
                        score1 += maxv - lastmin;
                        score2 += lastmax - minv;
                    } else {
                        score1 += lastmax - minv;
                        score2 += maxv - lastmin;
                    }
                }
            }
            float score = ((score1 > score2) ? score1 : score2) / i;
            if (score < freqs[freq_i])
                freqs[freq_i] = score;
        }
    }

    freq *= NOTE_FREQ_RATIO;
    for (int i = 0, m = nb_freqs; i < m; i++, freq *= NOTE_FREQ_RATIO) {
        float d = music->rate / freq;
        float trgt = music->rate / (d - ((2*d*d) / length));
        int j = i;
        for (float f = freq; f < trgt; f *= NOTE_FREQ_RATIO, j++);
        if (j < nb_freqs)
            freqs[i] -= freqs[j];
    }

    free(data);

    return freqs;
}


float* compute_sample_freqs_2(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Create frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    // Copy data sample
    float* src = &music->data[offset];
    float* data = (float*) malloc(sizeof(float) * length);
    for (int i = length; i--;) data[i] = src[i];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        float d = music->rate / freq; // The period length in number of samples
        if (d >= length) continue;
        // Split sample in area of length d for compute of average

        // Compute initial average
        float avg = 0;
        int i = 0;
        int m = (int) d;
        for (; i < m; i++) {
            avg += data[i];
        }
        avg /= m;

        float s1 = 0;
        float s2 = 0;
        int cut = i + (m / 2); // Next point where the signal must be reverted
        float target = 0;
        float current = 0;
        float growth = 0;
        for (; i < length; i++) {
            if (i >= cut) cut = i + (m / 2);

            float v = data[i] - avg;
            target += v;

            if ((growth > 0 && current >= target) || (growth < 0 && current <= target)) {
                cut = i + (m / 2);
            }
            growth = (target - current) / ((cut - i) * 2);

            current += growth;
            s1 += growth;
            s2 += current;

            // Compute new average
            avg += (data[i + 1] - data[i]) / m;
        }

        float n = (length - m);
        s1 /= n;
        s2 /= n;
        freqs[freq_i] = s2;
    }

    free(data);

    return freqs;
}


float* compute_sample_freqs_3(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Create frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    // Copy data sample
    float* src = &music->data[offset];
    float* data = (float*) malloc(sizeof(float) * length);
    for (int i = length; i--;) data[i] = src[i];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        float d = music->rate / (freq * 2); // Half the period length in number of samples
        if (d >= length) continue;

        // Split sample in area of length d for compute of average
        int nb_phases = (int) d;
        float* phases = malloc(sizeof(float) * nb_phases);

        for (int i = 0; i < length; i++) {
            int step = i / nb_phases;
            int offset = i - (step * nb_phases);
            if (step & 1) {
                phases[offset] -= data[i];
            } else {
                phases[offset] += data[i];
            }
        }

        // Get the score of the best phases
        float score = 0;
        for (int i = 0; i < nb_phases; i++) {
            float v = phases[i];
            if (v < 0) v = -v;
            if (v > score) score = v;
        }
        free(phases);

        freqs[freq_i] = score / (length / d);
    }

    for (int i = 0, m = nb_freqs / 2; i < m; i++) {
        freqs[i] -= freqs[i * 2];
    }

    free(data);

    return freqs;
}


float* compute_sample_freqs_4(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Create frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];
    float* avgs = (float*) malloc(length * sizeof(float)); // avgs_len = ((max_freq * 2 * length) / music->rate) * (music->rate / (2 * max_freq)) = length

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        int d = (int) (music->rate / (freq * 2));
        if (d < 1) continue;
        int nb_phases = d;
        int nb_avgs = (length - nb_phases + 1) / d;
        if (nb_avgs < 2) continue;

        // Compute initial average
        for (int i = 0, j = 0, m1 = length - d; i < m1; j++) {
            float sum = 0;
            for(int m2 = i + d; i < m2; i++) sum += data[i];
            avgs[j] = sum / d;
        }

        float score = 0;
        int bphase = 0;

        int freqint = (int) (freq + 0.5);

        // For each phase
        for (int phase = 0; phase < nb_phases; phase++) {
            // Compute the new averages
            if (phase > 0) {
                for (int i = 0; i < nb_avgs; i++) {
                    int x = i * d + phase - 1;
                    avgs[i] += (data[x + d] - data[x]) / d;
                }
            }

            // Compute the score
            float s = 0;
            for (int i = 1; i < nb_avgs; i++) {
                if (i & 1) {
                    s -= avgs[i] - avgs[i - 1];
                } else {
                    s += avgs[i] - avgs[i - 1];
                }
            }

            if (s < 0) s = -s;
            if (s > score) {
                score = s;
                bphase = phase;
            }
        }
        score /= nb_avgs - 1;

        printf("F:%i, P:%i, S:%f\n", freqint, bphase, score);
        freqs[freq_i] = score;
    }
    printf("\n");
    fflush(stdout);

    free(avgs);

    return freqs;
}


float* compute_sample_freqs_5(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];
    float* avgs = (float*) malloc(length * sizeof(float)); // avgs_len = ((max_freq * 2 * length) / music->rate) * (music->rate / (2 * max_freq)) = length

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        float period = music->rate / freq;
        float min_period = music->rate / (freq * HALF_NOTE_FREQ_RATIO);
        float max_period = music->rate / (freq / HALF_NOTE_FREQ_RATIO);
        float half_period = 0.5 * period;
        float half_length = length / 2.0;
        int nb_phases = (int) half_period;
        int nb_avgs = (length - nb_phases + 1) / half_period;

        if (half_period < 1) continue;
        if (nb_avgs < 2) continue;

        // Compute initial average
        for (int j = 0; j < nb_avgs; j++) {
            float sum = 0;
            int i = j * half_period;
            for(int m = (j + 1) * half_period; i < m; i++)
                sum += data[i];
            avgs[j] = sum / i;
        }

        // Compute average for each phases
        for (int phase = 1; phase < nb_phases; phase++) {
            int avgs_offset = phase * nb_avgs;
            int last_offset = (phase - 1) * nb_avgs;
            for (int i = 0; i < nb_avgs; i++) {
                int a = ((int) (i * half_period)) + phase - 1;
                int b = ((int) ((i + 1) * half_period)) + phase - 1;
                avgs[avgs_offset + i] = avgs[last_offset + i] + (data[b] - data[a]) / (b - a);
            }
        }

        float part_len = half_period / (period - min_period);
        if (part_len > half_length) part_len = half_length;

        // Compute the average score for the first half and for each phase and find the phase with the best score
        float first_best_phase_score = 0;
        float first_best_phase = 0;

        int part_len_int = (int) part_len;
        for (int phase = 0; phase < nb_phases; phase++) {
            float phase_score = 0;
            int avgs_offset = phase * nb_avgs;
            for (int i = 0; i < part_len_int; i++) {
                if (i & 1)
                    phase_score -= avgs[avgs_offset + i] - avgs[avgs_offset + i - 1];
                else
                    phase_score += avgs[avgs_offset + i] - avgs[avgs_offset + i - 1];
            }

            uint8_t neg = phase_score < 0;
            if (neg) phase_score = -phase_score;
            if (phase_score > first_best_phase_score) {
                first_best_phase_score = phase_score;
                first_best_phase = phase;
                if (neg) first_best_phase -= half_period;
            }
        }

        // Compute the average score for the second half and for each phase and find the phase with the best score
        float scnd_best_phase_score = 0;
        float scnd_best_phase = 0;

        int part_len_int_x2 = (int) (part_len * 2);
        for (int phase = 0; phase < nb_phases; phase++) {
            float phase_score = 0;
            int avgs_offset = phase * nb_avgs;
            for (int i = part_len_int; i < part_len_int_x2; i++) {
                if (i & 1)
                    phase_score -= avgs[avgs_offset + i] - avgs[avgs_offset + i - 1];
                else
                    phase_score += avgs[avgs_offset + i] - avgs[avgs_offset + i - 1];
            }

            uint8_t neg = phase_score < 0;
            if (neg) phase_score = -phase_score;
            if (phase_score > scnd_best_phase_score) {
                scnd_best_phase_score = phase_score;
                scnd_best_phase = phase;
                if (neg) scnd_best_phase -= half_period;
            }
        }

        // Compute the progressive phase offset and clamp it
        float phase_offset = scnd_best_phase - first_best_phase;
        if (phase_offset < -half_period) phase_offset = period + phase_offset;
        else if (phase_offset > half_period) phase_offset = period - phase_offset;

        float phase_offset_per_period = phase_offset / part_len;
        if (period + phase_offset_per_period > max_period)
            phase_offset_per_period = max_period - period;
        else if (period - phase_offset_per_period < min_period)
            phase_offset_per_period = min_period - period;

        // Compute the best phase for the first period in data
        float first_period_phase = (first_best_phase + phase_offset / 2) - (phase_offset_per_period * part_len);

        printf("First: %f; Offset: %f\n", first_period_phase, phase_offset_per_period); fflush(stdout);

        // Compute the final score with taking into account the progressive phase offset (or frequency/period offset)
        float final_score = 0;
        float phase = first_period_phase;

        if (phase < -half_period) phase += period;
        if (phase >  half_period) phase -= period;

        int last_avg = 0;

        int i = 0;
        int m = part_len_int_x2 < nb_avgs ? part_len_int_x2 : nb_avgs;
        for (; i < m; i++) {
            float x = phase / half_period;
            int y = (int) x;
            int phase_index = (int) ((x * y) * half_period);
            int avgs_index = i + y;
            if (avgs_index >= nb_avgs) break;

            float avg = avgs[phase_index * nb_avgs + avgs_index];

            if (i > 0) {
                if (i & 1)
                    final_score -= avg - last_avg;
                else
                    final_score += avg - last_avg;
            }

            last_avg = avg;
        }
        //if (final_score < 0) final_score = -final_score;

        /*float final_score = 0;
        int part_len_int = (int) part_len;
        for (int phase = 0; phase < nb_phases; phase++) {
            float phase_score = 0;
            int avgs_offset = phase * nb_avgs;
            for (int i = 1; i < part_len_int; i++) {
                if (i & 1)
                    phase_score -= avgs[avgs_offset + i] - avgs[avgs_offset + i - 1];
                else
                    phase_score += avgs[avgs_offset + i] - avgs[avgs_offset + i - 1];
            }
            if (phase_score < 0) phase_score = -phase_score;
            if (phase_score > final_score) {
                final_score = phase_score;
            }
        }*/

        final_score /= i - 1;

        printf("Frequency: %i, Score: %f\n", (int) (freq + 0.5), final_score);

        freqs[freq_i] = final_score;
    }
    printf("\n");
    fflush(stdout);

    free(avgs);

    return freqs;
}


float* compute_sample_freqs_6(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        float speed = 0;
        float dist = 0;
        float k = 0;

        // Compute the average in two part to find the estimate phase shift

        float period = music->rate / freq;

        if (period > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        float min_period = music->rate / (freq * HALF_NOTE_FREQ_RATIO);
        float max_period = music->rate / (freq / HALF_NOTE_FREQ_RATIO);
        int half_period    = (int) (period * .5 + .5);
        int quarter_period = (int) (period * .25 + .5);

        float avg1 = 0;
        for (int i = 0; i < quarter_period; i++)
            avg1 += data[i];
        avg1 /= quarter_period;

        float avg2 = 0;
        for (int i = 0; i < quarter_period; i++)
            avg2 += data[i];
        avg2 /= quarter_period;

        // Find initial value of dist and speed
        // dist(n) = integral(0, n, k * integral(0, m-1, 1 - dist(t) dt) dm)
        // dist(n) = integral(0, n, (m-1 - primitive(dist)(m-1)) dm) * k
        // dist(n) = (n^2/2 - n - primitive(primitive(dist))(n-1)) * k
        // dist'(n) = (n - 1 - primitive(dist)(n-1)) * k
        // dist"(n) = (1 - dist(n-1)) * k
        // f"(t) = (1 - f(t-1)) * k
        // f(t) = sin(sqrt(k)*t)
        // sin(sqrt(k)*n) = 1 <=> sqrt(k)*n = pi/2 <=> k = (pi/(2*n))^2

        /*
        // See the file "test.py" to known how I found the formula to find k
        //k = (1.f / (quarter_period*quarter_period));
        k = powf((float) PI / (2.f * quarter_period), 2.f);

        // Begin analyze

        float last_extremum_val = 0.f;
        int last_extremum_pos = 0;
        float amplitude = 0.f;
        float last_speed = speed;
        for (int i = 0; i < length; i++) {
            float val = data[i];
            speed += (val - dist) * k;
            dist += speed;

            if (speed > 0) if (dist > val) dist = val;
            else if (dist < val) dist = val;

            if (speed * last_speed < 0) {
                amplitude += absf(last_extremum_val - dist);// * (i - last_extremum_pos);
                last_extremum_val = dist;
                last_extremum_pos = i;
            }
            last_speed = speed;
        }
        */

        // dist(t) = k * primitive(1 - dist)(t)
        // f'(t) = k * (1 - f(t))
        // f(t) = 1 - 2*e^(-k*t)
        // f(n) = 0 <=> 1 - 2*e^(-k*n) = 0 <=> k = ln(2)/n
        k = 2.f / half_period;

        float last_extremum_val = 0.f;
        int last_extremum_pos = 0;
        float amplitude = 0.f;
        float last_speed = speed;
        for (int i = 0; i < length; i++) {
            float val = data[i];
            speed = (val < dist) ? -k : k;
            dist += speed;

            if (speed > 0) if (dist > val) dist = val;
            else if (dist < val) dist = val;

            if (speed * last_speed < -0.0001f) {
                amplitude += absf(last_extremum_val - dist);
                last_extremum_val = dist;
                last_extremum_pos = i;
            }
            last_speed = speed;
        }

        freqs[freq_i] = amplitude / freq;
    }

    return freqs;
}


float* compute_sample_freqs_7(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        const float period = music->rate / freq;

        const int half_period    = period * .5;
        const int half_period_i = (int) (half_period + .5f);

        const int period_i = half_period_i * 2;

        if (period_i > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        const int quarter_period = period * .25;

        // Commpute initial averages
        float avg1 = 0.f;
        for (int i = 0; i < half_period_i; i++) avg1 += data[i];
        avg1 /= half_period_i;

        float avg2 = 0.f;
        for (int i = half_period_i; i < period_i; i++) avg2 += data[i];
        avg2 /= half_period_i;

        // Compute two dynamic average
        float sum_of_diff = absf(avg1 - avg2);
        for (int i = 0, m = length - period_i; i < m; i++) {
            avg1 += (data[i + half_period_i] - data[i]) / half_period_i;
            avg2 += (data[i + period_i] - data[i + half_period_i]) / half_period_i;
            sum_of_diff += absf(avg1 - avg2);
        }

        const float max_possible_score = quarter_period * ((length - period) / quarter_period);
        freqs[freq_i] = sum_of_diff / max_possible_score;
    }

    return freqs;
}


float* compute_sample_freqs_8(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        const float period = music->rate / freq;

        const int period_i = (int) (period + .5f);

        if (period_i > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        // Commpute initial averages
        float avg = 0.f;
        for (int i = 0; i < period_i; i++) avg += data[i];
        avg /= period_i;

        // Compute dynamic average
        float score = 0.f;
        for (int i = 0, m = length - period_i; i < m; i++) {
            avg += (data[i + period_i] - data[i]) / period_i;
            float v = data[i] - avg;
            float minv = v;
            float maxv = v;
            for (int j = i + 1, n = i + period_i; j < n; j++) {
                v += data[j] - avg;
                if (v < minv) minv = v;
                if (v > maxv) maxv = v;
            }
            score += maxv - minv;
        }

        const float max_possible_score = (float) (length - period_i) * period;
        freqs[freq_i] = score / max_possible_score;
    }

    return freqs;
}

// Fourier transform
float* compute_sample_freqs_9(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        const float period = music->rate / freq;
        const int period_i = (int) (period + .5f);

        if (period_i > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        const float k = 2.f * (float) PI / (float) period_i;

        float sin_buff[period_i];
        for (int i = 0; i < period_i; i++)
            sin_buff[i] = sinf(k * i);

        float cos_buff[period_i];
        for (int i = 0; i < period_i; i++)
            cos_buff[i] = cosf(k * i);

        float x = 0, y = 0;
        for (int i = 0, m = length; i < m; i++) {
            float v = data[i];
            x += cos_buff[i % period_i] * v;
            y += sin_buff[i % period_i] * v;
        }

        freqs[freq_i] = sqrtf(x*x + y*y) / (float) (length);
    }

    return freqs;
}

// Identical to compute_sample_freqs_8 but faster
float* compute_sample_freqs_10(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        const float period = music->rate / freq;

        const int period_i = (int) (period + .5f);

        if (period_i > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        float score = 0.f;
        for (int i = 0, m = length - period_i; i < m; i += period_i) {
            // Compute average
            float avg = 0.f;
            for (int j = i, n = i + period_i; j < n; j++) avg += data[j];
            avg /= period_i;

            float v = data[i] - avg;
            float minv = v;
            float maxv = v;

            for (int j = i + 1, n = i + period_i; j < n; j++) {
                v += data[j] - avg;
                if (v < minv) minv = v;
                if (v > maxv) maxv = v;
            }

            score += maxv - minv;
        }

        const float max_possible_score = (float) (length);
        freqs[freq_i] = score / max_possible_score;
    }

    return freqs;
}

// As compute_sample_freqs_10 but taking advantage of the length of the sample as compute_sample_freqs_1
float* compute_sample_freqs_11(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        const float period = music->rate / freq;

        const int period_i = (int) (period + .5f);

        if (period_i > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        float sum_of_periods[period_i];
        for (int i = 0; i < period_i; i++)
            sum_of_periods[i] = 0.f;

        for (int i = 0, m = length - period_i; i < m; i += period_i) {
            for (int j = 0; j < period_i; j++)
                sum_of_periods[j] += data[i + j];
        }

        float avg = 0.f;
        for (int i = 0; i < period_i; i++) avg += sum_of_periods[i];
        avg /= period_i;

        float v = sum_of_periods[0] - avg;
        float minv = v;
        float maxv = v;

        for (int i = 1; i < period_i; i++) {
            v += sum_of_periods[i] - avg;
            if (v < minv) minv = v;
            if (v > maxv) maxv = v;
        }

        freqs[freq_i] = (maxv - minv) / (float) length;
    }

    return freqs;
}

// As compute_sample_freqs_11 but taking into account a range of frequency (so compasate a little phase shift)
float* compute_sample_freqs_12(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        const float period = music->rate / freq;

        const int period_i = (int) (period + .5f);

        if (period_i > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        const float k = 2.f * (float) PI / (float) period_i;

        float sin_buff[period_i];
        for (int i = 0; i < period_i; i++)
            sin_buff[i] = sinf(k * i);

        float cos_buff[period_i];
        for (int i = 0; i < period_i; i++)
            cos_buff[i] = cosf(k * i);

        const float min_period = music->rate / (freq * HALF_NOTE_FREQ_RATIO);
        const float max_period = music->rate / (freq / HALF_NOTE_FREQ_RATIO);

        const float min_shift = period - max_period;
        const float max_shift = period - min_period;

        float sum_of_periods[period_i];
        for (int i = 0; i < period_i; i++)
            sum_of_periods[i] = 0.f;

        float current_phase = NAN;
        float current_shift = 0.f;

        for (int i = 0, m = length - period_i; i < m; i += period_i) {
            // Compute phase to shift the phase if needed
            float x = 0, y = 0;
            for (int j = 0; j < period_i; j++) {
                float v = data[j + i];
                x += cos_buff[j] * v;
                y += sin_buff[j] * v;
            }
            float phase = atan2f(y, x);

            // Compute needed shift
            if (isnan(current_phase)) current_phase = phase;
            float delta_shift = clampf(cyclef(phase + current_shift, current_phase, period), min_shift, max_shift);
            current_shift += delta_shift;

            // Add to sum with shift
            int shift_i = -((int) (current_shift + .5f));
            shift_i = shift_i % period_i;
            if (shift_i < 0) shift_i += period_i;
            for (int j = 0; j < period_i; j++)
                sum_of_periods[(j + shift_i) % period_i] += data[i + j];
        }

        float avg = 0.f;
        for (int i = 0; i < period_i; i++) avg += sum_of_periods[i];
        avg /= period_i;

        float v = sum_of_periods[0] - avg;
        float minv = v;
        float maxv = v;

        for (int i = 1; i < period_i; i++) {
            v += sum_of_periods[i] - avg;
            if (v < minv) minv = v;
            if (v > maxv) maxv = v;
        }

        freqs[freq_i] = (maxv - minv) / (float) length;
    }

    return freqs;
}

// As compute_sample_freqs_11 but with a dynamic average
float* compute_sample_freqs_13(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        const float period = music->rate / freq;

        const int period_i = (int) (period + .5f);

        if (period_i > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        const int half_period_i = (int) (period * .5f + .5f);

        float sum_of_periods[period_i];
        for (int i = 0; i < period_i; i++)
            sum_of_periods[i] = 0.f;

        // Initial everage
        float avg = 0.f;
        for (int i = 0; i < period_i; i++) avg += data[i];
        avg /= period_i;

        for (int i = half_period_i, m = length - period_i - half_period_i; i < m; i += period_i) {
            for (int j = 0; j < period_i; j++) {
                const int index = i + j;
                sum_of_periods[j] += data[index] - avg;
                // Compute new average
                avg += (data[index + half_period_i] - data[index - half_period_i]) / period_i;
            }
        }

        avg = 0.f;
        for (int i = 0; i < period_i; i++) avg += sum_of_periods[i];
        avg /= period_i;

        float v = sum_of_periods[0] - avg;
        float minv = v;
        float maxv = v;

        for (int i = 1; i < period_i; i++) {
            v += sum_of_periods[i] - avg;
            if (v < minv) minv = v;
            if (v > maxv) maxv = v;
        }

        freqs[freq_i] = (maxv - minv) / (float) length;
    }

    return freqs;
}

// Combinaison of  compute_sample_freqs_12 and compute_sample_freqs_13
float* compute_sample_freqs_14(music_t* music, int offset, int length, float max_freq, int nb_freqs) {
    // Alloc frequencies histograme
    float* freqs = (float*) malloc(nb_freqs * sizeof(float));
    for (int i = nb_freqs; i--;) freqs[i] = 0;

    float* data = &music->data[offset];

    // For each frequency
    float freq = max_freq;
    for (int freq_i = nb_freqs; freq_i--; freq /= NOTE_FREQ_RATIO) {
        const float period = music->rate / freq;

        const int period_i = (int) (period + .5f);

        if (period_i > length) {
            freqs[freq_i] = 0.f;
            continue;
        }

        const int half_period_i = (int) (period * .5f + .5f);

        const float k = 2.f * (float) PI / (float) period_i;

        float sin_buff[period_i];
        for (int i = 0; i < period_i; i++)
            sin_buff[i] = sinf(k * i);

        float cos_buff[period_i];
        for (int i = 0; i < period_i; i++)
            cos_buff[i] = cosf(k * i);

        const float min_period = music->rate / (freq * HALF_NOTE_FREQ_RATIO);
        const float max_period = music->rate / (freq / HALF_NOTE_FREQ_RATIO);

        const float min_shift = period - max_period;
        const float max_shift = period - min_period;

        float sum_of_periods[period_i];
        for (int i = 0; i < period_i; i++)
            sum_of_periods[i] = 0.f;

        // Initial average
        float avg = 0.f;
        for (int i = 0; i < period_i; i++) avg += data[i];
        avg /= period_i;

        float avgs[period_i];

        float current_phase = NAN;
        float current_shift = 0.f;

        for (int i = half_period_i, m = length - period_i - half_period_i; i < m; i += period_i) {
            for (int j = 0; j < period_i; j++) {
                const int index = i + j;
                avgs[j] = avg;
                // Compute new average
                avg += (data[index + half_period_i] - data[index - half_period_i]) / period_i;
            }

            // Compute phase to shift the phase if needed
            float x = 0, y = 0;
            for (int j = 0; j < period_i; j++) {
                float v = data[j + i] - avgs[j];
                x += cos_buff[j] * v;
                y += sin_buff[j] * v;
            }
            float phase = atan2f(y, x);

            // Compute needed shift
            if (isnan(current_phase)) current_phase = phase;
            float delta_shift = clampf(cyclef(phase + current_shift, current_phase, period), min_shift, max_shift);
            current_shift += delta_shift;

            // Add to sum with shift
            int shift_i = -((int) (current_shift + .5f));
            shift_i = shift_i % period_i;
            if (shift_i < 0) shift_i += period_i;
            for (int j = 0; j < period_i; j++)
                sum_of_periods[(j + shift_i) % period_i] += data[i + j] - avgs[j];
        }

        avg = 0.f;
        for (int i = 0; i < period_i; i++) avg += sum_of_periods[i];
        avg /= period_i;

        float v = sum_of_periods[0] - avg;
        float minv = v;
        float maxv = v;

        for (int i = 1; i < period_i; i++) {
            v += sum_of_periods[i] - avg;
            if (v < minv) minv = v;
            if (v > maxv) maxv = v;
        }

        freqs[freq_i] = (maxv - minv) / (float) length * 2.f;
    }

    return freqs;
}


// https://williamssoundstudio.com/tools/iso-226-equal-loudness-calculator-fletcher-munson.php
const float HEAR_STRENGTH_PER_FREQ[] = {
    20, 99.85,
    25, 93.94,
    31.5, 88.17,
    40, 82.63,
    50, 77.78,
    63, 73.08,
    80, 68.48,
    100, 64.37,
    125, 60.59,
    160, 56.70,
    200, 53.41,
    250, 50.40,
    315, 47.58,
    400, 44.98,
    500, 43.05,
    630, 41.34,
    800, 40.06,
    1000, 40.01,
    1250, 41.82,
    1600, 42.51,
    2000, 39.23,
    2500, 36.51,
    3150, 35.61,
    4000, 36.65,
    5000, 40.01,
    6300, 45.83,
    8000, 51.80,
    10000, 54.28,
    12500, 51.49
};

float get_perceived_strength(float freq, float strength) {
    const int n = sizeof(HEAR_STRENGTH_PER_FREQ) / sizeof(HEAR_STRENGTH_PER_FREQ[0]);
    for (int i = 2; i < n; i += 2) {
        float a = HEAR_STRENGTH_PER_FREQ[i];
        if (freq < a) {
            float b = HEAR_STRENGTH_PER_FREQ[i-2];
            float c = HEAR_STRENGTH_PER_FREQ[i-1];
            float d = HEAR_STRENGTH_PER_FREQ[i+1];
            float db = (freq - a) / (b - a) * (d - c) + c;
            return strength / sqrtf(powf(10.f, db/10.f)) * 56.f;
        }
    }
    return 0.f;
}


const bool PLAY_MUSIC = false;


const char* paths[] = {
    "audios/tetris.wav",
    "C:\\Data\\Dev\\cpp\\music-to-notes\\audios\\tetris.wav",
    "/storage/9016-4EF8/Dev/c/audio2notes/audios/tetris.wav",

    "audios/super-mario-piano.wav",
    "C:\\Data\\Dev\\cpp\\music-to-notes\\audios\\super-mario-piano.wav",
    "/storage/9016-4EF8/Dev/c/audio2notes/audios/super-mario-piano.wav",

    "audios/bad-apple-3.wav",
    "C:\\Data\\Dev\\cpp\\music-to-notes\\audios\\bad-apple-3.wav",
    "/storage/9016-4EF8/Dev/c/audio2notes/audios/bad-apple-3.wav",

    "audios/lost-ones-weeping-piano.wav",
    "C:\\Data\\Dev\\cpp\\music-to-notes\\audios\\lost-ones-weeping-piano.wav",
    "/storage/9016-4EF8/Dev/c/audio2notes/audios/lost-ones-weeping-piano.wav",

    "audios/still-alive.wav",
    "C:\\Data\\Dev\\cpp\\music-to-notes\\audios\\still-alive.wav",
    "/storage/9016-4EF8/Dev/c/audio2notes/audios/still-alive.wav",

    "audios/arslan-senki-lapis-lazuli.wav",
    "C:\\Data\\Dev\\cpp\\music-to-notes\\audios\\arslan-senki-lapis-lazuli.wav",
    "/storage/9016-4EF8/Dev/c/audio2notes/audios/arslan-senki-lapis-lazuli.wav",

    "audios/440hz-sq.wav",
    "C:\\Data\\Dev\\cpp\\music-to-notes\\audios\\440hz-sq.wav",
    "/storage/9016-4EF8/Dev/c/audio2notes/audios/440hz-sq.wav",

    "audios/lost-ones-weeping.wav",
    "C:\\Data\\Dev\\cpp\\music-to-notes\\audios\\lost-ones-weeping.wav",
    "/storage/9016-4EF8/Dev/c/audio2notes/audios/lost-ones-weeping.wav",
};


const int NOTES_PER_SECOND = 10;


int main(int argc, char** args) {
    music_t music;
    int r = 1;
    for (int i = 0; r == 1 && i < 3; i++) {
        r = load_music_from_filename(paths[i], &music);
    }
    if (r) {
        fprintf(stderr, "Failed to load music file with error : %i\n", r);
        return 2;
    }
    printf("Successfully loaded music file\n");
    fflush(stdout);

    #ifndef MINIMAL

    Pa_Initialize();

    music_player_t mplayer;
    if (PLAY_MUSIC) {
        play_music(&mplayer, &music);
    }

    tone_player_t tplayer;
    open_tone_player(&tplayer);

    graph_t graph;
    graph_create(&graph, 960, 550);

    #endif

    const float maxf = 13289.750322558279;
    const int nfreqs = 108;
    const float length = music.rate / NOTES_PER_SECOND;

    for (int i = 0, m = music.length - length - 1; i < m; i += length) {
        clock_t start = clock();

        float* freqs = compute_sample_freqs_14(&music, i, length, maxf, nfreqs);

        float cfreq = maxf;
        for (int j = nfreqs - 1; j >= 0; j--) {
            freqs[j] = get_perceived_strength(cfreq, freqs[j]);
            cfreq /= NOTE_FREQ_RATIO;
        }

        float maxv = freqs[0];
        int maxi = 0;
        for (int j = 1; j < nfreqs; j++) {
            if (freqs[j] > maxv) {
                maxv = freqs[j];
                maxi = j;
            }
        }

        float freq = maxf / powf(NOTE_FREQ_RATIO, nfreqs - maxi - 1);
        printf("Frequency : %i; Value : %f\n", (int) freq, maxv);
        fflush(stdout);

        #ifndef MINIMAL
        if (maxv < 0.001f) freq = 0.f;
        play_tone(&tplayer, freq, 1.f);
        graph_render(&graph, freqs, nfreqs, 0.6f * 30.f); // / maxv);
        clock_t difference = clock() - start;
        int msec = difference * 1000 / CLOCKS_PER_SEC;
        int wait = (1000 / NOTES_PER_SECOND) - msec;
        if (wait > 0) Pa_Sleep(wait);
        #else
        int c; while((c = fgetc(stdin)) != '\n' && c != EOF);
        #endif

        free(freqs);
    }

    #ifndef MINIMAL
    close_tone_player(&tplayer);
    graph_destroy(&graph);
    if (PLAY_MUSIC)
        stop_music(&mplayer);
    Pa_Terminate();
    #endif

    return 0;
}
