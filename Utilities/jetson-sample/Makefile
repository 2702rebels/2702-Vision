MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

#CXX=g++
OBJS=jetson-sample.o
CXXFLAGS+=-std=c++20 -I/usr/local/frc/include -L/usr/local/frc/lib -I../../lib -L../../lib -L.
#CXXFLAGS+=-std=c++14 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/include/opencv4 -Wno-psabi -I../lib -L../lib
#CXXFLAGS+=-pg
CXXFLAGS+=-I/usr/local/include/cameraserver
CXXFLAGS+=-I/usr/local/include/wpiutil
CXXFLAGS+=-I/usr/local/include/cscore
CXXFLAGS+=-I/usr/local/include/ntcore
#CXXFLAGS+=$(shell pkg-config --cflags opencv4)
CXXFLAGS+=-I../../2026/2026AprilTag/wpilib/include/cameraserver
CXXFLAGS+=-I../../2026/2026AprilTag/wpilib/include/wpiutil
CXXFLAGS+=-I../../2026/2026AprilTag/wpilib/include/cscore
CXXFLAGS+=-I../../2026/2026AprilTag/wpilib/include/ntcore
CXXFLAGS+=-I../../2026/2026AprilTag/include/opencv4
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
#CXXFLAGS+=-O2
CXXFLAGS+=-g
#LIBS+=$(shell pkg-config -libs libcamera)
LDFLAGS+=-L../../2026/2026AprilTag/wpilib/lib -L../../2026/2026AprilTag/lib
LIBS+=-lntcore -lcscore -lcameraserver -lwpiutil -lcurses

jetson-sample: $(OBJS)
	$(CXX) $(CXXFLAGS) -o jetson-sample $(OBJS) $(LDFLAGS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lopencv_core -lpthread -lX11 -L/usr/local/cuda/lib64 -lcudart $(LIBS)
#	$(CXX) $(CXXFLAGS) -o jetson-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath

$(OBJS): ../../lib/v4lcam.h ../../lib/cammgr.h Makefile

v4linfo: v4linfo.o
	$(CXX) $(CXXFLAGS) -o v4linfo v4linfo.o
	
# DO NOT DELETE
