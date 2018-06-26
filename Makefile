TARGET = csrt

ARCH=$(shell bash -c "ldd `which dpkg` | grep libc.so | sed 's:.*/lib/\(.*\)/libc.*:\1:g'")

LIBS = -L /opt/ros/kinetic/lib/$(ARCH)/ -lopencv_core3 -lopencv_objdetect3 -lopencv_tracking3 -lopencv_videoio3 -lopencv_imgproc3 -lopencv_core3 -lopencv_calib3d3 -lopencv_core3 -lopencv_features2d3 -lopencv_flann3 -lopencv_highgui3 -lopencv_imgproc3 -lopencv_ml3 -lopencv_objdetect3 -lopencv_photo3 -lopencv_stitching3 -lopencv_superres3 -lopencv_video3 -lopencv_videostab3 -o csrt

CC = g++
#CFLAGS = --verbose -O0 -g3 -Wall -I . -I /opt/ros/kinetic/include/opencv-3.3.1-dev/
CFLAGS = -march=native -O3 -g0 -Wall -I . -I /opt/ros/kinetic/include/opencv-3.3.1-dev/

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
HEADERS = $(wildcard *.h)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
