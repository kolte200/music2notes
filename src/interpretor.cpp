#include "interpretor.h"

#include "sort.h"
#include "utils.h"

#include <math.h>


/*
TODO :
    - Local minimum threshold based on the average amplitude of last x seconds
    - Undo average on last histogrames for each frequency based on window width
    - Minimum note duration and minimum note cut duration
*/


struct HistogramEntryRef {
    float strength;
    uint32_t index;

    // Operator override needed to perform sorting
    inline bool operator>(const HistogramEntryRef& other) {
        return strength > other.strength;
    }
};


Interpretor::Interpretor() {
    histo = nullptr;
    current_histo_lenght = 0;
    human_adjust_coefs = nullptr;
}


void Interpretor::check_histo() {
    if (current_histo_lenght != histo->nb_entries) {
        gen_human_adjust_coefs();
    }
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


static float get_human_adjust_coef(float freq) {
    const int n = sizeof(HEAR_STRENGTH_PER_FREQ) / sizeof(HEAR_STRENGTH_PER_FREQ[0]);
    for (int i = 2; i < n; i += 2) {
        float a = HEAR_STRENGTH_PER_FREQ[i];
        if (freq < a) {
            float b = HEAR_STRENGTH_PER_FREQ[i-2];
            float c = HEAR_STRENGTH_PER_FREQ[i-1];
            float d = HEAR_STRENGTH_PER_FREQ[i+1];
            float db = (freq - a) / (b - a) * (d - c) + c;
            return 56.f / sqrtf(powf(10.f, db/10.f));
        }
    }
    return 0.f;
}


void Interpretor::gen_human_adjust_coefs() {
    const size_t nb_entries = histo->nb_entries;
    HistogramEntry* entries = histo->entries;
    human_adjust_coefs = (float*) realloc(human_adjust_coefs, nb_entries * sizeof(float));
    for (size_t i = 0; i < nb_entries; i++) {
        human_adjust_coefs[i] = get_human_adjust_coef(entries[i].freq);
    }
}


void Interpretor::set_extractor(Extractor* extractor) {
    this->extractor = extractor;
    histo = &extractor->histogram;
    check_histo();
}


void Interpretor::adjust_to_human_hear() {
    check_histo();
    const size_t nb_entries = histo->nb_entries;
    HistogramEntry* entries = histo->entries;
    for (size_t i = 0; i < nb_entries; i++) {
        entries[i].value *= human_adjust_coefs[i];
    }
}


static inline void lower_unharmony(HistogramEntry* entries, size_t index, bool* lowereds, size_t nb_entries) {
    lowereds[index] = true;
    for (size_t i = max<size_t>(0, index - 3); i < index; i++) {
        entries[i].value *= .5f;
        lowereds[i] = true;
    }
    for (size_t i = index + 1, m = min<size_t>(nb_entries, index + 3); i <= m; i++) {
        entries[i].value *= .5f;
        lowereds[i] = true;
    }
}


void Interpretor::keep_harmony() {
    check_histo();

    const size_t nb_entries = histo->nb_entries;
    HistogramEntry* entries = histo->entries;

    HistogramEntryRef* sorted = new HistogramEntryRef[nb_entries];
    for (size_t i = 0; i < nb_entries; i++) {
        sorted[i].index = i;
        sorted[i].strength = entries[i].value;
    }

    sort(sorted, nb_entries);

    bool* lowereds = new bool[nb_entries] {};

    for (size_t i = nb_entries; i-- > 0;) {
        const size_t index = sorted[i].index;
        if (!lowereds[index])
            lower_unharmony(entries, index, lowereds, nb_entries);
    }

    delete[] lowereds;
    delete[] sorted;
}


void Interpretor::keep_peeks() {
    check_histo();
    // TODO
}


void Interpretor::adjust_to_speaker_physics() {
    check_histo();
    // TODO
}


size_t Interpretor::extract_notes(Note* output, size_t maximum) {
    check_histo();

    const size_t nb_entries = histo->nb_entries;
    HistogramEntry* entries = histo->entries;

    HistogramEntryRef* sorted = new HistogramEntryRef[nb_entries];
    for (size_t i = 0; i < nb_entries; i++) {
        sorted[i].index = i;
        sorted[i].strength = entries[i].value;
    }

    sort(sorted, nb_entries);

    float best_threshold = 0.f;

    // Find the best threshold

/*
    float best_score = 0.f;
    for (size_t i = 0, m = nb_entries - 1; i < m; i++) {
        const float delta = sorted[i + 1].strength - sorted[i].strength;
        if (delta > best_score) {
            best_score = delta;
            best_threshold = sorted[i].strength + delta * .5f;
        }
    }
*/

    float variance = 0.f;
    best_threshold = sorted[0].strength;
    for (size_t i = 1; i < nb_entries; i++) {
        best_threshold += sorted[i].strength;
        variance += abs(sorted[i].strength - sorted[i - 1].strength);
    }
    best_threshold /= nb_entries;
    best_threshold += variance / (nb_entries - 1);

/*
    float best_score = 0.f;

    float right_avg = 0.f;
    for (size_t i = 0; i < nb_entries; i++)
        right_avg += sorted[i].strength;
    right_avg /= nb_entries;

    float left_avg = 0.f;

    for (size_t i = 0, m = nb_entries - 1; i < m; i++) {
        const float nb_right_values = (float) (nb_entries - i);
        const float k1 = 1.f / (nb_right_values - 1.f);
        right_avg = right_avg * (nb_right_values * k1) - sorted[i].strength * k1;

        const float nb_left_values = (float) (i);
        const float k2 = 1.f / (nb_left_values + 1.f);
        left_avg  = left_avg  * (nb_right_values * k2) + sorted[i].strength * k2;

        const float score = right_avg - left_avg;
        if (score > best_score) {
            best_score = score;
            best_threshold = (left_avg + right_avg) * .5f;
        }
    }
*/

/*
    float best_score = INFINITY;

    float right_avg = 0.f;
    float right_weight = 0.f;
    for (size_t i = 0; i < nb_entries; i++) {
        right_avg += i * sorted[i].strength;
        right_weight += sorted[i].strength;
    }
    right_avg /= right_weight;

    float left_avg = 0.f;
    float left_weight = 0.f;

    for (size_t i = 0, m = nb_entries - 1; i < m; i++) {
        right_avg = right_avg * right_weight - i * sorted[i].strength;
        right_weight -= sorted[i].strength;
        if (right_weight > 0) right_avg /= right_weight;
        else right_avg = nb_entries - 1;

        left_avg = left_avg * left_weight + i * sorted[i].strength;
        left_weight += sorted[i].strength;
        if (left_weight > 0) left_avg /= left_weight;
        else left_avg = 0;

        const float score = abs((right_avg - i) - (i - left_avg));
        if (score < best_score) {
            best_score = score;
            best_threshold = entries[i].value;
        }
    }
*/

    size_t n = 0;
    for (size_t i = nb_entries; n < maximum && i-- > 0;) {
        const size_t index = sorted[i].index;
        if (entries[index].value > best_threshold) {
            output[n].freq     = entries[index].freq;
            const float k = human_adjust_coefs[index];
            output[n].strength = entries[index].value / (k*k);
            n++;
        }
    }

    delete[] sorted;

    return n;
}
