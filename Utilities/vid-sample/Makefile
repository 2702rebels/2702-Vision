MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=vid.o
CXXFLAGS+=-std=c++14 -I../../lib 
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=$(shell pkg-config --cflags ../../pkgconfig/ubuntu/wpilib.pc)
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
LDFLAGS+=-L../../lib
LIBS+=$(shell pkg-config -libs ../../pkgconfig/ubuntu/wpilib.pc)

vid: $(OBJS)
	$(CXX) $(LDFLAGS) -o vid $(OBJS) -l2702 -lopencv_core
#	$(CXX) $(CXXFLAGS) -o vid $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath

$(OBJS): ../../lib/v4lcam.h ../../lib/cammgr.h Makefile

