#pragma once

// Load audio from file


#include <string>


class Audio {
private:
    float** data; // Per channel data
    float* all_data;

public:
    int64_t length;
    int32_t rate;
    int32_t nb_channels;

    bool load_wav_file(std::string filename);

    // Do the mean of all channel and put the result on the first channel
    void convert_to_monochannel();

    inline const float* get_data(int channel) { return data[channel]; }
};
