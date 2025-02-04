MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

#CXX=g++
OBJS=2025AprilTag.o

CXXFLAGS+=-I../../lib  --std=c++20
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=$(shell pkg-config --cflags wpilib )
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
#CXXFLAGS+=-O2
CXXFLAGS+=-g

LDFLAGS=-g -L../../lib
LDFLAGS+=$(shell pkg-config --libs-only-L wpilib)
LDFLAGS+=$(shell pkg-config --libs-only-L opencv4)
LDFLAGS+=$(shell pkg-config --libs-only-L cuda-12.6)
LIBS+=$(shell pkg-config --libs-only-l wpilib)
LIBS+=$(shell pkg-config --libs-only-l opencv4)
LIBS+=$(shell pkg-config --libs-only-l cuda-12.6)
LIBS+=$(shell pkg-config --libs-only-l cudart-12.6)

2025AprilTag: $(OBJS)
	$(CXX) $(LDFLAGS) -o 2025AprilTag $(OBJS) -l2702 $(LIBS)

$(OBJS): ../../lib/v4lcam.h ../../lib/cammgr.h ../../lib/fbdisp.h Makefile

TSTOBJS=tst.o raw2rgb.o
tst.o: ../../lib/v4lcam.h
tst: $(TSTOBJS)
	$(CXX) $(CXXFLAGS) -o tst $(TSTOBJS) -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lapril_tagging

v4linfo: v4linfo.o
	$(CXX) $(CXXFLAGS) -o v4linfo v4linfo.o
	
# DO NOT DELETE
LDFLAGS+=-L../../lib -L/usr/local/lib
LIBS+=$(shell pkg-config -libs libcamera)
LIBS+=-lntcore -lcscore -lcameraserver -lwpiutil

