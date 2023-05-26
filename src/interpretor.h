#pragma once

// Analyze a histogram to extract notes


#include "extractor.h"


struct Note {
    float freq;
    float strength;
};


class Interpretor {
private:
    Histogram* histo;
    size_t current_histo_lenght;
    float* human_adjust_coefs;

    void check_histo();

    void gen_human_adjust_coefs();

public:
    Extractor* extractor;

    Interpretor();

    // Set extractor to use
    void set_extractor(Extractor* extractor);

    // Vary strength based on human perception of loudness for each frequencies
    void adjust_to_human_hear();

    // Lower strength of notes that don't make an harmony with a foundamental note
    void keep_harmony();

    // Keep peeks (lower frequencies at a flat level with other near frequencies)
    void keep_peeks();

    // Vary strength of notes based the acceleration of the speaker's membrane (lower very high frequencies)
    void adjust_to_speaker_physics();

    // Extract notes of the current histogram
    size_t extract_notes(Note* output, size_t maximum);
};
