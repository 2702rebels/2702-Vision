MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=test.o
CXXFLAGS+=--std=c++20
CXXFLAGS+=-I../lib
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
CXXFLAGS+=$(shell pkg-config --cflags ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/gtest.pc)
CXXFLAGS+=$(shell pkg-config --cflags ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/opencv4.pc)
CXXFLAGS+=-I$(shell pwd)/../vcpkg_installed/$(shell ../findtriplet.sh)/include/ntcore
CXXFLAGS+=-I$(shell pwd)/../vcpkg_installed/$(shell ../findtriplet.sh)/include/wpiutil
CXXFLAGS+=-I$(shell pwd)/../vcpkg_installed/$(shell ../findtriplet.sh)/include/cameraserver
CXXFLAGS+=-I$(shell pwd)/../vcpkg_installed/$(shell ../findtriplet.sh)/include/cscore
CXXFLAGS+=-I$(shell pwd)/../vcpkg_installed/$(shell ../findtriplet.sh)/include/wpimath
CXXFLAGS+=$(shell pkg-config --cflags ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/libcamera.pc)
CXXFLAGS+=$(shell pkg-config --cflags ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/gtk+-x11-3.0.pc)
CXXFLAGS+=$(shell pkg-config --cflags ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/libxml-2.0.pc)
CXXFLAGS+=$(shell pkg-config --cflags ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/bzip2.pc)
LDFLAGS+=-L../lib
LDFLAGS+=-L$(shell pwd)/../vcpkg_installed/$(shell ../findtriplet.sh)/lib
LIBS+=$(shell pkg-config -libs ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/gtest.pc)
LIBS+=-lprotobuf
LIBS+=$(shell pkg-config -libs ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/libcamera.pc)
LIBS+=$(shell pkg-config -libs ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/opencv4.pc)
LIBS+=$(shell pkg-config -libs ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/gtk+-x11-3.0.pc)
LIBS+=-lxml2 -lexpat
LIBS+=$(shell pkg-config -libs ../vcpkg_installed/$(shell ../findtriplet.sh)/lib/pkgconfig/bzip2.pc)
LIBS+=-lapriltag -lwpilibNewCommands -lwpilibc -lcameraserver -lcscore -lntcore -lwpinet -lwpimath -lwpiutil
LIBS+=-luv -lfmt
all: test

test: $(OBJS) ../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o test $(OBJS) -l2702 $(LIBS)

test.o: main.cpp SerializationTest.cpp