#pragma once

// Play tones and musics

#include "audio.h"
#include "linked_list.h"

#include "portaudio.h"


class Player {
protected:
    Player();

public:
    size_t sample_rate;
    PaStream* stream;
};


class MusicPlayer : public Player {
private:
    Audio* audio;
    size_t cursor;

    static int pa_callback(
        const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData
    );

public:
    MusicPlayer();

    void play_audio(Audio* audio);
    void stop();

    void pause();
    void resume();
};


class TonesPlayer;


class ToneTrack {
public:
    float frequency;
    float volume;
    int32_t offset;
    float* sample;
    double start_time;
    double duration;
    size_t sample_len;
    LinkedListNode<ToneTrack*>* node;
    TonesPlayer* player;

    ToneTrack(TonesPlayer* player, LinkedListNode<ToneTrack*>* node, float frequency, float volume, double duration);

    void set_volume(float volume);
    void set_frequency(float frequency);
    void set_duration(double duration);
    void set_tone(float frequency, float volume);

    void update();
};


class TonesPlayer : public Player {
private:
    static int pa_callback(
        const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData
    );

public:
    LinkedList<ToneTrack*> tracks;

    TonesPlayer();

    ToneTrack* create_track(float frequency, float volume = 0.5, double duration = -1);
    void delete_track(ToneTrack* track);
};
