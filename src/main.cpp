#include <iostream>

#include "utils.h"
#include "audio.h"
#include "player.h"
#include "graphic.h"
#include "extractor.h"
#include "interpretor.h"


constexpr char audio_filename[] = "audios/tetris.wav";

constexpr int nb_notes = 4; // Maximum number of simultaneous notes
constexpr int nps = 8; // Notes per seconds
constexpr int fps = 12; // Frame per seconds


extern "C" int main(int argc, char** argv) {
    std::cout << "====== Music to Notes ======" << std::endl << std::endl;

    Audio audio;
    Graphic graphic;
    Extractor extractor;
    Interpretor interpretor;
    MusicPlayer music_player;
    TonesPlayer tones_player;

    // Load audio file in memory
    std::cout << "Loading audio ..." << std::endl;
    audio.load_wav_file(audio_filename);
    std::cout << "Audio loaded" << std::endl << std::endl;

    // Configure extractor
    extractor.set_audio(&audio);
    extractor.set_window_width(1.f / (float) nps);
    extractor.set_freq_domain(20, 5000);

    // Configure interpretor
    interpretor.set_extractor(&extractor);

    // Create window
    graphic.create(800, 650);

    // Play audio file
    std::cout << "Start playing" << std::endl;
    //music_player.play_audio(&audio);

    Note notes[nb_notes] {};
    ToneTrack* tracks[nb_notes];

    for (int i = 0; i < nb_notes; i++) {
        tracks[i] = tones_player.create_track(0);
    }

    // Main loop
    int64_t last_time = millis();
    while (!graphic.update()) {
        // Find top notes (from the histogram of the extractor)
        interpretor.adjust_to_human_hear();
        //interpretor.keep_harmony();
        int n = interpretor.extract_notes(notes, nb_notes);

        // Draw current histogram
        graphic.clear(Color(0x000000));
        graphic.render_histogram(&extractor.histogram, Color(0xFFFFFF), 8);

        // Play notes
        for (int i = 0; i < nb_notes; i++) {
            const float freq = i < n ? notes[i].freq : 0;
            std::cout << "Note " << i << " : " << freq << std::endl;
            tracks[i]->set_tone(freq, min(notes[i].strength * 20.f, 0.8f));
        }

        // Forward and update histogram
        extractor.forward(1.f / (float) fps);

        int64_t current_time = millis();
        int64_t wait_time = (1000 / fps) - (current_time - last_time);
        last_time = current_time;
        if (wait_time > 0) sleep(wait_time);
    }

    std::cout << "Stop playing" << std::endl;

    // Stop audio
    //music_player.stop();

    return EXIT_SUCCESS;
}
