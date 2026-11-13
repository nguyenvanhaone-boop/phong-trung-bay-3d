#pragma once

#include <chrono>

class FrameCounter {
public:
    using Clock = std::chrono::steady_clock;

    explicit FrameCounter(Clock::time_point start = Clock::now()) : started(start) {}

    void record(Clock::time_point now = Clock::now()) {
        ++frames;
        const double seconds = std::chrono::duration<double>(now - started).count();
        if (seconds >= 0.5) {
            value = frames / seconds;
            frames = 0;
            started = now;
        }
    }

    double fps() const { return value; }

private:
    Clock::time_point started;
    int frames = 0;
    double value = 0;
};
