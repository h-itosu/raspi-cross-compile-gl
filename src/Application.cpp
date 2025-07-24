#include "Application.h"
#include "GStreamerSupport.h"
#include "FPSCounter.h"
#include "TelopRenderer.h"
#include "Util.h"
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

Application::Application() {}

Application::~Application()
{
    gstreamer_.finalize();
    renderer_.shutdown();
    platform_.shutdown();
}

TelopRenderer telopRenderer;

bool Application::initialize()
{
    if (!gstreamer_.initialize() ||
        !platform_.initialize() ||
        !renderer_.initialize())
    {
        return false;
    }

    if (!telopRenderer.initialize("/usr/share/fonts/truetype/vlgothic/VL-Gothic-Regular.ttf"))
    {
        std::cerr << "Failed to initialize TelopRenderer" << std::endl;
        return false;
    }

    // アウトライン描画を有効化（シェーダー側対応）
    telopRenderer.setOutline(true);                        // フラグのみセット、描画はシェーダーで対応
    telopRenderer.setOutlineColor(0.0f, 0.0f, 0.0f, 1.0f); // 黒色アウトライン
    telopRenderer.setOutlineWidth(0.05f);                  // ピクセル単位の太さ

    return true;
}

static bool kbhit()
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return true;
    }

    return false;
}

bool Application::run()
{
    // coutにシステムのロケールを設定して3桁区切りを有効にする
    std::cout.imbue(std::locale(""));

    util timer;
    timer.StartTimer();

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
        usleep(1000);
    }

    if (retries <= 0)
    {
        std::cerr << "[App] Timeout waiting for first frame." << std::endl;
        return false;
    }

    FPSCounter fpsCounter;
    int failCount = 0;

    while (true)
    {
        if (kbhit())
        {
            int ch = getchar();
            if (ch == 27) // ESC key
            {
                std::cout << "ESC pressed. Exiting..." << std::endl;
                break;
            }
        }

        if (!gstreamer_.checkBusMessages())
        {
            std::cerr << "[App] Error checking GStreamer bus messages." << std::endl;
            break;
        }

        if (gstreamer_.getFrameData(frame))
        {
            int ySize = frame.width * frame.height;
            int uSize = (frame.width / 2) * (frame.height / 2);

            renderer_.uploadYUVTextures(frame.data, frame.width, frame.height, ySize, uSize);
            renderer_.renderYUV(platform_.getScreenWidth(), platform_.getScreenHeight());

            telopRenderer.update();
            telopRenderer.render();

            platform_.swapBuffers();

            if (fpsCounter.frame())
            {
                // 2. 残りメモリの取得と表示
                util::LogAvailableMemory();
                timer.LogElapsedTimeHMS();
            }
            failCount = 0;

            gstreamer_.releaseFrame(frame);
        }
        else
        {
            /*
            if (++failCount > 60)
            {
                std::cout << "End of stream detected. Restarting pipeline..." << std::endl;
                gstreamer_.restartPipeline("sample.mp4");
                failCount = 0;
            }
            else*/
            if (failCount % 10 == 0)
            {
                std::cerr << "[App] No frame yet (" << failCount << " fails)" << std::endl;
            }
            usleep(1000);
        }
    }

    std::cout << "Playback finished." << std::endl;
    return true;
}
