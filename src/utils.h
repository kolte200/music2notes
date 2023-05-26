#pragma once

// Useful functions

#include <concepts>
#include <stdint.h>
#include <math.h>


#define PI (3.14159265f)


inline uint16_t ltohs(const uint16_t v) {
    const uint8_t* x = (uint8_t*) &v;
    return (x[1] << 8) | x[0];
}

inline uint32_t ltohi(const uint32_t v) {
    const uint8_t* x = (uint8_t*) &v;
    return  (x[3] << 24) | (x[2] << 16) | (x[1] << 8) | x[0];
}

inline int16_t utoss(const uint16_t v) {
    return *((int16_t*)&v);
}


template<typename T>
inline T min(const T a, const T b) {
    return a > b ? b : a;
}

template<typename T>
inline T max(const T a, const T b) {
    return a > b ? a : b;
}

template<typename T>
inline T abs(const T a) {
    return a > 0 ? a : -a;
}

template<typename T>
inline T clamp(const T a, const T minv, const T maxv) {
    return min<T>(max<T>(a, minv), maxv);
}

template<typename T>
requires std::integral<T> || std::same_as<T, float> || std::same_as<T, double>
inline T mod(const T a, const T b) {
    const T r = a % b;
    return (r < 0) ? (r + b) : r;
}

template<>
inline float mod(const float a, const float b) {
    const float r = fmodf(a, b);
    return (r < 0) ? (r + b) : r;
}

template<>
inline double mod(const double a, const double b) {
    const double r = fmod(a, b);
    return (r < 0) ? (r + b) : r;
}

template<typename T>
inline T cycle(const T from, const T to, const T modulus) {
    const T s = modulus / ((T) 2);
    const T r = mod<T>(to - from + s, modulus);
    return r - s;
}


#include <chrono>

static inline uint64_t millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
static inline void sleep(uint32_t duration) {
    Sleep(duration);
}

#else

#include <unistd.h>
static inline void sleep(uint32_t duration) {
    usleep(duration * 1000);
}

#endif
