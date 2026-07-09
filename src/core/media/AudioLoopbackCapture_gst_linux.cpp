#include "core/media/AudioLoopbackCaptureGStreamer.h"

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include <QDebug>

namespace AudioLoopbackGst {

namespace {

GstElement *g_pipeline = nullptr;
GstElement *g_appsink = nullptr;

} // namespace

bool start(const QString &pulseMonitorDeviceId) {
    stop();

    if (pulseMonitorDeviceId.isEmpty())
        return false;

    if (!gst_is_initialized())
        gst_init(nullptr, nullptr);

    g_pipeline = gst_pipeline_new("audio-loopback");
    GstElement *src = gst_element_factory_make("pulsesrc", "src");
    GstElement *convert = gst_element_factory_make("audioconvert", "convert");
    GstElement *resample = gst_element_factory_make("audioresample", "resample");
    GstElement *capsfilter = gst_element_factory_make("capsfilter", "caps");
    g_appsink = gst_element_factory_make("appsink", "sink");

    if (!g_pipeline || !src || !convert || !resample || !capsfilter || !g_appsink) {
        qWarning() << "AudioLoopbackGst: failed to create GStreamer elements";
        stop();
        return false;
    }

    g_object_set(src, "device", pulseMonitorDeviceId.toUtf8().constData(), nullptr);

    GstCaps *caps = gst_caps_from_string(
        "audio/x-raw,format=F32LE,channels=2,rate=44100,layout=interleaved");
    g_object_set(capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    g_object_set(g_appsink,
                 "emit-signals", FALSE,
                 "sync", FALSE,
                 "max-buffers", 1,
                 "drop", TRUE,
                 nullptr);

    gst_bin_add_many(GST_BIN(g_pipeline), src, convert, resample, capsfilter, g_appsink, nullptr);
    if (!gst_element_link_many(src, convert, resample, capsfilter, g_appsink, nullptr)) {
        qWarning() << "AudioLoopbackGst: failed to link pipeline";
        stop();
        return false;
    }

    const GstStateChangeReturn ret = gst_element_set_state(g_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qWarning() << "AudioLoopbackGst: failed to start pipeline for" << pulseMonitorDeviceId;
        stop();
        return false;
    }

    qInfo() << "AudioLoopbackGst: capturing monitor" << pulseMonitorDeviceId;
    return true;
}

void stop() {
    if (g_pipeline) {
        gst_element_set_state(g_pipeline, GST_STATE_NULL);
        gst_object_unref(g_pipeline);
        g_pipeline = nullptr;
    }
    g_appsink = nullptr;
}

bool isRunning() {
    return g_pipeline != nullptr;
}

bool pull(QByteArray &pcmOut) {
    pcmOut.clear();
    if (!g_appsink)
        return false;

    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(g_appsink), 0);
    if (!sample)
        return false;

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        return false;
    }

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return false;
    }

    pcmOut = QByteArray(reinterpret_cast<const char *>(map.data),
                        static_cast<int>(map.size));
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return !pcmOut.isEmpty();
}

} // namespace AudioLoopbackGst
