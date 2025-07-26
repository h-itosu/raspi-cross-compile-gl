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
#include <vector>

Application::Application() {}

Application::~Application()
{
    gstreamer_.finalize();
    renderer_.shutdown();
    platform_.shutdown();
}

/// @brief アプリケーションに必要な全てのコンポーネントを初期化する。
/// @return
bool Application::initialize()
{
    // GStreamerの初期化
    if (!gstreamer_.initialize())
    {
        std::cerr << "Failed to initialize GStreamerSupport." << std::endl;
        return false;
    }
    // グラフィックスプラットフォームの初期化
    if (!platform_.initialize())
    {
        std::cerr << "Failed to initialize GraphicsPlatform." << std::endl;
        return false;
    }
    // レンダラーの初期化
    if (!renderer_.initialize(platform_.getScreenWidth(), platform_.getScreenHeight()))
    {
        std::cerr << "Failed to initialize Renderer." << std::endl;
        return false;
    }
    // テロップレンダラーの初期化
    if (!telopRenderer_.initialize("/usr/share/fonts/truetype/vlgothic/VL-Gothic-Regular.ttf"))
    {
        std::cerr << "Failed to initialize TelopRenderer" << std::endl;
        return false;
    }

    // 初期テキストの設定
    telopRenderer_.SetFontSize(64); // フォントサイズを設定
    telopRenderer_.SetText("こんにちは、世界！テロップのテスト中です・・・・・いかがでしょうか？〇(^^♪〇");
    telopRenderer_.setOutline(true);
    telopRenderer_.setOutlineColor(0.0f, 0.0f, 0.0f, 0.8f);
    telopRenderer_.setOutlinePixelWidth(4.0f);

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
    std::cout.imbue(std::locale(""));

    util timer;
    // タイマーを開始
    timer.StartTimer();

    if (!gstreamer_.startPipeline("sample.mp4"))
    {
        std::cerr << "Failed to start GStreamer pipeline." << std::endl;
        return false;
    }
    std::cout << "[App] Waiting for first frame..." << std::endl;

    GStreamerSupport::FrameData frame;
    int retries = 300;
    // 3秒間、フレームが取得できるのを待つ
    while (retries-- > 0)
    {
        if (gstreamer_.getFrameData(frame))
        {
            std::cout << "[App] First frame received." << std::endl;
            break;
        }
        usleep(10000);
    }

    if (retries <= 0)
    {
        std::cerr << "[App] Timeout waiting for first frame." << std::endl;
        return false;
    }

    FPSCounter fpsCounter;
    bool isScreenshot = false;

    // 動画は無限ループで再生するので、ESCキーで終了
    while (true)
    {
        if (kbhit())
        {
            int ch = getchar();
            if (ch == 27)
            {
                std::cout << "ESC pressed. Exiting..." << std::endl;
                break;
            }
            else if (ch == 's' || ch == 'S')
            {
                isScreenshot = true;
            }
        }

        // GStreamerのバスからメッセージをチェック
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
            renderer_.renderToFBO();
            renderer_.renderYUV(platform_.getScreenWidth(), platform_.getScreenHeight());
            telopRenderer_.update();
            telopRenderer_.render();
            renderer_.endOffscreenRender();

            // ここで FBO の内容を画面に描画
            renderer_.renderFBOToScreen(platform_.getScreenWidth(), platform_.getScreenHeight());

            platform_.swapBuffers();

            // スクリーンショット処理
            if (isScreenshot)
            {
                auto t = std::time(nullptr);
                std::stringstream ss;
                ss << "ScreenShot_" << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".png";

                std::vector<unsigned char> pixelData;
                // FBOからピクセルデータを読み取る
                if (renderer_.readPixelsFromFBO(pixelData, platform_.getScreenWidth(), platform_.getScreenHeight()))
                {
                    // ピクセルデータをPNGとして保存
                    platform_.savePixelsToPNG(ss.str().c_str(), pixelData.data());
                    std::cout << "[App] Screenshot saved to " << ss.str() << std::endl;
                }
                else
                {
                    std::cerr << "[App] Failed to read pixels from FBO." << std::endl;
                }

                isScreenshot = false;
            }

            // FPSカウンターの更新
            if (fpsCounter.frame())
            {
                // 1秒経過したのでログ出力
                util::LogAvailableMemory();
                // 経過時間をログ出力
                timer.LogElapsedTimeHMS();
            }

            // フレームデータの解放
            gstreamer_.releaseFrame(frame);
        }
        else
        {
            // フレームが取得できなかった場合は少し待機
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::cout << "Playback finished." << std::endl;
    return true;
}
