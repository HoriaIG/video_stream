# video_stream
## Description

A C++ application showcasing video streaming using GStreamer and H.264 encoding.

## Requirements
### Hardware
- computer
- camera
### Software
- Linux OS
- V4L2-compatible camera
- GStreamer
- GStreamer development libraries
- GStreamer plugins:
    - x264enc for H.264 video encoding
    - avdec_h264 for H.264 video decoding
    - fpsdisplaysink for FPS display and video output
    - videoflip for video rotation
- C++ compiler
- GNU Make

## Preparing and Running

The project contains a Makefile that builds the required executables. 
Run:

```
make
```

This will build both main and camerafind.

### Finding the Camera

camerafind uses GStreamer to enumerate the available V4L2 video devices and display their paths and capabilities.

Run:

```
./camerafind
```


Use the output to determine the desired camera interface and its supported width, height, and framerate.

### Running the Video Stream

The main application accepts four arguments:

```
./main <location> <videoWidth> <videoHeight> <framerate>
```

For example:

```
./main /dev/video2 1280 720 15
```
The width, height, and framerate must match a configuration supported by the selected camera.

## GStreamer Pipeline
- Camera
- YUY2
- videoconvert
- RGB
- Invert colors (pad probe)
- Rotate 180°
- RGB → encoder-compatible format
- H.264 encode
- H.264 parse
- H.264 decode
- Display


The camera stream is converted to RGB, after which the colors are inverted using a GStreamer pad probe. The video is then flipped by 180 degrees.
The processed video is converted to a format suitable for H.264 encoding, encoded, parsed, and decoded before being displayed.