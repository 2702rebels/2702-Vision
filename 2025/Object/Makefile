MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=object.o
CXXFLAGS+=--std=c++20
CXXFLAGS+=-I../../lib 
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
CXXFLAGS+=$(shell pkg-config --cflags ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/opencv4.pc)
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/ntcore
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/wpiutil
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/cameraserver
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/cscore
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/wpimath
CXXFLAGS+=$(shell pkg-config --cflags ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/libcamera.pc)
CXXFLAGS+=$(shell pkg-config --cflags ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/gtk+-x11-3.0.pc)
CXXFLAGS+=$(shell pkg-config --cflags ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/libxml-2.0.pc)
CXXFLAGS+=$(shell pkg-config --cflags ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/bzip2.pc)
LDFLAGS+=-L../../lib
LDFLAGS+=-L$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib
LIBS+=-lprotobuf
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/libcamera.pc)
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/opencv4.pc)
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/gtk+-x11-3.0.pc)
LIBS+=-lxml2 -lexpat
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/bzip2.pc)
LIBS+=-lapriltag -lwpilibNewCommands -lwpilibc -lcameraserver -lcscore -lntcore -lwpinet -lwpimath -lwpiutil
LIBS+=-luv -lfmt
all: object replay nettableserver testnetworktablesclient calibrate testudpserver logtostdvideo
norpicam: replay nettableserver testnetworktablesclient calibrate testudpserver logtostdvideo

object: $(OBJS) ../../lib/lib2702.a
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