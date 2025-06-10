OBJS=log2mjpeg.o
#CXXFLAGS+=-std=c++20 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/frc/include/opencv4 -Wno-psabi -fopenmp -Wno-deprecated-enum-enum-conversion
#CXXFLAGS+=-std=c++20 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/frc/include/opencv4 -Wno-psabi -Wno-deprecated-enum-enum-conversion -I../../lib -L../../lib
CXXFLAGS+=-std=c++20 -Wno-psabi -Wno-deprecated-enum-enum-conversion -I../../lib -L../../lib
CXXFLAGS+=$(shell pkg-config --cflags ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/opencv4.pc)
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/ntcore
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/wpiutil
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/cameraserver
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/cscore
CXXFLAGS+=-I$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/include/wpimath
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
#CXXFLAGS+=-O2
LDFLAGS+=-L$(shell pwd)/../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib
LIBS+=-lprotobuf
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/libcamera.pc)
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/opencv4.pc)
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/gtk+-x11-3.0.pc)
LIBS+=-lxml2 -lexpat
LIBS+=$(shell pkg-config -libs ../../vcpkg_installed/$(shell ../../findtriplet.sh)/lib/pkgconfig/bzip2.pc)
LIBS+=-lapriltag -lwpilibNewCommands -lwpilibc -lcameraserver -lcscore -lntcore -lwpinet -lwpimath -lwpiutil
LIBS+=-luv -lfmt

all: log2mjpeg

clean:
	rm -f *.o
	rm log2mjpeg

log2mjpeg: $(OBJS)
	$(CXX) $(CXXFLAGS) -o log2mjpeg $(OBJS) $(LDFLAGS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lpthread

#$(CXX) $(CXXFLAGS) -o log2mjpeg $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lpthread -lntcore -lcscore -lcameraserver -lwpiutil

#$(OBJS): v4lcam-mp.h bpicam.h

