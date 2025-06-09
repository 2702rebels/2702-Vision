CXXFLAGS+=-I../../lib
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
LIBS+=-l2702
LDFLAGS+=-L$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/opencv4.pc)
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/gtk+-x11-3.0.pc)
LIBS+=-lxml2 -lexpat
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/bzip2.pc)
LIBS+=-lapriltag -lwpilibNewCommands -lwpilibc -lcameraserver -lcscore -lntcore -lwpinet -lwpimath -lwpiutil
LIBS+=-luv -lfmt
LIBS+=-lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lm -lz

OBJS=logffmpeg.o

all: logffmpeg

clean:
    rm -f *.o
	rm -f logffmpeg

logffmpeg: $(OBJS)
	$(CXX) -o logffmpeg $(LDFLAGS) $(OBJS) $(LIBS) -lavcodec -lavutil
