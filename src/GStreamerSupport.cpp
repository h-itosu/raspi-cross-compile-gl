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
                               " ! decodebin ! videoconvert ! video/x-raw,format=I420 ! appsink name=mysink";

    GError *error = nullptr;
    pipeline_ = gst_parse_launch(pipelineDesc.c_str(), &error);
    if (!pipeline_)
    {
        std::cerr << "Failed to create pipeline: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");
    if (!appsink_)
    {
        std::cerr << "Failed to get appsink." << std::endl;
        return false;
    }

    gst_app_sink_set_emit_signals((GstAppSink *)appsink_, false);
    gst_app_sink_set_drop((GstAppSink *)appsink_, true);
    gst_app_sink_set_max_buffers((GstAppSink *)appsink_, 1);

    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    return true;
}

bool GStreamerSupport::getFrameData(FrameData &outFrame)
{
    if (!appsink_)
        return false;

    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink_), 10000000); // 10ms timeout
    if (!sample)
        return false;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *s = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(s, "width", &outFrame.width);
    gst_structure_get_int(s, "height", &outFrame.height);

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ))
    {
        size_t ySize = outFrame.width * outFrame.height;
        size_t uSize = (outFrame.width / 2) * (outFrame.height / 2);
        size_t total = ySize + uSize * 2;

        outFrame.buffer.resize(total);
        std::memcpy(outFrame.buffer.data(), map.data, total);

        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
    return true;
}
