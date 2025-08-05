MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

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
all: object replay nettableserver testnetworktablesclient calibrate testudpserver logtostdvideo
norpicam: replay nettableserver testnetworktablesclient calibrate testudpserver logtostdvideo
clean:
	rm -f *.o
	rm -f object
	rm -f replay
	rm -f nettableserver
	rm -f testnetworktablesclient
	rm -f calibrate
	rm -f testudpserver
	rm -f logtostdvideo

object: object.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o object $(OBJS) -l2702 $(LIBS)

replay: replay.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o replay replay.o -l2702 $(LIBS)

#	$(CXX) $(CXXFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath
nettableserver: nettableserver.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o nettableserver nettableserver.o -l2702 $(LIBS)

testnetworktablesclient: testnetworktablesclient.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testnetworktablesclient testnetworktablesclient.o -l2702 $(LIBS)

calibrate: calibrate.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o calibrate calibrate.o -l2702 $(LIBS)

testudpserver: testudpserver.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testudpserver testudpserver.o -l2702 $(LIBS)

logtostdvideo: logtostdvideo.o ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o logtostdvideo logtostdvideo.o -l2702 $(LIBS)

object.o: object.cpp object.h config.h ObjectDetectionStruct.cpp ObjectDetectionStruct.h udpsender.h
replay.o: replay.cpp object.h config.h udpsender.h
nettableserver.o: nettableserver.cpp udpsender.h
testnetworktablesclient.o: testnetworktablesclient.cpp udpsender.h
calibrate.o: calibrate.cpp udpsender.h
testudpserver.o: ObjectDetectionStruct.h ObjectDetectionStruct.cpp testudpserver.cpp udpsender.h
logtostdvideo.o: object.h config.h logtostdvideo.h logtostdvideo.cpp udpsender.h
