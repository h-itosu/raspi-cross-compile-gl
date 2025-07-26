#include "Application.h"
#include "GStreamerSupport.h"
#include "FPSCounter.h"
#include "TelopRenderer.h"
#include "Util.h"
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

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
    if (!gstreamer_.initialize())
    {
        std::cerr << "Failed to initialize GStreamerSupport." << std::endl;
        return false;
    }
    if (!platform_.initialize())
    {
        std::cerr << "Failed to initialize GraphicsPlatform." << std::endl;
        return false;
    }
    if (!renderer_.initialize())
    {
        std::cerr << "Failed to initialize Renderer." << std::endl;
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
    telopRenderer.setOutlineWidth(2.0f);                   // ピクセル単位の太さ

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
    // 【修正点】最初のフレーム取得のタイムアウトを少し長くします
    int retries = 300; // 3秒程度待つ
    while (retries-- > 0)
    {
        if (gstreamer_.getFrameData(frame))
        {
            std::cout << "[App] First frame received." << std::endl;
            break;
        }
        usleep(10000); // 10ms待機
    }

    if (retries <= 0)
    {
        std::cerr << "[App] Timeout waiting for first frame." << std::endl;
        return false;
    }

    FPSCounter fpsCounter;
    bool isScreenshotMode = false;
    // --- 【ここから修正】 ---
    // GStreamer側で同期を行うため、手動でのフレームレート制限ロジックは不要になります。
    // ループはGStreamerからのフレーム供給に合わせて、可能な限り速く回します。
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
            else if (ch == 's' || ch == 'S') // Sキーでスクリーンショット
            {
                isScreenshotMode = true;
            }
        }

        if (!gstreamer_.checkBusMessages())
        {
            std::cerr << "[App] Error checking GStreamer bus messages." << std::endl;
            break;
        }

        // gstreamer_.getFrameData()がGStreamerの再生速度に合わせてブロックし、
        // タイミングを制御するようになります。
        if (gstreamer_.getFrameData(frame))
        {
            int ySize = frame.width * frame.height;
            int uSize = (frame.width / 2) * (frame.height / 2);

            renderer_.uploadYUVTextures(frame.data, frame.width, frame.height, ySize, uSize);
            renderer_.renderYUV(platform_.getScreenWidth(), platform_.getScreenHeight());

            telopRenderer.update();
            telopRenderer.render();

            platform_.swapBuffers();
            if (isScreenshotMode)
            {
                // 時刻を取得してファイル名を作成
                auto t = std::time(nullptr);
                std::stringstream ss;
                ss << "ScreenShot_" << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".png";

                // 保存処理
                std::cout << "[App] Saving screenshot to " << ss.str() << std::endl;
                platform_.saveFramebufferToPNG(ss.str().c_str());
                isScreenshotMode = false; // スクリーンショットモードをリセット
            }

            if (fpsCounter.frame())
            {
                // 2. 残りメモリの取得と表示
                util::LogAvailableMemory();
                timer.LogElapsedTimeHMS();
            }

            gstreamer_.releaseFrame(frame);
        }
        else
        {
            // 新しいフレームがまだない場合、少し待機する
            usleep(1000);
        }
    }

    std::cout << "Playback finished." << std::endl;
    return true;
}
