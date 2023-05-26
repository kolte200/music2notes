#include "extractor.h"

#include <math.h>

#include "utils.h"

#include <iostream>


struct NoteRange {
    float min;
    float mid;
    float max;
};


constexpr NoteRange NOTES[] = {
    {13.36f, 13.75f, 14.15f},
    {14.15f, 14.57f, 14.99f},
    {14.99f, 15.43f, 15.89f},
    {15.89f, 16.35f, 16.83f},
    {16.83f, 17.32f, 17.83f},
    {17.83f, 18.35f, 18.89f},
    {18.89f, 19.45f, 20.02f},
    {20.02f, 20.60f, 21.21f},
    {21.21f, 21.83f, 22.47f},
    {22.47f, 23.12f, 23.80f},
    {23.80f, 24.50f, 25.22f},
    {25.22f, 25.96f, 26.72f},
    {26.72f, 27.50f, 28.31f},
    {28.31f, 29.14f, 29.99f},
    {29.99f, 30.87f, 31.77f},
    {31.77f, 32.70f, 33.66f},
    {33.66f, 34.65f, 35.66f},
    {35.66f, 36.71f, 37.78f},
    {37.78f, 38.89f, 40.03f},
    {40.03f, 41.20f, 42.41f},
    {42.41f, 43.65f, 44.93f},
    {44.93f, 46.25f, 47.60f},
    {47.60f, 49.00f, 50.44f},
    {50.44f, 51.91f, 53.43f},
    {53.43f, 55.00f, 56.61f},
    {56.61f, 58.27f, 59.98f},
    {59.98f, 61.74f, 63.54f},
    {63.54f, 65.41f, 67.32f},
    {67.32f, 69.30f, 71.33f},
    {71.33f, 73.42f, 75.57f},
    {75.57f, 77.78f, 80.06f},
    {80.06f, 82.41f, 84.82f},
    {84.82f, 87.31f, 89.87f},
    {89.87f, 92.50f, 95.21f},
    {95.21f, 98.00f, 100.87f},
    {100.87f, 103.83f, 106.87f},
    {106.87f, 110.00f, 113.22f},
    {113.22f, 116.54f, 119.96f},
    {119.96f, 123.47f, 127.09f},
    {127.09f, 130.81f, 134.65f},
    {134.65f, 138.59f, 142.65f},
    {142.65f, 146.83f, 151.13f},
    {151.13f, 155.56f, 160.12f},
    {160.12f, 164.81f, 169.64f},
    {169.64f, 174.61f, 179.73f},
    {179.73f, 185.00f, 190.42f},
    {190.42f, 196.00f, 201.74f},
    {201.74f, 207.65f, 213.74f},
    {213.74f, 220.00f, 226.45f},
    {226.45f, 233.08f, 239.91f},
    {239.91f, 246.94f, 254.18f},
    {254.18f, 261.63f, 269.29f},
    {269.29f, 277.18f, 285.30f},
    {285.30f, 293.66f, 302.27f},
    {302.27f, 311.13f, 320.24f},
    {320.24f, 329.63f, 339.29f},
    {339.29f, 349.23f, 359.46f},
    {359.46f, 369.99f, 380.84f},
    {380.84f, 392.00f, 403.48f},
    {403.48f, 415.30f, 427.47f},
    {427.47f, 440.00f, 452.89f},
    {452.89f, 466.16f, 479.82f},
    {479.82f, 493.88f, 508.36f},
    {508.36f, 523.25f, 538.58f},
    {538.58f, 554.37f, 570.61f},
    {570.61f, 587.33f, 604.54f},
    {604.54f, 622.25f, 640.49f},
    {640.49f, 659.26f, 678.57f},
    {678.57f, 698.46f, 718.92f},
    {718.92f, 739.99f, 761.67f},
    {761.67f, 783.99f, 806.96f},
    {806.96f, 830.61f, 854.95f},
    {854.95f, 880.00f, 905.79f},
    {905.79f, 932.33f, 959.65f},
    {959.65f, 987.77f, 1016.71f},
    {1016.71f, 1046.50f, 1077.17f},
    {1077.17f, 1108.73f, 1141.22f},
    {1141.22f, 1174.66f, 1209.08f},
    {1209.08f, 1244.51f, 1280.97f},
    {1280.97f, 1318.51f, 1357.15f},
    {1357.15f, 1396.91f, 1437.85f},
    {1437.85f, 1479.98f, 1523.34f},
    {1523.34f, 1567.98f, 1613.93f},
    {1613.93f, 1661.22f, 1709.90f},
    {1709.90f, 1760.00f, 1811.57f},
    {1811.57f, 1864.66f, 1919.29f},
    {1919.29f, 1975.53f, 2033.42f},
    {2033.42f, 2093.00f, 2154.33f},
    {2154.33f, 2217.46f, 2282.44f},
    {2282.44f, 2349.32f, 2418.16f},
    {2418.16f, 2489.02f, 2561.95f},
    {2561.95f, 2637.02f, 2714.29f},
    {2714.29f, 2793.83f, 2875.69f},
    {2875.69f, 2959.96f, 3046.69f},
    {3046.69f, 3135.96f, 3227.85f},
    {3227.85f, 3322.44f, 3419.79f},
    {3419.79f, 3520.00f, 3623.14f},
    {3623.14f, 3729.31f, 3838.59f},
    {3838.59f, 3951.07f, 4066.84f},
    {4066.84f, 4186.01f, 4308.67f},
    {4308.67f, 4434.92f, 4564.88f},
    {4564.88f, 4698.64f, 4836.32f},
    {4836.32f, 4978.03f, 5123.90f},
    {5123.90f, 5274.04f, 5428.58f},
    {5428.58f, 5587.65f, 5751.38f},
    {5751.38f, 5919.91f, 6093.38f},
    {6093.38f, 6271.93f, 6455.71f},
    {6455.71f, 6644.88f, 6839.58f},
    {6839.58f, 7040.00f, 7246.29f},
    {7246.29f, 7458.62f, 7677.17f},
    {7677.17f, 7902.13f, 8133.68f},
    {8133.68f, 8372.02f, 8617.34f},
    {8617.34f, 8869.84f, 9129.75f},
    {9129.75f, 9397.27f, 9672.63f},
    {9672.63f, 9956.06f, 10247.80f},
    {10247.80f, 10548.08f, 10857.16f},
    {10857.16f, 11175.30f, 11502.76f},
    {11502.76f, 11839.82f, 12186.75f},
    {12186.75f, 12543.85f, 12911.42f},
    {12911.42f, 13289.75f, 13679.17f},
    {13679.17f, 14080.00f, 14492.58f},
    {14492.58f, 14917.24f, 15354.35f},
    {15354.35f, 15804.27f, 16267.37f},
    {16267.37f, 16744.04f, 17234.67f},
    {17234.67f, 17739.69f, 18259.50f},
    {18259.50f, 18794.55f, 19345.27f},
    {19345.27f, 19912.13f, 20495.60f},
    {20495.60f, 21096.16f, 21714.33f}
};

constexpr size_t NB_NOTES = sizeof(NOTES) / sizeof(NOTES[0]);



Histogram::Histogram(size_t nb_entries) {
    this->nb_entries = nb_entries;
    this->entries = (nb_entries > 0) ? new HistogramEntry[nb_entries] : nullptr;
}


Histogram::~Histogram() {
    delete[] this->entries;
}


void Histogram::resize(size_t nb_entries) {
    this->nb_entries = nb_entries;
    entries = (HistogramEntry*) realloc(entries, nb_entries * sizeof(HistogramEntry));
}


Extractor::Extractor() : histogram(0) {
    freqs = nullptr;
    nb_freqs = 0;
    cursor = 0;
    audio = nullptr;
    all_periods_sums = nullptr;
    all_periods_data = nullptr;
}

Extractor::~Extractor() {
    delete[] freqs;
    delete[] all_periods_sums;
    delete[] all_periods_data;
}


void Extractor::set_audio(Audio* audio) {
    this->audio = audio;
    jump(0);
    gen_audio_ranges();
}


void Extractor::forward(float duration) {
    cursor += duration * audio->rate;
    analyze_all();
}


void Extractor::jump(float time) {
    cursor = time * audio->rate;
    analyze_all();
}


float Extractor::get_cursor() const {
    return (float) cursor / (float) audio->rate;
}


void Extractor::set_freq_domain(float min_freq, float max_freq) {
    this->min_freq = min_freq;
    this->max_freq = max_freq;
    gen_audio_ranges();
}


void Extractor::set_window_width(float duration) {
    window_width = duration * audio->rate;
}


void Extractor::analyze_all() {
    const float* data = audio->get_data(0);
    for (size_t i = 0, m = nb_freqs; i < m; i++) {
        float r = analyze(&freqs[i], data);
        if (!isnan(r)) {
            histogram.entries[i].value = r;
        }
    }
}


void Extractor::gen_audio_ranges() {
    size_t start = 0;
    for (; NOTES[start].mid < min_freq && start < NB_NOTES; start++);

    size_t end = start;
    for (; end < NB_NOTES && NOTES[end].mid <= max_freq; end++);

    nb_freqs = end - start;
    freqs = (FrequencyData*) realloc(freqs, nb_freqs * sizeof(FrequencyData));

    histogram.resize(nb_freqs);

    const float rate = (float) audio->rate;

    size_t all_periods_sums_len = 0;
    size_t all_periods_data_len = 0;

    for (size_t i = start, j = 0; i < end; i++, j++) {
        const float period = rate / NOTES[i].mid;
        const int64_t period_i = (int64_t) (period + .5f);
        const int64_t nb_periods = window_width / period_i;
        all_periods_sums_len += period_i;
        all_periods_data_len += period_i * nb_periods;
        freqs[j].freq = NOTES[i].mid;
        freqs[j].min_freq = NOTES[i].min;
        freqs[j].max_freq = NOTES[i].max;
        freqs[j].period = period;
        freqs[j].period_int = period_i;
        freqs[j].min_period = rate / NOTES[i].min;
        freqs[j].max_period = rate / NOTES[i].max;
        freqs[j].nb_periods = nb_periods;
        freqs[j].cursor = -0x7FFFFFFF;
        freqs[j].start_cursor = -0x7FFFFFFF;
        freqs[j].end_cursor = -0x7FFFFFFF;
        freqs[j].total_shift = 0;
        freqs[j].period_phase = NAN;
    }

    all_periods_sums = (float*) realloc(all_periods_sums, all_periods_sums_len * sizeof(float));
    all_periods_data = (float*) realloc(all_periods_data, all_periods_data_len * sizeof(float));

    size_t periods_sums_offset = 0;
    size_t periods_data_offset = 0;
    for (size_t i = 0, m = nb_freqs; i < m; i++) {
        freqs[i].periods_sum  = &all_periods_sums[periods_sums_offset];
        freqs[i].periods_data = &all_periods_data[periods_data_offset];
        periods_sums_offset += freqs[i].period_int;
        periods_data_offset += freqs[i].period_int * freqs[i].nb_periods;
        histogram.entries[i].freq = freqs[i].freq;
        histogram.entries[i].value = 0.f;
    }
}


float Extractor::analyze(FrequencyData* range, const float* data) {
    const int64_t period_i = range->period_int;
    const int64_t audio_length = audio->length / period_i;
    const int64_t target_cursor = clamp<int64_t>(cursor / period_i, 0, audio_length - 1);
    const int64_t forward = target_cursor - range->cursor;
    if (forward == 0) return NAN;

    const float period = range->period;
    const float min_period = range->min_period;
    const float max_period = range->max_period;
    const int64_t half_period_i = (int64_t) (period * .5f + .5f);

    const float k = 2.f * (float) PI / (float) period_i;

    float sin_buff[period_i];
    for (size_t i = 0; i < period_i; i++)
        sin_buff[i] = sinf(k * i);

    float cos_buff[period_i];
    for (size_t i = 0; i < period_i; i++)
        cos_buff[i] = cosf(k * i);

    const float min_shift = period_i - max_period;
    const float max_shift = period_i - min_period;

    const int64_t nb_periods = range->nb_periods;
    float* periods_sum  = range->periods_sum;
    float* periods_data = range->periods_data;

    float avgs[period_i];

    float current_phase = range->period_phase;
    float current_shift = range->total_shift;

    const int64_t nb_left_periods = nb_periods / 2;
    const int64_t nb_right_periods = nb_periods - nb_left_periods - 1;

    const int64_t expected_cursor = range->cursor + forward;
    const int64_t expected_start_cursor = max<int64_t>(expected_cursor - nb_left_periods, 0);
    const int64_t expected_end_cursor = min<int64_t>(expected_cursor + nb_right_periods, audio_length - 1);

    int64_t add_start_cursor, add_end_cursor;
    int64_t rem_start_cursor, rem_end_cursor;
    int64_t add_start_index, rem_start_index;

    const int64_t periods_data_offset = range->periods_data_offset;

    if (forward < 0) {
        add_start_cursor = expected_start_cursor;
        add_end_cursor   = range->start_cursor - 1;
        rem_start_cursor = expected_end_cursor + 1;
        rem_end_cursor   = range->end_cursor;
        add_start_index  = mod(periods_data_offset - nb_left_periods - (add_end_cursor - add_start_cursor + 1), nb_periods);
        rem_start_index  = mod(periods_data_offset + nb_right_periods + 1 + forward, nb_periods);
    } else {
        add_start_cursor = range->end_cursor + 1;
        add_end_cursor   = expected_end_cursor;
        rem_start_cursor = range->start_cursor;
        rem_end_cursor   = expected_start_cursor - 1;
        add_start_index  = mod(periods_data_offset + nb_right_periods + 1, nb_periods);
        rem_start_index  = mod(periods_data_offset - nb_left_periods, nb_periods);
    }

    // Remove
    if (abs(forward) > nb_periods / 2) {
        for (int64_t i = 0; i < period_i; i++) {
            periods_sum[i] = 0.f;
        }
        add_start_cursor = expected_start_cursor;
        add_end_cursor = expected_end_cursor;
        add_start_index = mod(-nb_left_periods, nb_periods);
        range->periods_data_offset = 0;
        current_phase = NAN;
        current_shift = 0;
    } else {
        int64_t nb_rem_periods = rem_end_cursor - rem_start_cursor + 1;
        for (int64_t i = 0; i < nb_rem_periods; i++) {
            int64_t offset = mod(rem_start_index + i, nb_periods) * period_i;
            for (int64_t j = 0; j < period_i; j++) {
                periods_sum[j] -= periods_data[offset + j];
            }
        }
        range->periods_data_offset = mod(periods_data_offset + forward, nb_periods);
    }

    // Initial average
    float avg = 0.f;
    for (int64_t i = 0; i < period_i; i++) avg += data[i];
    avg /= period_i;

    int64_t period_data_index = add_start_index;
    for (int64_t i = add_start_cursor; i <= add_end_cursor; i++) {
        int64_t offset = i * period_i + half_period_i;

        // Compute new averages

        for (int64_t j = 0; j < period_i; j++) {
            const int index = offset + j;
            avgs[j] = avg;
            avg += (data[index + half_period_i] - data[index - half_period_i]) / period_i;
        }

        // Compute phase to shift if needed

        float x = 0, y = 0;
        for (int64_t j = 0; j < period_i; j++) {
            float v = data[offset + j] - avgs[j];
            x += cos_buff[j] * v;
            y += sin_buff[j] * v;
        }
        const float phase = mod(atan2f(y, x) * period / (2.f * (float) PI), period);

        // Compute needed shift
        if (isnan(current_phase)) current_phase = phase;
        float delta_shift = clamp(cycle(phase + current_shift, current_phase, period), min_shift, max_shift);
        current_shift += delta_shift;

        // Add to sum with shift
        const int64_t shift_i = 0;//mod((int64_t) current_shift, period_i);

        const int64_t d = period_data_index * period_i;
        for (int64_t j = 0; j < period_i; j++) {
            const float v = data[offset + j] - avgs[j];
            int64_t index = (j + shift_i) % period_i;
            periods_data[d + index] = v;
            periods_sum[index] += v;
        }

        period_data_index = mod(period_data_index + 1, nb_periods);
    }

    avg = 0.f;
    for (int64_t i = 0; i < period_i; i++) avg += periods_sum[i];
    avg /= period_i;

    float v = periods_sum[0] - avg;
    float minv = v;
    float maxv = v;

    for (int64_t i = 1; i < period_i; i++) {
        v += periods_sum[i] - avg;
        if (v < minv) minv = v;
        if (v > maxv) maxv = v;
    }

    // Update range values
    range->cursor = expected_cursor;
    range->start_cursor = expected_start_cursor;
    range->end_cursor = expected_end_cursor;
    range->total_shift = current_shift;
    range->period_phase = current_phase;

    return (maxv - minv) / (float) ((expected_end_cursor - expected_start_cursor + 1) * period_i) * 2.f;
}
