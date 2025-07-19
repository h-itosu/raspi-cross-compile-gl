/**
 * @file Application.cpp
 * @brief アプリケーション全体を管理するクラスの実装
 */
#include "Application.h"
#include <unistd.h> // usleepのため

Application::Application() {}
Application::~Application() {}

/**
 * @brief アプリケーションの初期化
 * プラットフォーム(ハードウェア) -> レンダラー(描画) の順で初期化を行う
 */
bool Application::initialize() {
    if (!platform_.initialize()) {
        return false;
    }
    if (!renderer_.initialize()) {
        return false;
    }
    return true;
}

/**
 * @brief メインループの実行
 * 決められたフレーム数だけループし、アニメーションを行う
 */
void Application::run() {
    float angle = 0.0f;
    for (int i = 0; i < 600; ++i) { // 60fpsで約10秒間
        // 1. 描画処理を呼び出す
        renderer_.render(angle, platform_.getScreenWidth(), platform_.getScreenHeight());

        // 2. 描画結果を画面に反映させる
        platform_.swapBuffers();

        // 3. 次のフレームのために角度を更新
        angle += 1.0f;

        // 4. 約60fpsになるように少し待機
        usleep(16666);
    }
}
