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
        std::chrono::duration<double> elapsed = now - lastTime_;
        if (elapsed.count() >= 1.0)
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
