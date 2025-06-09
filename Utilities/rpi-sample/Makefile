MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=rpi-sample.o
CXXFLAGS+=-I../../lib  --std=c++20
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
LDFLAGS+=-L../../lib -L/usr/local/lib
LDFLAGS+=-L$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib
LIBS+=-lprotobuf
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/libcamera.pc)
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/opencv4.pc)
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/gtk+-x11-3.0.pc)
LIBS+=-lxml2 -lexpat
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/bzip2.pc)
LIBS+=-lapriltag -lwpilibNewCommands -lwpilibc -lcameraserver -lcscore -lntcore -lwpinet -lwpimath -lwpiutil
LIBS+=-luv -lfmt

all: rpi-sample

clean:
    rm -f *.o
	rm -f rpi-sample

rpi-sample: $(OBJS) ../../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgproc -lopencv_imgcodecs $(LIBS)
#	$(CXX) $(CXXFLAGS) -o rpi-sample $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lntcore -lcscore -lcameraserver -lwpiutil -lapriltag -lwpimath
