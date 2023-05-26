#include "player.h"

#include <portaudio.h>
#include <math.h>
#include "utils.h"



#define SAMPLE_RATE (44100)


static bool pa_initialized = false;


// TODO : Pa_Terminate() called nowhere


Player::Player() {
    PaError err;

    if (!pa_initialized) {
        err = Pa_Initialize();
        if( err != paNoError ) {
            // TODO
        } else {
            pa_initialized = true;
        }
    }

    if (!pa_initialized) return;

    sample_rate = SAMPLE_RATE;
}




MusicPlayer::MusicPlayer() {
    cursor = 0;
    audio = nullptr;
}

void MusicPlayer::play_audio(Audio* audio) {
    this->audio = audio;

    PaError err = Pa_OpenDefaultStream(
        &stream,
        0, /* no input channels */
        audio->nb_channels,
        paFloat32,
        audio->rate,
        paFramesPerBufferUnspecified,
        MusicPlayer::pa_callback, /* this is your callback function */
        this /*This is a pointer that will be passed to your callback*/
    );

    if( err != paNoError ) {
        // TODO
    }

    err = Pa_StartStream(stream);

    if (err != paNoError) {
        // TODO
    }
}


void MusicPlayer::stop() {
    PaError err;

    err = Pa_StopStream(stream);

    if (err != paNoError) {
        // TODO
    }

    err = Pa_CloseStream(&stream);

    if (err != paNoError) {
        // TODO
    }
}


int MusicPlayer::pa_callback(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
) {
    MusicPlayer* const player = (MusicPlayer*) userData;
    if (!player->audio) return paAbort;

    float* const out = (float*) outputBuffer;
    const size_t nbchnls = player->audio->nb_channels;
    const size_t cursor = player->cursor;

    for (size_t i = 0; i < nbchnls; i++) {
        const float* data = player->audio->get_data(i);
        for (size_t j = 0, m = min<size_t>(framesPerBuffer, player->audio->length - cursor); j < m; j++) {
            out[j * nbchnls + i] = data[j + cursor];
        }
    }

    player->cursor = cursor + framesPerBuffer;

    return paContinue;
}




ToneTrack::ToneTrack(
    TonesPlayer* player,
    LinkedListNode<ToneTrack*>* node,
    float frequency,
    float volume,
    double duration
) {
    this->frequency = frequency;
    this->volume = volume;
    this->node = node;
    this->player = player;
    this->sample = nullptr;
    this->sample_len = 0;
    this->offset = 0;
    set_duration(duration);
    update();
}


void ToneTrack::set_volume(float volume) {
    this->volume = volume;
    update();
}


void ToneTrack::set_frequency(float frequency) {
    this->frequency = frequency;
    update();
}


void ToneTrack::set_tone(float frequency, float volume) {
    this->frequency = frequency;
    this->volume = volume;
    update();
}


void ToneTrack::set_duration(double duration) {
    this->duration = duration;
    this->start_time = Pa_GetStreamTime(player->stream);
}


void ToneTrack::update() {
    size_t old_sample_len = sample_len;
    if (frequency > 10) {
        sample_len = (size_t) (player->sample_rate / frequency + .5f);
        sample = (float*) realloc(sample, sample_len * sizeof(float));
        for (size_t i = 0; i < sample_len; i++) {
            sample[i] = sin(PI * 2 * i / sample_len) * volume;
        }
    } else {
        sample_len = 0;
    }
    offset = old_sample_len > 0 ? offset * sample_len / old_sample_len : 0;
}




TonesPlayer::TonesPlayer() {
    PaError err;

    err = Pa_OpenDefaultStream(
        &stream,
        0, /* no input channels */
        1, /* mono output */
        paFloat32,
        sample_rate,
        paFramesPerBufferUnspecified,
        TonesPlayer::pa_callback, /* this is your callback function */
        this /*This is a pointer that will be passed to your callback*/
    );

    if(err != paNoError) {
        // TODO
    }

    err = Pa_StartStream(stream);

    if (err != paNoError) {
        // TODO
    }
}


ToneTrack* TonesPlayer::create_track(float frequency, float volume, double duration) {
    LinkedListNode<ToneTrack*>* node = new LinkedListNode<ToneTrack*>();
    node->data = new ToneTrack(this, node, frequency, volume, duration);
    tracks.add(node);
    return node->data;
}


void TonesPlayer::delete_track(ToneTrack* track) {
    tracks.remove(track->node);
    delete track->node;
    delete track;
}


int TonesPlayer::pa_callback(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
) {
    TonesPlayer* const player = (TonesPlayer*) userData;
    float* const out = (float*) outputBuffer;

    for (size_t i = 0; i < framesPerBuffer; i++)
        out[i] = 0;

    float n = 0;

    LinkedListNode<ToneTrack*>* node = player->tracks.first();
    for (; node; node = node->next) {
        ToneTrack* track = node->data;
        const size_t sample_len = track->sample_len;
        if (sample_len <= 0) continue;
        float* const sample = track->sample;
        size_t offset = track->offset;
        for (size_t j = 0; j < framesPerBuffer; j++) {
            if (offset >= sample_len) offset = 0;
            out[j] += sample[offset];
            offset++;
        }
        track->offset = offset;
        if (track->duration >= 0 && timeInfo->currentTime - track->start_time >= track->duration)
            player->delete_track(track);
        n++;
    }

    n = 1.f / n;
    for (size_t i = 0; i < framesPerBuffer; i++)
        out[i] *= n;

    return paContinue;
}
