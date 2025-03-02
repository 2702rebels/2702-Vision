MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=object.o
CXXFLAGS+=--std=c++20
CXXFLAGS+=-I../../lib 
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags libcamera )
CXXFLAGS+=$(shell pkg-config --cflags ../../pkgconfig/ubuntu/wpilib.pc)
LDFLAGS+=-L../../lib
LIBS+=$(shell pkg-config -libs libcamera)
LIBS+=$(shell pkg-config -libs ../../pkgconfig/ubuntu/wpilib.pc)
LIBS+=$(shell pkg-config -libs opencv4)
all: object replay nettableserver testnetworktablesclient udppacketstest calibrate testudpserver

object: $(OBJS) ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o object $(OBJS) -l2702 -lopencv_core -lopencv_imgproc -lopencv_highgui $(LIBS)

replay: replay.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o replay replay.o -l2702 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs $(LIBS)

#	$(CXX) $(CXXFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath
nettableserver: nettableserver.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o nettableserver nettableserver.o -l2702 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs $(LIBS)

testnetworktablesclient: testnetworktablesclient.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testnetworktablesclient testnetworktablesclient.o -l2702 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs $(LIBS)

udppacketstest: udppacketstest.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o udppacketstest udppacketstest.o -l2702 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs $(LIBS)

calibrate: calibrate.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o calibrate calibrate.o -l2702 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs $(LIBS)

testudpserver: testudpserver.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testudpserver testudpserver.o -l2702 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs $(LIBS)

object.o: object.cpp object.h config.h ObjectDetectionStruct.cpp ObjectDetectionStruct.h
replay.o: replay.cpp object.h config.h
nettableserver.o: nettableserver.cpp
testnetworktablesclient.o: testnetworktablesclient.cpp
udppacketstest.o: ObjectDetectionStruct.h ObjectDetectionStruct.cpp udppacketstest.cpp
calibrate.o: calibrate.cpp
testudpserver.o: ObjectDetectionStruct.h ObjectDetectionStruct.cpp testudpserver.cpp