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

static GstPadProbeReturn invert_colors(
    GstPad *pad,
    GstPadProbeInfo *info,
    gpointer user_data)
{
    (void) pad;
    (void) user_data;
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    if (!buffer)
        return GST_PAD_PROBE_OK;

    GstMapInfo map;

    if (gst_buffer_map(buffer, &map, GST_MAP_READWRITE)) {

        // RGB = 3 bytes per pixel
        for (gsize i = 0; i + 2 < map.size; i += 3) {
            map.data[i]     = 255 - map.data[i];     // R
            map.data[i + 1] = 255 - map.data[i + 1]; // G
            map.data[i + 2] = 255 - map.data[i + 2]; // B
        }

        gst_buffer_unmap(buffer, &map);
    }

    return GST_PAD_PROBE_OK;
}


int main(int argc, char *argv[]) {
    
    //declare the main variables
    GstElement *pipeline, *source, *sink;
    GstBus *bus;
    GstMessage *msg;
    //filters
    GstElement 
        *capabilityfilter, 
        *convertToRGB, 
        *convertFromRGB, 
        *flip, 
        *capsRGB, 
        *encoder, 
        *decoder,
        *h264parse,
        *convertFinal;
    //find the video camera code
    ////////////////////////////////////////////////////////

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

    for (GList *i = devices; i; i = i->next) {
        device = GST_DEVICE(i->data);

        //the next three are just printing the device name/properties/capabilites
        cout << "Device is " << gst_device_get_display_name(device) << "\n" << "\n";

        GstStructure* propString = gst_device_get_properties(device);
        cout << "Properties: " << gst_structure_to_string(propString) << "\n" << "\n";

        GstCaps* capString = gst_device_get_caps(device);
        cout << "Capabilities: " << gst_caps_to_string(capString) << "\n" << "\n";

        //the next part can be removed if you just want the last camera interface
        //for me it has to be video2 (usually the last one seems to be default but not today)
        const gchar *path = gst_structure_get_string(propString, "api.v4l2.path");
        if (path && strcmp(path, "/dev/video2") == 0) {
            gst_structure_free(propString);
            break;
        }
        
        gst_structure_free(propString);
    }

    gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);
    //disclaimer all the code above was just to get the video2 path for the camera
    //create video pipe
    ////////////////////////////////////////////////////////
    
    //this is the process pipeline creation to some default, could be test-pipeline
    pipeline = gst_pipeline_new("camera-pipeline");
    //create the element by using the found camera device
    source = gst_device_create_element(device, "camera");
    //this is the endpoint sink display, we use the standard audiovideosink
    sink = gst_element_factory_make("autovideosink", "display");
    if (!pipeline || !source || !sink) {
        cerr << "Failed to create GStreamer elements\n";
        return 1;
    }   

    //the next piece of code is needed because audiovideosink has no autonegotiation, so we force it
    capabilityfilter = gst_element_factory_make("capsfilter", "camera-caps");

    //you MUST Use the correct width height framerate from the previously printed capabilities in the loop above
    GstCaps *capabilities = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, 1280,
        "height", G_TYPE_INT, 720,
        "framerate", GST_TYPE_FRACTION, 15, 1,
        nullptr
    );
    g_object_set(capabilityfilter, "caps", capabilities, nullptr);
    gst_caps_unref(capabilities);

    //convert the video format
    convertToRGB = gst_element_factory_make("videoconvert", "convertToRGB");
    convertFromRGB = gst_element_factory_make("videoconvert", "convertFromRGB");

    capsRGB = gst_element_factory_make("capsfilter", "capsRGB");

    //you need the same ones here from the list
    GstCaps *capabilitiesRGB = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "RGB",
        nullptr
    );


    g_object_set(capsRGB, "caps", capabilitiesRGB, nullptr);
    gst_caps_unref(capabilitiesRGB);
    
    //color inverter using a pad probe 
    GstPad *rgbPad = gst_element_get_static_pad(capsRGB, "src");

    gst_pad_add_probe(
        rgbPad,
        GST_PAD_PROBE_TYPE_BUFFER,
        invert_colors,
        nullptr,
        nullptr
    );

    gst_object_unref(rgbPad);

    
    //flip upside down
    flip = gst_element_factory_make("videoflip", "flip180");
    g_object_set(flip, "method", 2, nullptr); 

    //encode decoder
    encoder = gst_element_factory_make("x264enc", "encoder");
    g_object_set(encoder, "tune", 0x00000004, nullptr); 

    h264parse = gst_element_factory_make("h264parse", "h264parse");
    decoder = gst_element_factory_make("avdec_h264", "decoder");

    convertFinal = gst_element_factory_make("videoconvert", "convertFinal");

    //need this to bind the pipeline to the source and sink created earlier
    gst_bin_add_many(
        GST_BIN(pipeline), 
        source, 
        capabilityfilter, 
        convertToRGB, 
        capsRGB, 
        flip, 
        convertFromRGB,
        encoder,
        h264parse,  
        decoder,
        convertFinal,
        sink, 
        nullptr
    );

    //connect the input to output in the order
    bool linkSuccess = gst_element_link_many(
        source, 
        capabilityfilter, 
        convertToRGB, 
        capsRGB, 
        flip, 
        convertFromRGB,
        encoder,
        h264parse,
        decoder,
        convertFinal,
        sink, 
        nullptr
    );
    if (!linkSuccess) {
        std::cerr << "FAILED TO LINK PIPELINE\n";
        return 1;
    }


    //starting the actual streaming
    ///////////////////////////////////////////////////////////////////
    
    //start the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    //wait until error or EOS
    bus = gst_element_get_bus(pipeline);

    while (true) {
        //blocking function
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

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