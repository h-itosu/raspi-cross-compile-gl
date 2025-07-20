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
        std::vector<uint8_t> buffer; ///< YUV連結データ
        int width = 0;
        int height = 0;
    };

    bool getFrameData(FrameData &outFrame);

private:
    GstElement *pipeline_ = nullptr;
    GstElement *appsink_ = nullptr;
};

#endif // GSTREAMER_SUPPORT_H
