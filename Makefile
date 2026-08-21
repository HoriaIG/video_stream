CXX = g++

CXXFLAGS = -std=c++17 -Wall -Wextra -O2 \
           $(shell pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0 opencv4)

LDLIBS = $(shell pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0 opencv4)

TARGET = main
SRC = main.cpp
HEADERS = main_headers.h

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean