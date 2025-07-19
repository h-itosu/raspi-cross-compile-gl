/**
 * @file GraphicsPlatform.h
 * @brief グラフィックスプラットフォームの初期化と制御を行うクラスの宣言
 */
#ifndef GRAPHICS_PLATFORM_H
#define GRAPHICS_PLATFORM_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <cstdint>

/**
 * @class GraphicsPlatform
 * @brief DRM/KMS/GBM/EGLといった低レベルAPIを管理し、OSとハードウェアの差異を抽象化するクラス。
 * ウィンドウの作成、描画コンテキストの管理、画面の更新などを担当する。
 */
class GraphicsPlatform {
public:
    /** @brief コンストラクタ */
    GraphicsPlatform();
    /** @brief デストラクタ。自動的にshutdown()を呼び出す。 */
    ~GraphicsPlatform();

    /**
     * @brief グラフィックス環境を初期化する。
     * @return 初期化に成功した場合はtrue、失敗した場合はfalse。
     */
    bool initialize();

    /**
     * @brief確保したグラフィックスリソースを全て解放する。
     */
    void shutdown();

    /**
     * @brief バックバッファとフロントバッファを交換し、描画内容を画面に表示する。
     */
    void swapBuffers();

    /** @brief 画面の幅を取得する。 @return 画面の幅（ピクセル数）。 */
    uint32_t getScreenWidth() const;
    /** @brief 画面の高さを取得する。 @return 画面の高さ（ピクセル数）。 */
    uint32_t getScreenHeight() const;

private:
    // --- EGL関連のリソース ---
    /// @brief EGLディスプレイ接続ハンドル
    EGLDisplay display_ = EGL_NO_DISPLAY;
    /// @brief EGLフレームバッファ設定
    EGLConfig config_;
    /// @brief EGL描画コンテキスト
    EGLContext context_ = EGL_NO_CONTEXT;
    /// @brief EGLウィンドウサーフェス
    EGLSurface surface_ = EGL_NO_SURFACE;
    
    // --- GBM関連のリソース ---
    /// @brief GBMデバイスハンドル
    struct gbm_device* gbm_device_ = nullptr;
    /// @brief GBMサーフェスハンドル
    struct gbm_surface* gbm_surface_ = nullptr;
    
    // --- DRM/KMS関連のリソース ---
    /// @brief DRMデバイスのファイルディスクリプタ
    int drm_fd_ = -1;
    /// @brief DRMリソース情報へのポインタ
    drmModeRes* drm_resources_ = nullptr;
    /// @brief 使用するディスプレイコネクタ情報へのポインタ
    drmModeConnector* drm_connector_ = nullptr;
    /// @brief コネクタのID
    uint32_t connector_id_;
    /// @brief 使用するディスプレイモード情報
    drmModeModeInfo mode_info_;
    /// @brief CRTC(ディスプレイコントローラ)のID
    uint32_t crtc_id_;
    /// @brief プログラム実行前のCRTCの状態（終了時に復元するため）
    drmModeCrtc* original_crtc_ = nullptr;
    
    // --- バッファ管理 ---
    /// @brief 前のフレームで表示したGBMバッファオブジェクト
    struct gbm_bo* previous_bo_ = nullptr;
    /// @brief 前にフレームで表示したDRMフレームバッファのID
    uint32_t previous_fb_id_ = 0;
};

#endif // GRAPHICS_PLATFORM_H