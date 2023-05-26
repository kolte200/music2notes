#pragma once

// Extract frequencies from a sound


#include <stdint.h>

#include "audio.h"


#define NOTE_FREQ_RATIO (1.0594630943592953) // 2**(1/12)
#define HALF_NOTE_FREQ_RATIO (1.029302236643492) // 2**(1/24)


struct HistogramEntry {
    float freq;
    float value;
};


struct FrequencyData {
    int64_t cursor;
    int64_t start_cursor;
    int64_t end_cursor;
    float* periods_sum;
    float* periods_data;
    float freq;
    float min_freq;
    float max_freq;
    float period;
    float min_period;
    float max_period;
    int32_t nb_periods;
    int32_t periods_data_offset;
    int32_t period_int;
    float total_shift;
    float period_phase;
};


class Histogram {
public:
    Histogram(size_t nb_entries);
    ~Histogram();

    size_t nb_entries;
    HistogramEntry* entries;

    void resize(size_t nb_entries);
};


class Extractor {
private:
    Audio* audio;
    FrequencyData* freqs;
    size_t nb_freqs;
    float min_freq, max_freq;
    int64_t window_width;
    float* all_periods_sums;
    float* all_periods_data;
    int64_t cursor;

    float analyze(FrequencyData* range, const float* data);
    void analyze_all();
    void gen_audio_ranges();

public:
    Histogram histogram;

    Extractor();
    ~Extractor();

    // Set audio to analyze
    void set_audio(Audio* audio);

    // Forward analyze by duration
    void forward(float duration);

    // Set begin of analyze window offset in second from the begining of the audio
    void jump(float time);

    // Get current begin of analyze window offset in seconds from the begining of the audio
    float get_cursor() const;

    inline float get_min_freq() const { return min_freq; }
    inline float get_max_freq() const { return max_freq; }

    void set_freq_domain(float min_freq, float max_freq);
    void set_window_width(float duration);
};
