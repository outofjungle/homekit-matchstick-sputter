#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

// Arduino boolean type
typedef uint8_t boolean;

// Controllable millis() for tests — set _stub_millis before calling update()
inline unsigned long _stub_millis = 0;
inline unsigned long millis() { return _stub_millis; }

// Minimal Serial stub
struct _SerialClass {
    void printf(const char* fmt, ...) { (void)fmt; }
    void print(const char*) {}
    void println(const char*) {}
};
inline _SerialClass Serial;

// Arduino-like random functions
inline long random(long max) { return std::rand() % max; }
inline long random(long min, long max) { return min + (std::rand() % (max - min)); }

// Arduino map function
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Arduino constrain function (overloaded for different types)
template<typename T>
inline T constrain(T x, T low, T high) {
    return (x < low) ? low : (x > high) ? high : x;
}

// Specific overload for mixed int types
inline int8_t constrain(int8_t x, int low, int high) {
    return (x < low) ? low : (x > high) ? high : x;
}

// FastLED-like saturating add/subtract
inline uint8_t qadd8(uint8_t a, uint8_t b) {
    return (a + b > 255) ? 255 : a + b;
}

inline uint8_t qsub8(uint8_t a, uint8_t b) {
    return (a > b) ? a - b : 0;
}
