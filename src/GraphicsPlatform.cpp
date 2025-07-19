#include "GraphicsPlatform.h"
#include <iostream>
#include <fcntl.h>  // open
#include <unistd.h> // close

GraphicsPlatform::GraphicsPlatform() {}
GraphicsPlatform::~GraphicsPlatform() {
    shutdown();
}

bool GraphicsPlatform::initialize() {
    // 1. DRM (Direct Rendering Manager) デバイスを開く
    //    /dev/dri/card0 はプライマリGPUデバイスを指す
    drm_fd_ = open("/dev/dri/card0", O_RDWR);
    if (drm_fd_ < 0) { std::cerr << "Failed to open DRM device." << std::endl; return false; }

    // 2. DRMリソースを取得する
    drm_resources_ = drmModeGetResources(drm_fd_);
    if (!drm_resources_) { std::cerr << "Failed to get DRM resources." << std::endl; return false; }

    // 3. 有効なディスプレイ接続(コネクタ)を探す
    for (int i = 0; i < drm_resources_->count_connectors; i++) {
        drmModeConnector* temp_connector = drmModeGetConnector(drm_fd_, drm_resources_->connectors[i]);
        if (temp_connector->connection == DRM_MODE_CONNECTED) {
            drm_connector_ = temp_connector;
            break;
        }
        drmModeFreeConnector(temp_connector);
    }
    if (!drm_connector_) { std::cerr << "No connected connector found." << std::endl; return false; }

    // 4. コネクタからディスプレイモードとエンコーダ、CRTC(ディスプレイコントローラ)を特定する
    connector_id_ = drm_connector_->connector_id;
    mode_info_ = drm_connector_->modes[0]; // 最初の利用可能なモードを使用

    drmModeEncoder* encoder = drmModeGetEncoder(drm_fd_, drm_connector_->encoder_id);
    if (!encoder) { std::cerr << "Failed to get DRM encoder." << std::endl; return false; }
    crtc_id_ = encoder->crtc_id;

    // 5. プログラム終了時に復元するため、現在のCRTC設定を保存しておく
    original_crtc_ = drmModeGetCrtc(drm_fd_, crtc_id_);
    drmModeFreeEncoder(encoder);

    // 6. GBM (Generic Buffer Management) デバイスを作成する
    gbm_device_ = gbm_create_device(drm_fd_);
    if (!gbm_device_) { std::cerr << "Failed to create GBM device." << std::endl; return false; }

    // 7. EGL (Embedded-System Graphics Library) を初期化する
    //    GBMデバイスを介してEGLディスプレイを取得
    display_ = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA, gbm_device_, NULL);
    if (display_ == EGL_NO_DISPLAY) { std::cerr << "Failed to get EGL display." << std::endl; return false; }
    if (eglInitialize(display_, NULL, NULL) == EGL_FALSE) { std::cerr << "Failed to initialize EGL." << std::endl; return false; }

    // 8. EGLのフレームバッファ設定(Config)を選択する
    const EGLint attrib_list[] = {
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, // 色深度
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, // ウィンドウへの描画用
        EGL_NONE
    };
    EGLint num_config;
    if (eglChooseConfig(display_, attrib_list, &config_, 1, &num_config) == EGL_FALSE) { std::cerr << "Failed to choose EGL config." << std::endl; return false; }

    // 9. GBMサーフェスと、それを元にしたEGLウィンドウサーフェスを作成
    gbm_surface_ = gbm_surface_create(gbm_device_, mode_info_.hdisplay, mode_info_.vdisplay, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbm_surface_) { std::cerr << "Failed to create GBM surface." << std::endl; return false; }
    surface_ = eglCreateWindowSurface(display_, config_, (EGLNativeWindowType)gbm_surface_, NULL);
    if (surface_ == EGL_NO_SURFACE) { std::cerr << "Failed to create EGL surface." << std::endl; return false; }

    // 10. EGL描画コンテキストを作成
    const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE }; // OpenGL ES 2.0
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attribs);
    if (context_ == EGL_NO_CONTEXT) { std::cerr << "Failed to create EGL context." << std::endl; return false; }

    // 11. 作成したコンテキストとサーフェスを現在のスレッドにバインドする
    if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_FALSE) {
        std::cerr << "Failed to make EGL current." << std::endl;
        return false;
    }

    return true;
}

void GraphicsPlatform::shutdown() {
    // 取得したリソースを、取得したのと逆の順序で解放していく

    // 1. 元のディスプレイ設定に復元
    if (original_crtc_) {
        drmModeSetCrtc(drm_fd_, original_crtc_->crtc_id, original_crtc_->buffer_id,
                       original_crtc_->x, original_crtc_->y, &connector_id_, 1, &original_crtc_->mode);
        drmModeFreeCrtc(original_crtc_);
    }

    // 2. EGLリソースの解放
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
        if (surface_ != EGL_NO_SURFACE) eglDestroySurface(display_, surface_);
        eglTerminate(display_);
    }

    // 3. DRMフレームバッファとGBMバッファの解放
    if (previous_fb_id_ != 0) drmModeRmFB(drm_fd_, previous_fb_id_);
    if (gbm_surface_) {
        if (previous_bo_) gbm_surface_release_buffer(gbm_surface_, previous_bo_);
        gbm_surface_destroy(gbm_surface_);
    }

    // 4. DRMとGBMのデバイスレベルのリソースを解放
    if (drm_connector_) drmModeFreeConnector(drm_connector_);
    if (drm_resources_) drmModeFreeResources(drm_resources_);
    if (gbm_device_) gbm_device_destroy(gbm_device_);
    if (drm_fd_ >= 0) close(drm_fd_);
}

void GraphicsPlatform::swapBuffers() {
    // 1. EGLにバッファの交換を指示
    eglSwapBuffers(display_, surface_);
    // 2. 描画が完了したバックバッファを、次のフロントバッファとしてロックする
    struct gbm_bo* next_bo = gbm_surface_lock_front_buffer(gbm_surface_);

    // 3. GBMバッファからDRMフレームバッファを作成
    uint32_t handle = gbm_bo_get_handle(next_bo).u32;
    uint32_t pitch = gbm_bo_get_stride(next_bo);
    uint32_t fb_id;
    drmModeAddFB(drm_fd_, mode_info_.hdisplay, mode_info_.vdisplay, 24, 32, pitch, handle, &fb_id);

    // 4. 新しいフレームバッファをディスプレイに表示するようにCRTCに指示(ページフリップ)
    drmModeSetCrtc(drm_fd_, crtc_id_, fb_id, 0, 0, &connector_id_, 1, &mode_info_);

    // 5. 前のフレームバッファを解放
    if (previous_bo_) {
        drmModeRmFB(drm_fd_, previous_fb_id_);
        gbm_surface_release_buffer(gbm_surface_, previous_bo_);
    }
    previous_bo_ = next_bo;
    previous_fb_id_ = fb_id;
}

uint32_t GraphicsPlatform::getScreenWidth() const { return mode_info_.hdisplay; }
uint32_t GraphicsPlatform::getScreenHeight() const { return mode_info_.vdisplay; }
