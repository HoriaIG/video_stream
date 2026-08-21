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

    GstDevice *device;

    for (GList *l = devices; l; l = l->next) {
        device = GST_DEVICE(l->data);

        cout << "Device is " << gst_device_get_display_name(device) << "\n";

        GstStructure* propString = gst_device_get_properties(device);
        cout << "Properties: " << gst_structure_to_string(propString) << "\n";

        GstCaps* capString = gst_device_get_caps(device);
        cout << "Capabilities: " << gst_caps_to_string(capString) << "\n";

        
        gst_structure_free(propString);
    }

    gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);


    return 0;
}