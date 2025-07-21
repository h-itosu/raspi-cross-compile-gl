#ifndef GSTREAMER_SUPPORT_H
#define GSTREAMER_SUPPORT_H

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <cstdint>
#include <vector>

/**
 * @brief GStreamer から YUV(I420) 映像フレームを取得するサポートクラス
 */
class GStreamerSupport
{
public:
    GStreamerSupport() = default;
    ~GStreamerSupport() = default;

    bool initialize();
    void finalize();
    bool startPipeline(const char *filepath);

    struct FrameData
    {
        uint8_t *data = nullptr;
        int width = 0;
        int height = 0;
        int stride = 0;
        GstSample *sample = nullptr; // 追加
        GstMapInfo map;              // 追加
    };

    bool getFrameData(FrameData &outFrame);
    void releaseFrame(FrameData &frame);

private:
    GstElement *pipeline_ = nullptr;
    GstElement *appsink_ = nullptr;
};

#endif // GSTREAMER_SUPPORT_H
