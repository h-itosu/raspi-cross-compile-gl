/**
 * @file main.cpp
 * @brief アプリケーションのエントリーポイント(開始地点)
 */
#include "Application.h"
#include <iostream>

int main()
{
    // アプリケーションオブジェクトを作成
    Application app;

    // 初期化処理を実行
    if (!app.initialize())
    {
        std::cerr << "Application failed to initialize." << std::endl;
        return 1; // 初期化失敗
    }

    if (!app.run())
    {
        std::cerr << "Application failed to run." << std::endl;
        return 2; // 実行失敗
    }
    // アプリケーションの実行が成功した場合の処理
    std::cout << "Application is running successfully." << std::endl;
    return 0; // 正常終了
}
