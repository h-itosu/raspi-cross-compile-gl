#include "GStreamerSupport.h"
#include <iostream>
#include <cstring> // ← これを追加

bool GStreamerSupport::initialize()
{
    gst_init(nullptr, nullptr);
    return true;
}

void GStreamerSupport::finalize()
{
    if (pipeline_)
    {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
}

bool GStreamerSupport::startPipeline(const char *filepath)
{
    std::string pipelineDesc = std::string("filesrc location=") + filepath +
                               " ! qtdemux name=demux "
                               " demux.video_0 ! queue "
                               " ! h264parse ! v4l2h264dec "
                               " ! queue "
                               " ! v4l2convert output-io-mode=dmabuf-import "
                               " ! video/x-raw,format=I420 "
                               " ! appsink name=mysink sync=false";

    GError *error = nullptr;
    pipeline_ = gst_parse_launch(pipelineDesc.c_str(), &error);
    if (!pipeline_)
    {
        std::cerr << "[GStreamer] Failed to create pipeline: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");
    if (!appsink_)
    {
        std::cerr << "[GStreamer] Failed to get appsink." << std::endl;
        return false;
    }

    gst_app_sink_set_emit_signals(GST_APP_SINK(appsink_), false);
    gst_app_sink_set_drop(GST_APP_SINK(appsink_), true); // drop有効化！
    gst_app_sink_set_max_buffers(GST_APP_SINK(appsink_), 10);
    g_object_set(G_OBJECT(appsink_), "sync", FALSE, nullptr);

    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    return true;
}

bool GStreamerSupport::restartPipeline(const char *filepath)
{
    finalize();
    if (!initialize())
        return false;
    return startPipeline(filepath);
}

bool GStreamerSupport::getFrameData(FrameData &outFrame)
{
    if (!appsink_)
        return false;

    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink_), 10000000); // 10ms
    if (!sample)
    {
        if (gst_app_sink_is_eos(GST_APP_SINK(appsink_)))
        {
            //            std::cerr << "[GStreamer] End of stream reached (EOS)." << std::endl;
        }
        else
        {
            //            std::cerr << "[GStreamer] Failed to pull sample (timeout or not ready)." << std::endl;
        }
        return false;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    if (!buffer || !caps)
    {
        gst_sample_unref(sample);
        return false;
    }

    GstStructure *s = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(s, "width", &outFrame.width);
    gst_structure_get_int(s, "height", &outFrame.height);

    if (!gst_buffer_map(buffer, &outFrame.map, GST_MAP_READ))
    {
        gst_sample_unref(sample);
        return false;
    }

    outFrame.data = outFrame.map.data;
    outFrame.sample = sample;
    // stride を取得したい場合は caps から取り出す（必要なら）
    return true;
}

void GStreamerSupport::releaseFrame(FrameData &frame)
{
    if (frame.sample)
    {
        GstBuffer *buffer = gst_sample_get_buffer(frame.sample);
        gst_buffer_unmap(buffer, &frame.map);
        gst_sample_unref(frame.sample);
        frame.sample = nullptr;
        frame.data = nullptr;
    }
}
