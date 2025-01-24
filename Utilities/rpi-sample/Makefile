MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=rpi-sample.o
CXXFLAGS+=-I../../lib 
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags libcamera )
LDFLAGS+=-L../../lib
LIBS+=$(shell pkg-config -libs libcamera)

rpi-sample: $(OBJS) ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core $(LIBS)
#	$(CXX) $(CXXFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath
