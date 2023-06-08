TARGET = csrt

ARCH=$(shell bash -c "ldd `which dpkg` | grep libc.so | sed 's:.*/lib/\(.*\)/libc.*:\1:g'")

# ENABLE THESE FOR CUSTOM OPENCV INSTALL
OPENCV_INC="/usr/include/opencv4/"
OPENCV_LIBS="/usr/lib/x86_64-linux-gnu/"
LIBS = -lopencv_core -lopencv_objdetect -lopencv_tracking -lopencv_videoio -lopencv_imgproc -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_ml -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_video -lopencv_videostab -o csrt

# ENABLE THESE TO USE STANDARD ROS OPENCV INSTALL
#OPENCV_INC="/opt/ros/kinetic/include/opencv-3.3.1-dev/"
#OPENCV_LIBS="/opt/ros/kinetic/lib/$(ARCH)/"
#LIBS = -lopencv_core3 -lopencv_objdetect3 -lopencv_tracking3 -lopencv_videoio3 -lopencv_imgproc3 -lopencv_calib3d3 -lopencv_features2d3 -lopencv_flann3 -lopencv_highgui3 -lopencv_ml3 -lopencv_photo3 -lopencv_stitching3 -lopencv_superres3 -lopencv_video3 -lopencv_videostab3 -o csrt

LIB_COMMON = -L $(OPENCV_LIBS) -lgcov

CC = g++

# STEP 0
#CFLAGS = -std=c++14 -march=native -O0 -g3 -Wall -I . -I $(OPENCV_INC)

# STEP 1
CFLAGS = -std=c++14 -march=native -O3 -g0 -fprofile-generate -fprofile-dir=/tmp -Wall -I . -I $(OPENCV_INC)

# STEP 2
#CFLAGS = -std=c++14 -march=native -O3 -g0 -fprofile-use -fprofile-dir=/tmp -fprofile-correction -Wall -I . -I $(OPENCV_INC)

# PROFIT!

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
HEADERS = $(wildcard *.h)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIB_COMMON) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
