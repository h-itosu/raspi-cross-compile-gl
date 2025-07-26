#include "GraphicsPlatform.h"
#include <iostream>
#include <vector>
#include <fcntl.h>  // open
#include <unistd.h> // close
#include <fstream>
#include <png.h>
#include <GLES2/gl2.h>

GraphicsPlatform::GraphicsPlatform() {}
GraphicsPlatform::~GraphicsPlatform()
{
    shutdown();
}

bool GraphicsPlatform::initialize()
{
    // 1. 利用可能なDRMデバイスを開く
    const std::vector<std::string> drm_devices = {"/dev/dri/card0", "/dev/dri/card1"};
    for (const auto &device_path : drm_devices)
    {
        drm_fd_ = open(device_path.c_str(), O_RDWR);
        if (drm_fd_ >= 0)
        {
            drm_resources_ = drmModeGetResources(drm_fd_);
            if (drm_resources_)
            {
                std::cout << "Success: Using DRM device " << device_path << std::endl;
                break;
            }
            close(drm_fd_);
            drm_fd_ = -1;
        }
    }

    if (drm_fd_ < 0 || !drm_resources_)
    {
        std::cerr << "Error: Could not get DRM resources from any device." << std::endl;
        return false;
    }

    // 2. 接続されているコネクタを探す
    for (int i = 0; i < drm_resources_->count_connectors; i++)
    {
        drmModeConnector *connector = drmModeGetConnector(drm_fd_, drm_resources_->connectors[i]);
        if (connector && connector->connection == DRM_MODE_CONNECTED)
        {
            drm_connector_ = connector;
            connector_id_ = connector->connector_id;
            break;
        }
        if (connector)
            drmModeFreeConnector(connector);
    }

    if (!drm_connector_)
    {
        std::cerr << "Error: No connected connector found." << std::endl;
        return false;
    }

    // 3. コネクタの「推奨モード」を探す
    bool found_preferred_mode = false;
    for (int i = 0; i < drm_connector_->count_modes; i++)
    {
        if (drm_connector_->modes[i].type & DRM_MODE_TYPE_PREFERRED)
        {
            mode_info_ = drm_connector_->modes[i];
            found_preferred_mode = true;
            break;
        }
    }
    // 推奨モードがなければ、リストの最初のモードをフォールバックとして使用
    if (!found_preferred_mode && drm_connector_->count_modes > 0)
    {
        mode_info_ = drm_connector_->modes[0];
    }
    else if (drm_connector_->count_modes == 0)
    {
        std::cerr << "Error: No modes available for the connector." << std::endl;
        return false;
    }

    // 4. コネクタに合ったエンコーダとCRTCを探す
    drmModeEncoder *encoder = nullptr;
    for (int i = 0; i < drm_resources_->count_encoders; i++)
    {
        encoder = drmModeGetEncoder(drm_fd_, drm_resources_->encoders[i]);
        if (encoder->encoder_id == drm_connector_->encoder_id)
        {
            break;
        }
        drmModeFreeEncoder(encoder);
        encoder = nullptr;
    }
    if (!encoder)
    {
        std::cerr << "Error: Failed to get a suitable DRM encoder." << std::endl;
        return false;
    }
    crtc_id_ = encoder->crtc_id;
    original_crtc_ = drmModeGetCrtc(drm_fd_, crtc_id_);
    drmModeFreeEncoder(encoder);

    // 5. GBMデバイスを作成
    gbm_device_ = gbm_create_device(drm_fd_);
    if (!gbm_device_)
    {
        std::cerr << "Error: Failed to create GBM device." << std::endl;
        return false;
    }

    // 6. GBMサーフェスを作成し、成功したフォーマットを変数に保存
    EGLint gbm_format = GBM_FORMAT_XRGB8888; // 第一候補をセット
    gbm_surface_ = gbm_surface_create(gbm_device_, mode_info_.hdisplay, mode_info_.vdisplay,
                                      gbm_format, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbm_surface_)
    {
        std::cout << "Info: Failed to create GBM surface with XRGB8888. Retrying with ARGB8888..." << std::endl;
        gbm_format = GBM_FORMAT_ARGB8888; // 第二候補をセット
        gbm_surface_ = gbm_surface_create(gbm_device_, mode_info_.hdisplay, mode_info_.vdisplay,
                                          gbm_format, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    }
    if (!gbm_surface_)
    {
        std::cerr << "Error: Failed to create GBM surface with any format." << std::endl;
        return false;
    }

    // 7. EGLを初期化
    display_ = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA, gbm_device_, NULL);
    if (display_ == EGL_NO_DISPLAY)
    {
        std::cerr << "Failed to get EGL display." << std::endl;
        return false;
    }
    if (eglInitialize(display_, NULL, NULL) == EGL_FALSE)
    {
        std::cerr << "Failed to initialize EGL." << std::endl;
        return false;
    }

    // 8. 保存したgbm_formatと互換性のあるEGL設定(Config)を検索する
    EGLint num_configs;
    eglGetConfigs(display_, NULL, 0, &num_configs);
    std::vector<EGLConfig> configs(num_configs);
    eglGetConfigs(display_, configs.data(), num_configs, &num_configs);

    bool found_config = false;
    for (int i = 0; i < num_configs; ++i)
    {
        EGLint id;
        if (eglGetConfigAttrib(display_, configs[i], EGL_NATIVE_VISUAL_ID, &id) && id == gbm_format)
        {
            config_ = configs[i];
            found_config = true;
            break;
        }
    }
    if (!found_config)
    {
        std::cerr << "Failed to find a compatible EGL config." << std::endl;
        return false;
    }

    // 9. EGLウィンドウサーフェスを作成
    surface_ = eglCreateWindowSurface(display_, config_, (EGLNativeWindowType)gbm_surface_, NULL);
    if (surface_ == EGL_NO_SURFACE)
    {
        std::cerr << "Failed to create EGL surface." << std::endl;
        return false;
    }

    // 10. EGL描画コンテキストを作成
    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attribs);
    if (context_ == EGL_NO_CONTEXT)
    {
        std::cerr << "Failed to create EGL context." << std::endl;
        return false;
    }

    // 11. 作成したコンテキストとサーフェスを現在のスレッドにバインドする
    if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_FALSE)
    {
        std::cerr << "Failed to make EGL current." << std::endl;
        return false;
    }

    std::cout << "Graphics platform initialized successfully." << std::endl;
    return true;
}
void GraphicsPlatform::shutdown()
{
    if (original_crtc_)
    {
        drmModeSetCrtc(drm_fd_, original_crtc_->crtc_id, original_crtc_->buffer_id,
                       original_crtc_->x, original_crtc_->y, &connector_id_, 1, &original_crtc_->mode);
        drmModeFreeCrtc(original_crtc_);
        original_crtc_ = nullptr;
    }
    if (display_ != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT)
        {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE)
        {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
    if (previous_fb_id_ != 0)
    {
        drmModeRmFB(drm_fd_, previous_fb_id_);
        previous_fb_id_ = 0;
    }
    if (gbm_surface_)
    {
        if (previous_bo_)
        {
            gbm_surface_release_buffer(gbm_surface_, previous_bo_);
            previous_bo_ = nullptr;
        }
        gbm_surface_destroy(gbm_surface_);
        gbm_surface_ = nullptr;
    }
    if (drm_connector_)
    {
        drmModeFreeConnector(drm_connector_);
        drm_connector_ = nullptr;
    }
    if (drm_resources_)
    {
        drmModeFreeResources(drm_resources_);
        drm_resources_ = nullptr;
    }
    if (gbm_device_)
    {
        gbm_device_destroy(gbm_device_);
        gbm_device_ = nullptr;
    }
    if (drm_fd_ >= 0)
    {
        close(drm_fd_);
        drm_fd_ = -1;
    }
}

void GraphicsPlatform::swapBuffers()
{
    eglSwapBuffers(display_, surface_);
    struct gbm_bo *next_bo = gbm_surface_lock_front_buffer(gbm_surface_);
    if (!next_bo)
    {
        std::cerr << "Failed to lock front buffer." << std::endl;
        return;
    }

    uint32_t handle = gbm_bo_get_handle(next_bo).u32;
    uint32_t pitch = gbm_bo_get_stride(next_bo);
    uint32_t fb_id = 0;
    if (drmModeAddFB(drm_fd_, mode_info_.hdisplay, mode_info_.vdisplay, 24, 32, pitch, handle, &fb_id) != 0)
    {
        std::cerr << "Failed to add DRM framebuffer." << std::endl;
        gbm_surface_release_buffer(gbm_surface_, next_bo);
        return;
    }

    drmModeSetCrtc(drm_fd_, crtc_id_, fb_id, 0, 0, &connector_id_, 1, &mode_info_);

    if (previous_bo_)
    {
        drmModeRmFB(drm_fd_, previous_fb_id_);
        gbm_surface_release_buffer(gbm_surface_, previous_bo_);
    }
    previous_bo_ = next_bo;
    previous_fb_id_ = fb_id;
}

void GraphicsPlatform::saveFramebufferToPNG(const char *filename)
{
    int width = mode_info_.hdisplay;
    int height = mode_info_.vdisplay;

    std::vector<uint8_t> pixels(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // 上下反転（OpenGLは左下原点）
    for (int y = 0; y < height / 2; ++y)
    {
        for (int x = 0; x < width * 3; ++x)
        {
            std::swap(pixels[y * width * 3 + x], pixels[(height - 1 - y) * width * 3 + x]);
        }
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp)
        return;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png)
        return;

    png_infop info = png_create_info_struct(png);
    if (!info)
        return;

    if (setjmp(png_jmpbuf(png)))
        return;

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8,
                 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png, info);

    std::vector<png_bytep> rows(height);
    for (int y = 0; y < height; ++y)
    {
        rows[y] = pixels.data() + y * width * 3;
    }
    png_write_image(png, rows.data());
    png_write_end(png, nullptr);

    fclose(fp);
    png_destroy_write_struct(&png, &info);
}

uint32_t GraphicsPlatform::getScreenWidth() const { return mode_info_.hdisplay; }
uint32_t GraphicsPlatform::getScreenHeight() const { return mode_info_.vdisplay; }
