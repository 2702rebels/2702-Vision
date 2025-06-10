MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=vid.o
CXXFLAGS+=-std=c++14 -I../../lib 
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

all: vid

clean:
	rm -f *.o
	rm -f vid

vid: $(OBJS)
	$(CXX) $(LDFLAGS) -o vid $(OBJS) -l2702 -lopencv_core
#	$(CXX) $(CXXFLAGS) -o vid $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath

$(OBJS): ../../lib/v4lcam.h ../../lib/cammgr.h Makefile

