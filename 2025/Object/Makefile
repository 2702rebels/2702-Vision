MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=object.o
CXXFLAGS+=-I../../lib 
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags libcamera )
LDFLAGS+=-L../../lib
LIBS+=$(shell pkg-config -libs libcamera)

object: $(OBJS) ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o object $(OBJS) -l2702 -lopencv_core -lopencv_imgproc -lopencv_highgui $(LIBS)

#	$(CXX) $(CXXFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath

object.o: object.cpp object.h
