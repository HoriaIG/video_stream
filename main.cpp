//#include "main_headers.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/app.h>
#include <opencv2/opencv.hpp>

using namespace std;

int main(int argc, char *argv[]) {
    
    //init gstreamer
    gst_init(&argc, &argv);
    
    //create the camera listing obj
    GstDeviceMonitor *monitor = gst_device_monitor_new();
    //add some filtering for video only
    gst_device_monitor_add_filter(monitor, "Video/Source", nullptr);

    //start the monitor with the obj 
    gst_device_monitor_start(monitor);

    GList *devices = gst_device_monitor_get_devices(monitor);

    GstDevice *device = nullptr;

    for (GList *l = devices; l; l = l->next) {
        device = GST_DEVICE(l->data);

        cout << "Device is " << gst_device_get_display_name(device) << "\n";

        GstStructure* propString = gst_device_get_properties(device);
        cout << "Properties: " << gst_structure_to_string(propString) << "\n";

        GstCaps* capString = gst_device_get_caps(device);
        cout << "Capabilities: " << gst_caps_to_string(capString) << "\n";

        //the next part can be removed if you just want the last camera interface
        //this can lead to some errors if you have multiple video cameras might change later
        //for me it has to be video2
        const gchar *path = gst_structure_get_string(propString, "api.v4l2.path");
        if (path && strcmp(path, "/dev/video2") == 0) {
            gst_structure_free(propString);
            break;
        }
        
        gst_structure_free(propString);

    }

    //this is the process pipeline
    GstElement *pipeline = gst_pipeline_new("camera-pipeline");

    //create the element by using the found camera
    GstElement *source = gst_device_create_element(device, "camera");

    //this is the endpoint sink display stuff
    GstElement *sink = gst_element_factory_make("autovideosink", "display");

    //setup some error to doublecheck
    if (!pipeline || !source || !sink) {
        cerr << "Failed to create GStreamer elements\n";
        return 1;
    }   

    gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);


    GstElement *capsfilter = gst_element_factory_make("capsfilter", "camera-caps");

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, 640,
        "height", G_TYPE_INT, 480,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        nullptr
    );
    g_object_set(capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    GstElement *convert = gst_element_factory_make("videoconvert", "convert");

    //need this to bind the pipeline to the source and sink created earlier
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, convert, sink, nullptr);

    //connect the input to output
    gst_element_link_many(source, capsfilter, convert, sink, nullptr);

    //start the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    GstBus *bus = gst_element_get_bus(pipeline);

    while (true) {
        GstMessage *msg = gst_bus_timed_pop_filtered(
            bus,
            GST_CLOCK_TIME_NONE,
            (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS)
        );

        if (msg) {
            gst_message_unref(msg);
            break;
        }
    }

    //clean up the rest of elements
    gst_element_set_state(pipeline, GST_STATE_NULL);

    gst_object_unref(bus);
    gst_object_unref(pipeline);

    return 0;
}