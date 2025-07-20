/**
 * @file Application.cpp
 * @brief アプリケーション全体を管理するクラスの実装（YUV対応）
 */
#include "Application.h"
#include "GStreamerSupport.h"
#include <iostream>
#include <unistd.h> // usleep

#include "FPSCounter.h"

Application::Application() {}

Application::~Application()
{
    gstreamer_.finalize();
    renderer_.shutdown();
    platform_.shutdown();
}

bool Application::initialize()
{
    if (!gstreamer_.initialize())
        return false;
    if (!platform_.initialize())
        return false;
    if (!renderer_.initialize())
        return false;
    return true;
}

void Application::run()
{
    if (!gstreamer_.startPipeline("sample.mp4"))
    {
        std::cerr << "Failed to start GStreamer pipeline." << std::endl;
        return;
    }

    FPSCounter fpsCounter;

    for (int i = 0; i < 300; ++i) // 約5秒再生（60fps換算）
    {
        GStreamerSupport::FrameData frame;
        if (gstreamer_.getFrameData(frame))
        {
            int ySize = frame.width * frame.height;
            int uSize = (frame.width / 2) * (frame.height / 2);
            renderer_.uploadYUVTextures(frame.buffer.data(), frame.width, frame.height, ySize, uSize);

            renderer_.renderYUV(platform_.getScreenWidth(), platform_.getScreenHeight());
            platform_.swapBuffers();
        }

        //        usleep(16000);      // 約60fps想定
        fpsCounter.frame(); // FPSカウンターを更新
    }

    std::cout << "Playback finished." << std::endl;
}
