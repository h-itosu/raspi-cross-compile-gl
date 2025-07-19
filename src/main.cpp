/**
 * @file main.cpp
 * @brief アプリケーションのエントリーポイント(開始地点)
 */
#include "Application.h"
#include <iostream>

int main() {
    // アプリケーションオブジェクトを作成
    Application app;

    // 初期化処理を実行
    if (app.initialize()) {
        // 初期化が成功した場合のみ、メインループを実行
        app.run();
    } else {
        // 初期化に失敗した場合はエラーメッセージを表示
        std::cerr << "Application failed to initialize." << std::endl;
        return -1;
    }

    // 正常終了
    return 0;
}