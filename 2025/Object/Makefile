MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target
DEFAULT_ARCHIVES+=../../lib/lib2702.a
DEFAULT_ARCHIVES+=liblog.a
CXXFLAGS+=--std=c++20
CXXFLAGS+=-I../../lib 
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags libcamera )
CXXFLAGS+=$(shell pkg-config --cflags ../../pkgconfig/ubuntu/wpilib.pc)
CXXFLAGS+=$(shell pkg-config --cflags spdlog)
LDFLAGS+=-L../../lib
LDFLAGS+=-L/usr/local/lib
LIBS+=$(shell pkg-config -libs libcamera)
LIBS+=$(shell pkg-config -libs opencv4)
LIBS+=$(shell pkg-config -libs spdlog)
LIBS+=-l2702
LIBS+=-lapriltag -lwpilibNewCommands -lwpilibc -lcameraserver -lcscore -lntcore -lwpinet -lwpimath -lwpiutil
LIBS+=-llog
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

object: object.o $(DEFAULT_ARCHIVES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o object object.o -l2702 $(LIBS)

replay: replay.o $(DEFAULT_ARCHIVES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o replay replay.o -l2702 $(LIBS)

#	$(CXX) $(CXXFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath
nettableserver: nettableserver.o $(DEFAULT_ARCHIVES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o nettableserver nettableserver.o -l2702 $(LIBS)

testnetworktablesclient: testnetworktablesclient.o $(DEFAULT_ARCHIVES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testnetworktablesclient testnetworktablesclient.o -l2702 $(LIBS)

calibrate: calibrate.o $(DEFAULT_ARCHIVES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o calibrate calibrate.o -l2702 $(LIBS)

testudpserver: testudpserver.o $(DEFAULT_ARCHIVES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o testudpserver testudpserver.o -l2702 $(LIBS)

logtostdvideo: logtostdvideo.o $(DEFAULT_ARCHIVES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o logtostdvideo logtostdvideo.o -l2702 $(LIBS)

object.o: object.cpp object.h config.h ObjectDetectionStruct.cpp ObjectDetectionStruct.h udpsender.h
replay.o: replay.cpp object.h config.h udpsender.h
nettableserver.o: nettableserver.cpp udpsender.h
testnetworktablesclient.o: testnetworktablesclient.cpp udpsender.h
calibrate.o: calibrate.cpp udpsender.h
testudpserver.o: ObjectDetectionStruct.h ObjectDetectionStruct.cpp testudpserver.cpp udpsender.h
logtostdvideo.o: object.h config.h logtostdvideo.h logtostdvideo.cpp udpsender.h
log.o: LoggingUtils.cpp LoggingUtils.h
	$(CXX) -c LoggingUtils.cpp -o log.o $(CXXFLAGS) $(LDFLAGS) $(LIBS)
liblog.a: log.o
	ar rcs liblog.a log.o