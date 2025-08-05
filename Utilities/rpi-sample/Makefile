MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=rpi-sample.o
CXXFLAGS+=-I../../lib  --std=c++20
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
CXXFLAGS+=-I/usr/local/include/cameraserver
CXXFLAGS+=-I/usr/local/include/wpiutil
CXXFLAGS+=-I/usr/local/include/cscore
CXXFLAGS+=-I/usr/local/include/ntcore
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags libcamera )
CXXFLAGS+=$(shell pkg-config --cflags ../../pkgconfig/ubuntu/wpilib.pc)
LDFLAGS+=-L../../lib -L/usr/local/lib
LIBS+=$(shell pkg-config -libs libcamera)
LIBS+=$(shell pkg-config -libs ../../pkgconfig/ubuntu/wpilib.pc)
LIBS+=-lntcore -lcscore -lcameraserver -lwpiutil

all: rpi-sample

clean:
	rm -f *.o
	rm -f rpi-sample

rpi-sample: $(OBJS) ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o rpi-sample $(OBJS) -l2702 $(LIBS)
#	$(CXX) $(CXXFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath
