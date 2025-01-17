MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

#CXX=g++
OBJS=vid.o
CXXFLAGS+=-std=c++14 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/include/opencv4 -I../../lib -L../../lib -L.
#CXXFLAGS+=-std=c++14 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/include/opencv4 -Wno-psabi -I../lib -L../lib
#CXXFLAGS+=-pg
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
#CXXFLAGS+=-O2
CXXFLAGS+=-g

vid: $(OBJS)
	$(CXX) $(CXXFLAGS) -o vid $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lopencv_core -lpthread -lX11 -lapril_tagging
#	$(CXX) $(CXXFLAGS) -o vid $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath

$(OBJS): ../../lib/v4lcam.h ../../lib/cammgr.h fbdisp.h Makefile

TSTOBJS=tst.o raw2rgb.o
tst.o: v4lcam.h
tst: $(TSTOBJS)
	$(CXX) $(CXXFLAGS) -o tst $(TSTOBJS) -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lapril_tagging
