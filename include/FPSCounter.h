#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <chrono>
#include <iostream>

class FPSCounter
{
public:
    FPSCounter() : frameCount_(0), lastTime_(std::chrono::steady_clock::now()) {}

    void frame()
    {
        frameCount_++;
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime_).count();
        if (duration >= 1000)
        {
            std::cout << "[FPS] " << frameCount_ << " fps" << std::endl;
            frameCount_ = 0;
            lastTime_ = now;
        }
    }

private:
    int frameCount_;
    std::chrono::steady_clock::time_point lastTime_;
};

#endif // FPS_COUNTER_H
