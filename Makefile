CXX = g++

CXXFLAGS = -std=c++17 -Wall -Wextra -O2 \
           $(shell pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0)

LDLIBS = $(shell pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0)

TARGETS = main camerafind

MAIN_SRC = main.cpp
CAMERAFIND_SRC = camerafind.cpp
HEADERS = main_headers.h

all: $(TARGETS)

main: $(MAIN_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(MAIN_SRC) -o $@ $(LDLIBS)

camerafind: $(CAMERAFIND_SRC)
	$(CXX) $(CXXFLAGS) $(CAMERAFIND_SRC) -o $@ $(LDLIBS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
