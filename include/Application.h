/**
 * @file Application.h
 * @brief アプリケーション全体を管理するクラスの宣言
 */
#ifndef APPLICATION_H
#define APPLICATION_H

#include "GraphicsPlatform.h"
#include "Renderer.h"

/**
 * @class Application
 * @brief アプリケーションのメインループを実行し、全体の初期化と終了処理を統括する。
 */
class Application {
public:
    /** @brief コンストラクタ */
    Application();
    /** @brief デストラクタ */
    ~Application();

    /**
     * @brief アプリケーションに必要な全てのコンポーネントを初期化する。
     * @return 初期化に成功した場合はtrue、失敗した場合はfalse。
     */
    bool initialize();

    /**
     * @brief アプリケーションのメインループを開始する。
     */
    void run();

private:
    /// @brief グラフィックスプラットフォームのインスタンス
    GraphicsPlatform platform_;
    /// @brief レンダラーのインスタンス
    Renderer renderer_;
};

#endif // APPLICATION_H