#include "Application.h"
#include "GStreamerSupport.h"
#include "FPSCounter.h"
#include <iostream>
#include <unistd.h>

Application::Application() {}

Application::~Application()
{
    gstreamer_.finalize();
    renderer_.shutdown();
    platform_.shutdown();
}

bool Application::initialize()
{
    return gstreamer_.initialize() &&
           platform_.initialize() &&
           renderer_.initialize();
}

bool Application::run()
{
    if (!gstreamer_.startPipeline("sample.mp4"))
    {
        std::cerr << "Failed to start GStreamer pipeline." << std::endl;
        return false;
    }
    std::cout << "[App] Waiting for first frame..." << std::endl;

    GStreamerSupport::FrameData frame;
    int retries = 100;
    while (retries-- > 0)
    {
        if (gstreamer_.getFrameData(frame))
        {
            std::cout << "[App] First frame received." << std::endl;
            break;
        }
        usleep(1000); // 1ms
    }

    if (retries <= 0)
    {
        std::cerr << "[App] Timeout waiting for first frame." << std::endl;
        return false;
    }

    FPSCounter fpsCounter;
    int failCount = 0;

    for (int i = 0; i < 300; ++i)
    {
        GStreamerSupport::FrameData frame;
        if (gstreamer_.getFrameData(frame))
        {
            int ySize = frame.width * frame.height;
            int uSize = (frame.width / 2) * (frame.height / 2);

            renderer_.uploadYUVTextures(frame.data, frame.width, frame.height, ySize, uSize);
            renderer_.renderYUV(platform_.getScreenWidth(), platform_.getScreenHeight());
            platform_.swapBuffers();

            fpsCounter.frame();
            failCount = 0; // 成功したらリセット

            // ★★★ メモリリーク防止：必ず解放！
            gstreamer_.releaseFrame(frame);
        }
        else
        {
            if (++failCount % 10 == 0)
            {
                std::cerr << "[App] No frame yet (" << failCount << " fails)" << std::endl;
            }
            usleep(1000); // 1ms待機してリトライ
        }
    }
    std::cout << "Playback finished." << std::endl;
    return true;
}
